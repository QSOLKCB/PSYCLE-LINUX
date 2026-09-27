#!/usr/bin/env python3
"""Negative controls for scoped same-witness onset timing interpretation."""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "phase6c-delayed-retrigger-onset-timing.py"
spec = importlib.util.spec_from_file_location("phase6c_onset_timing", SCRIPT)
assert spec is not None and spec.loader is not None
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def expect_value_error(fn, phrase: str) -> None:
    try:
        fn()
    except ValueError as exc:
        assert phrase in str(exc), exc
    else:
        raise AssertionError("expected ValueError")


old_path = (
    ROOT
    / "phase6c"
    / "evidence"
    / "sequencer-delayed-retrigger-same-witness"
    / "observation.json"
)
base = json.loads(old_path.read_text(encoding="utf-8"))

# Build a validator-shaped synthetic command-bearing pair without claiming that
# the historical #80 observation contained one.
pair = copy.deepcopy(base)
pair["original"].update(
    {
        "outcome": "rendered-twice",
        "inconclusive_reason": None,
        "runtime_command_execution_observed": True,
        "runtime_command_execution_scope": copy.deepcopy(
            m.same.RUNTIME_COMMAND_EXECUTION_SCOPE
        ),
        "render_sha256": "4" * 64,
        "analysis": copy.deepcopy(pair["candidate"]["analysis"]),
        "process_exit_code": None,
        "fresh_render_event_binding": "accepted",
        "fresh_render_event_binding_error": None,
    }
)
pair["comparison"].update(
    {
        "same_onset_analyzer": True,
        "command_bearing_runtime_pair_observed": True,
    }
)
pair["derived_observation"].update(
    {
        "original_command_execution_observed": True,
        "command_bearing_runtime_pair_observed": True,
    }
)
m.observation_module.validate_projection(pair)


def sync_onset_beats(analysis: dict) -> None:
    analysis["onset_beats"] = [
        round(frame / m.BEAT_FRAMES, 9) for frame in analysis["onset_frames"]
    ]


def sync_window_counts(analysis: dict) -> None:
    frames = analysis["onset_frames"]
    analysis["window_onset_counts"] = {
        "note_delay_beat_0": sum(
            1 for frame in frames if 0.0 <= frame / m.BEAT_FRAMES < 1.0
        ),
        "retrigger_beat_1": sum(
            1 for frame in frames if 1.0 <= frame / m.BEAT_FRAMES < 2.0
        ),
        "retr_cont_beat_2": sum(
            1 for frame in frames if 2.0 <= frame / m.BEAT_FRAMES < 3.0
        ),
        "extended_marker_beat_3": sum(
            1 for frame in frames if 3.0 <= frame / m.BEAT_FRAMES < 4.0
        ),
    }


timing_pass = m.derive_timing(
    pair,
    input_path="synthetic-qualified-observation.json",
    input_sha256="1" * 64,
)
assert timing_pass["scoped_timing_status"] == "PASS"
assert timing_pass["whole_contract_parity_status"] == "UNKNOWN"
assert timing_pass["whole_contract_classification_allowed"] is False
assert timing_pass["absolute_phase_is_parity_classifying"] is False
for entry in timing_pass["windows"].values():
    assert entry["relative_onset_frames_exact_match"] is True
    assert all(value == 0 for value in entry["original_minus_candidate_relative_frames"])

uniform_shift = copy.deepcopy(pair)
candidate_frames = [1000, 20000, 30000, 38000, 39000, 48000, 57000, 60000]
original_frames = [frame + 1000 for frame in candidate_frames]
uniform_shift["candidate"]["analysis"]["onset_frames"] = candidate_frames
uniform_shift["original"]["analysis"]["onset_frames"] = original_frames
for role in ("candidate", "original"):
    sync_onset_beats(uniform_shift[role]["analysis"])
    sync_window_counts(uniform_shift[role]["analysis"])
uniform_shift_timing = m.derive_timing(
    uniform_shift,
    input_path="synthetic-qualified-observation.json",
    input_sha256="9" * 64,
)
assert uniform_shift_timing["scoped_timing_status"] == "PASS"
assert (
    uniform_shift_timing["phase_alignment"]["original_minus_candidate_frames"]
    == 1000
)
assert all(
    entry["first_onset_phase_delta_frames"] == 1000
    and entry["relative_onset_frames_exact_match"] is True
    for entry in uniform_shift_timing["windows"].values()
)

different = copy.deepcopy(pair)
# Shift only a later FB onset by one sample. Beat membership stays unchanged,
# but the within-window relative timing vector must become DIFFERENT.
different["original"]["analysis"]["onset_frames"][2] += 1
sync_onset_beats(different["original"]["analysis"])
timing_different = m.derive_timing(
    different,
    input_path="synthetic-qualified-observation.json",
    input_sha256="2" * 64,
)
assert timing_different["scoped_timing_status"] == "DIFFERENT"
assert (
    timing_different["windows"]["fb_retrigger_beat_1"][
        "relative_onset_frames_exact_match"
    ]
    is False
)
assert timing_different["whole_contract_parity_status"] == "UNKNOWN"

stale_beats = copy.deepcopy(pair)
stale_beats["original"]["analysis"]["onset_frames"][2] += 1
expect_value_error(
    lambda: m.derive_timing(
        stale_beats,
        input_path="synthetic.json",
        input_sha256="2" * 64,
    ),
    "onset beats do not match onset frames",
)

phase_skew = copy.deepcopy(pair)
phase_skew_analysis = phase_skew["original"]["analysis"]
for index, frame in enumerate(list(phase_skew_analysis["onset_frames"])):
    beat = frame / m.BEAT_FRAMES
    if 1.0 <= beat < 2.0:
        phase_skew_analysis["onset_frames"][index] += 1
sync_onset_beats(phase_skew_analysis)
phase_skew_timing = m.derive_timing(
    phase_skew,
    input_path="synthetic-qualified-observation.json",
    input_sha256="2" * 64,
)
assert phase_skew_timing["scoped_timing_status"] == "DIFFERENT"
assert all(
    entry["relative_onset_frames_exact_match"]
    for entry in phase_skew_timing["windows"].values()
)
assert len(
    {
        entry["first_onset_phase_delta_frames"]
        for entry in phase_skew_timing["windows"].values()
    }
) == 2

inconclusive_original = copy.deepcopy(pair)
inconclusive_original["original"].update(
    {
        "outcome": "inconclusive",
        "inconclusive_reason": "ambiguous-final-render-attempt",
        "fresh_render_event_binding": "rejected",
        "fresh_render_event_binding_error": "synthetic rejected binding",
    }
)
expect_value_error(
    lambda: m.derive_timing(
        inconclusive_original,
        input_path="synthetic.json",
        input_sha256="8" * 64,
    ),
    "successful bound original render",
)

missing_pair = copy.deepcopy(pair)
missing_pair["comparison"]["command_bearing_runtime_pair_observed"] = False
missing_pair["derived_observation"]["command_bearing_runtime_pair_observed"] = False
expect_value_error(
    lambda: m.derive_timing(
        missing_pair,
        input_path="synthetic.json",
        input_sha256="3" * 64,
    ),
    "requires a command-bearing runtime pair",
)

wrong_scope = copy.deepcopy(pair)
wrong_scope["original"]["runtime_command_execution_scope"]["established_effects"].append(
    "FD 7F note-delay effect"
)
expect_value_error(
    lambda: m.derive_timing(
        wrong_scope,
        input_path="synthetic.json",
        input_sha256="4" * 64,
    ),
    "command-execution scope changed",
)

promoted = copy.deepcopy(timing_pass)
promoted["whole_contract_parity_status"] = "PASS"
expect_value_error(
    lambda: m.validate_timing(promoted),
    "onset-timing envelope is invalid",
)

bad_status = copy.deepcopy(timing_different)
bad_status["scoped_timing_status"] = "PASS"
expect_value_error(
    lambda: m.validate_timing(bad_status),
    "scoped timing status is inconsistent",
)

bad_vector = copy.deepcopy(timing_pass)
bad_vector["windows"]["fb_retrigger_beat_1"]["candidate"]["relative_frames"][1] += 1
expect_value_error(
    lambda: m.validate_timing(bad_vector),
    "timing derivation is inconsistent",
)

bad_ordinal = copy.deepcopy(timing_pass)
bad_ordinal["windows"]["fb_retrigger_beat_1"][
    "original_minus_candidate_relative_frames"
][1] += 1
expect_value_error(
    lambda: m.validate_timing(bad_ordinal),
    "relative-frame diagnostic is inconsistent",
)

bad_phase = copy.deepcopy(timing_pass)
bad_phase["windows"]["fb_retrigger_beat_1"]["first_onset_phase_delta_frames"] += 1
expect_value_error(
    lambda: m.validate_timing(bad_phase),
    "first-onset phase diagnostic is inconsistent",
)

with tempfile.TemporaryDirectory(dir=ROOT) as temporary:
    temporary_root = Path(temporary)
    bound_observation = temporary_root / "qualified-observation.json"
    bound_raw = (
        json.dumps(pair, indent=2, sort_keys=True) + "\n"
    ).encode("utf-8")
    bound_observation.write_bytes(bound_raw)
    relative_bound_observation = bound_observation.relative_to(ROOT).as_posix()
    bound_timing = m.derive_timing(
        pair,
        input_path=relative_bound_observation,
        input_sha256=m.digest(bound_raw),
    )
    assert m.validate_bound_receipt(copy.deepcopy(bound_timing)) == bound_timing

    expect_value_error(
        lambda: m.derive_timing(
            pair,
            input_path=bound_observation.resolve().as_posix(),
            input_sha256=m.digest(bound_raw),
        ),
        "input binding is invalid",
    )

    forged_hash = copy.deepcopy(bound_timing)
    forged_hash["input_observation"]["sha256"] = "f" * 64
    expect_value_error(
        lambda: m.validate_bound_receipt(forged_hash),
        "bound input observation hash mismatch",
    )

    forged_receipt = copy.deepcopy(bound_timing)
    for role in ("candidate", "original"):
        window = forged_receipt["windows"]["fb_retrigger_beat_1"][role]
        window["absolute_frames"] = [value + 5 for value in window["absolute_frames"]]
        window["first_onset_frame"] += 5
    assert m.validate_timing(forged_receipt) == forged_receipt
    expect_value_error(
        lambda: m.validate_bound_receipt(forged_receipt),
        "differs from bound observation derivation",
    )

with tempfile.TemporaryDirectory() as temporary:
    output = Path(temporary) / "onset-timing.json"
    m.write_new(output, timing_pass)
    assert json.loads(output.read_text(encoding="utf-8")) == timing_pass
    try:
        m.write_new(output, timing_pass)
    except FileExistsError:
        pass
    else:
        raise AssertionError("timing receipt write must be no-clobber")

print("phase6c-delayed-retrigger-onset-timing: PASS")
