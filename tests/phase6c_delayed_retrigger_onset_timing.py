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

different = copy.deepcopy(pair)
# Shift only a later FB onset by one sample. Beat membership stays unchanged,
# but the within-window relative timing vector must become DIFFERENT.
different["original"]["analysis"]["onset_frames"][2] += 1
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
