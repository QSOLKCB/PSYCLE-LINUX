#!/usr/bin/env python3
"""Interpret Phase 6C same-witness FB/FA onset timing without scope expansion."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OBSERVATION_SCRIPT = (
    ROOT / "scripts" / "phase6c-delayed-retrigger-same-witness-observation.py"
)
spec = importlib.util.spec_from_file_location(
    "phase6c_same_witness_observation", OBSERVATION_SCRIPT
)
if spec is None or spec.loader is None:
    raise RuntimeError("could not load same-witness observation validator")
observation_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(observation_module)

same = observation_module.same

CONTRACT = "sequencer-delayed-retrigger-same-witness-onset-timing"
INPUT_CONTRACT = observation_module.CONTRACT
BPM = 137
EXPECTED_SAMPLE_RATE = 44100
BEAT_FRAMES = EXPECTED_SAMPLE_RATE * 60.0 / BPM
NORMALIZATION = (
    "derive one render-wide original-minus-candidate phase offset from the first "
    "observed onset, apply it before command-window membership, then subtract each "
    "side's first onset in each classified window and require every classified "
    "window to retain that same render-wide phase delta"
)
WINDOWS = {
    "fb_retrigger_beat_1": {
        "label": "FB 3F retrigger",
        "start_beat": 1.0,
        "end_beat": 2.0,
    },
    "fa_retrigger_continue_beat_2": {
        "label": "FA 42 retrigger-continue",
        "start_beat": 2.0,
        "end_beat": 3.0,
    },
}
OUTPUT_PATH = (
    ROOT
    / "phase6c"
    / "evidence"
    / "sequencer-delayed-retrigger-qualified-same-witness"
    / "onset-timing.json"
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path}: expected JSON object")
    return value


def require_int_list(value: object, label: str) -> list[int]:
    if (
        not isinstance(value, list)
        or not value
        or any(type(item) is not int or item < 0 for item in value)
    ):
        raise ValueError(f"{label} must be a non-empty list of non-negative integers")
    return value


def require_number_list(value: object, label: str) -> list[float]:
    if (
        not isinstance(value, list)
        or not value
        or any(
            not isinstance(item, (int, float))
            or isinstance(item, bool)
            for item in value
        )
    ):
        raise ValueError(f"{label} must be a non-empty numeric list")
    return [float(item) for item in value]


def validate_analysis(analysis: object, role: str) -> dict:
    if not isinstance(analysis, dict):
        raise ValueError(f"{role} onset analysis is not an object")
    frames = require_int_list(analysis.get("onset_frames"), f"{role}.onset_frames")
    beats = require_number_list(analysis.get("onset_beats"), f"{role}.onset_beats")
    if len(frames) != len(beats):
        raise ValueError(f"{role} onset frame/beat lengths differ")
    if frames != sorted(frames) or len(frames) != len(set(frames)):
        raise ValueError(f"{role} onset frames are not strictly increasing")
    if (
        analysis.get("sample_rate") != EXPECTED_SAMPLE_RATE
        or analysis.get("channels") != 1
        or analysis.get("bits_per_sample") != 16
    ):
        raise ValueError(f"{role} render format changed")

    expected_beats = [round(frame / BEAT_FRAMES, 9) for frame in frames]
    if beats != expected_beats:
        raise ValueError(f"{role} onset beats do not match onset frames")

    counts = analysis.get("window_onset_counts")
    if not isinstance(counts, dict):
        raise ValueError(f"{role} onset-window counts are missing")
    expected_counts = {
        "retrigger_beat_1": sum(
            1 for frame in frames if 1.0 <= frame / BEAT_FRAMES < 2.0
        ),
        "retr_cont_beat_2": sum(
            1 for frame in frames if 2.0 <= frame / BEAT_FRAMES < 3.0
        ),
    }
    for key, expected_count in expected_counts.items():
        if (
            type(counts.get(key)) is not int
            or counts[key] != expected_count
        ):
            raise ValueError(
                f"{role} does not retain frame-derived command-bearing {key} onsets"
            )
    return analysis


def extract_window(
    analysis: dict,
    start: float,
    end: float,
    label: str,
    *,
    phase_alignment_frames: int = 0,
) -> dict:
    frames = analysis["onset_frames"]
    selected = [
        frame
        for frame in frames
        if start <= (frame - phase_alignment_frames) / BEAT_FRAMES < end
    ]
    if len(selected) < 2:
        raise ValueError(f"{label} does not contain multiple command onsets")
    relative = [frame - selected[0] for frame in selected]
    intervals = [
        selected[index] - selected[index - 1]
        for index in range(1, len(selected))
    ]
    return {
        "absolute_frames": selected,
        "relative_frames": relative,
        "interval_frames": intervals,
        "first_onset_frame": selected[0],
    }


def derive_timing(observation: dict, *, input_path: str, input_sha256: str) -> dict:
    observation_module.validate_projection(observation)
    if observation.get("contract") != INPUT_CONTRACT:
        raise ValueError("same-witness timing input contract changed")

    comparison = observation.get("comparison")
    candidate = observation.get("candidate")
    original = observation.get("original")
    derived = observation.get("derived_observation")
    if not all(
        isinstance(value, dict)
        for value in (comparison, candidate, original, derived)
    ):
        raise ValueError("same-witness timing input sections are malformed")

    if (
        comparison.get("command_bearing_runtime_pair_observed") is not True
        or comparison.get("same_onset_analyzer") is not True
        or candidate.get("runtime_command_execution_observed") is not True
        or original.get("runtime_command_execution_observed") is not True
        or derived.get("command_bearing_runtime_pair_observed") is not True
        or derived.get("timing_classification_permitted") is not False
    ):
        raise ValueError(
            "same-witness timing interpretation requires a command-bearing runtime pair"
        )

    if (
        original.get("outcome") != "rendered-twice"
        or original.get("inconclusive_reason") is not None
        or original.get("fresh_render_event_binding") != "accepted"
        or original.get("fresh_render_event_binding_error") is not None
        or original.get("process_exit_code") is not None
    ):
        raise ValueError(
            "same-witness timing interpretation requires a successful bound "
            "original render pair"
        )

    expected_scope = same.RUNTIME_COMMAND_EXECUTION_SCOPE
    if (
        candidate.get("runtime_command_execution_scope") != expected_scope
        or original.get("runtime_command_execution_scope") != expected_scope
    ):
        raise ValueError("same-witness command-execution scope changed")

    candidate_analysis = validate_analysis(candidate.get("analysis"), "candidate")
    original_analysis = validate_analysis(original.get("analysis"), "original")

    candidate_anchor_frame = candidate_analysis["onset_frames"][0]
    original_anchor_frame = original_analysis["onset_frames"][0]
    global_phase_delta = original_anchor_frame - candidate_anchor_frame

    windows: dict[str, dict] = {}
    all_relative_exact = True
    first_onset_phase_deltas: list[int] = []
    for key, window in WINDOWS.items():
        candidate_window = extract_window(
            candidate_analysis,
            window["start_beat"],
            window["end_beat"],
            f"candidate {window['label']}",
        )
        original_window = extract_window(
            original_analysis,
            window["start_beat"],
            window["end_beat"],
            f"original {window['label']}",
            phase_alignment_frames=global_phase_delta,
        )
        relative_exact = (
            candidate_window["relative_frames"]
            == original_window["relative_frames"]
        )
        all_relative_exact = all_relative_exact and relative_exact
        ordinal_delta = None
        if len(candidate_window["relative_frames"]) == len(
            original_window["relative_frames"]
        ):
            ordinal_delta = [
                original_value - candidate_value
                for candidate_value, original_value in zip(
                    candidate_window["relative_frames"],
                    original_window["relative_frames"],
                )
            ]
        phase_delta = (
            original_window["first_onset_frame"]
            - candidate_window["first_onset_frame"]
        )
        first_onset_phase_deltas.append(phase_delta)
        windows[key] = {
            "label": window["label"],
            "beat_window": [window["start_beat"], window["end_beat"]],
            "candidate": candidate_window,
            "original": original_window,
            "relative_onset_frames_exact_match": relative_exact,
            "original_minus_candidate_relative_frames": ordinal_delta,
            "first_onset_phase_delta_frames": phase_delta,
        }

    phase_delta_consistent = all(
        phase_delta == global_phase_delta
        for phase_delta in first_onset_phase_deltas
    )
    scoped_match = all_relative_exact and phase_delta_consistent
    scoped_status = "PASS" if scoped_match else "DIFFERENT"
    if scoped_match:
        interpretation = (
            "After one render-wide phase alignment, the FB/FA command-bearing "
            "relative onset vectors match exactly at 44.1 kHz and each command "
            "window retains that same original-minus-candidate phase delta."
        )
    elif not all_relative_exact:
        interpretation = (
            "At least one FB/FA command-bearing relative onset vector differs "
            "at sample resolution between original and candidate."
        )
    else:
        interpretation = (
            "The FB/FA within-window relative onset vectors match, but at least "
            "one command window does not retain the render-wide phase delta; "
            "this is not explainable by one fixed renderer/start latency."
        )

    result = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "input_observation": {
            "path": input_path,
            "sha256": input_sha256,
            "contract": INPUT_CONTRACT,
            "workflow_run_id": observation["canonical_observation"][
                "workflow_run_id"
            ],
            "workflow_head_sha": observation["canonical_observation"][
                "workflow_head_sha"
            ],
        },
        "scope": {
            "classification": "FB/FA retrigger-family relative onset timing",
            "established_effects": list(expected_scope["established_effects"]),
            "not_established": list(expected_scope["not_established"]),
            "normalization": NORMALIZATION,
            "sample_rate": EXPECTED_SAMPLE_RATE,
            "bpm": BPM,
        },
        "phase_alignment": {
            "anchor": "first-observed-onset",
            "candidate_anchor_frame": candidate_anchor_frame,
            "original_anchor_frame": original_anchor_frame,
            "original_minus_candidate_frames": global_phase_delta,
        },
        "windows": windows,
        "scoped_timing_status": scoped_status,
        "whole_contract_parity_status": "UNKNOWN",
        "whole_contract_classification_allowed": False,
        "absolute_phase_is_parity_classifying": False,
        "interpretation": interpretation,
        "interpretation_boundary": (
            "This receipt classifies only sample-exact relative onset geometry "
            "for the already-established FB 3F and FA 42 effects. One render-wide "
            "phase delta, anchored by the first observed onset, is applied before "
            "command-window membership and may be ignored as fixed renderer/start "
            "latency, but command-specific phase skew is a timing difference. Absolute phase "
            "itself remains diagnostic rather than parity-classifying. FD 7F note-"
            "delay and FE 04 extended-command behavior remain outside "
            "this witness's established runtime scope, so sequencer-delayed-"
            "retrigger remains UNKNOWN as a whole."
        ),
    }
    validate_timing(result)
    return result


def validate_timing(value: object) -> dict:
    if not isinstance(value, dict):
        raise ValueError("same-witness onset-timing receipt is not an object")
    if (
        value.get("schema_version") != 1
        or value.get("phase") != "6C"
        or value.get("contract") != CONTRACT
        or value.get("scoped_timing_status") not in {"PASS", "DIFFERENT"}
        or value.get("whole_contract_parity_status") != "UNKNOWN"
        or value.get("whole_contract_classification_allowed") is not False
        or value.get("absolute_phase_is_parity_classifying") is not False
    ):
        raise ValueError("same-witness onset-timing envelope is invalid")

    source = value.get("input_observation")
    if (
        not isinstance(source, dict)
        or not isinstance(source.get("path"), str)
        or not source.get("path")
        or Path(source["path"]).is_absolute()
        or source.get("contract") != INPUT_CONTRACT
        or not isinstance(source.get("workflow_run_id"), int)
        or isinstance(source.get("workflow_run_id"), bool)
        or source["workflow_run_id"] <= 0
        or not isinstance(source.get("workflow_head_sha"), str)
        or len(source["workflow_head_sha"]) != 40
    ):
        raise ValueError("same-witness onset-timing input binding is invalid")
    observation_module.require_sha256(
        source.get("sha256"), "input_observation.sha256"
    )
    resolve_bound_observation_path(source["path"])

    scope = value.get("scope")
    if (
        not isinstance(scope, dict)
        or scope.get("classification")
        != "FB/FA retrigger-family relative onset timing"
        or scope.get("established_effects")
        != same.RUNTIME_COMMAND_EXECUTION_SCOPE["established_effects"]
        or scope.get("not_established")
        != same.RUNTIME_COMMAND_EXECUTION_SCOPE["not_established"]
        or scope.get("normalization") != NORMALIZATION
        or scope.get("sample_rate") != EXPECTED_SAMPLE_RATE
        or scope.get("bpm") != BPM
    ):
        raise ValueError("same-witness onset-timing scope changed")

    alignment = value.get("phase_alignment")
    if (
        not isinstance(alignment, dict)
        or alignment.get("anchor") != "first-observed-onset"
        or type(alignment.get("candidate_anchor_frame")) is not int
        or alignment["candidate_anchor_frame"] < 0
        or type(alignment.get("original_anchor_frame")) is not int
        or alignment["original_anchor_frame"] < 0
        or type(alignment.get("original_minus_candidate_frames")) is not int
        or alignment["original_minus_candidate_frames"]
        != alignment["original_anchor_frame"] - alignment["candidate_anchor_frame"]
    ):
        raise ValueError("same-witness onset-timing phase alignment is invalid")
    global_phase_delta = alignment["original_minus_candidate_frames"]

    windows = value.get("windows")
    if not isinstance(windows, dict) or set(windows) != set(WINDOWS):
        raise ValueError("same-witness onset-timing windows changed")

    exact_flags = []
    phase_deltas = []
    for key, definition in WINDOWS.items():
        entry = windows.get(key)
        if (
            not isinstance(entry, dict)
            or entry.get("label") != definition["label"]
            or entry.get("beat_window")
            != [definition["start_beat"], definition["end_beat"]]
            or not isinstance(
                entry.get("relative_onset_frames_exact_match"), bool
            )
        ):
            raise ValueError(f"same-witness onset-timing {key} shape changed")
        for role in ("candidate", "original"):
            data = entry.get(role)
            if not isinstance(data, dict):
                raise ValueError(f"{key}.{role} timing data is missing")
            absolute = require_int_list(
                data.get("absolute_frames"), f"{key}.{role}.absolute_frames"
            )
            relative = require_int_list(
                data.get("relative_frames"), f"{key}.{role}.relative_frames"
            )
            intervals = require_int_list(
                data.get("interval_frames"), f"{key}.{role}.interval_frames"
            )
            if (
                relative[0] != 0
                or len(relative) != len(absolute)
                or len(intervals) != len(absolute) - 1
                or relative
                != [frame - absolute[0] for frame in absolute]
                or intervals
                != [
                    absolute[index] - absolute[index - 1]
                    for index in range(1, len(absolute))
                ]
                or data.get("first_onset_frame") != absolute[0]
            ):
                raise ValueError(f"{key}.{role} timing derivation is inconsistent")
        candidate_relative = entry["candidate"]["relative_frames"]
        original_relative = entry["original"]["relative_frames"]
        expected_ordinal_delta = None
        if len(candidate_relative) == len(original_relative):
            expected_ordinal_delta = [
                original_value - candidate_value
                for candidate_value, original_value in zip(
                    candidate_relative, original_relative
                )
            ]
        if (
            entry.get("original_minus_candidate_relative_frames")
            != expected_ordinal_delta
        ):
            raise ValueError(f"{key} relative-frame diagnostic is inconsistent")

        expected_phase_delta = (
            entry["original"]["first_onset_frame"]
            - entry["candidate"]["first_onset_frame"]
        )
        if entry.get("first_onset_phase_delta_frames") != expected_phase_delta:
            raise ValueError(f"{key} first-onset phase diagnostic is inconsistent")
        phase_deltas.append(expected_phase_delta)

        exact = candidate_relative == original_relative
        if entry["relative_onset_frames_exact_match"] is not exact:
            raise ValueError(f"{key} exact-match flag is inconsistent")
        exact_flags.append(exact)

    phase_delta_consistent = all(
        phase_delta == global_phase_delta for phase_delta in phase_deltas
    )
    expected_status = (
        "PASS" if all(exact_flags) and phase_delta_consistent else "DIFFERENT"
    )
    if value.get("scoped_timing_status") != expected_status:
        raise ValueError("same-witness scoped timing status is inconsistent")
    return value


def resolve_bound_observation_path(source_path: str) -> Path:
    path = Path(source_path)
    if path.is_absolute():
        raise ValueError("input observation path must be repository-relative")
    root = ROOT.resolve()
    resolved = (root / path).resolve()
    try:
        resolved.relative_to(root)
    except ValueError as exc:
        raise ValueError("input observation path escapes repository root") from exc
    return resolved


def validate_bound_receipt(value: object) -> dict:
    receipt = validate_timing(value)
    source = receipt["input_observation"]
    observation_path = resolve_bound_observation_path(source["path"])
    try:
        raw = observation_path.read_bytes()
    except OSError as exc:
        raise ValueError(
            f"bound input observation cannot be read: {source['path']}"
        ) from exc

    actual_sha256 = digest(raw)
    if actual_sha256 != source["sha256"]:
        raise ValueError("bound input observation hash mismatch")

    try:
        observation = json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ValueError("bound input observation is not valid UTF-8 JSON") from exc
    if not isinstance(observation, dict):
        raise ValueError("bound input observation is not an object")

    observation_module.validate_projection(observation)
    if observation.get("contract") != INPUT_CONTRACT:
        raise ValueError("bound input observation contract changed")
    canonical = observation.get("canonical_observation")
    if (
        not isinstance(canonical, dict)
        or source["workflow_run_id"] != canonical.get("workflow_run_id")
        or source["workflow_head_sha"] != canonical.get("workflow_head_sha")
    ):
        raise ValueError("bound input observation workflow identity mismatch")

    expected = derive_timing(
        observation,
        input_path=source["path"],
        input_sha256=actual_sha256,
    )
    if receipt != expected:
        raise ValueError(
            "same-witness onset-timing receipt differs from bound observation derivation"
        )
    return receipt


def write_new(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)

    derive = sub.add_parser("derive")
    derive.add_argument("observation", type=Path)
    derive.add_argument("output", type=Path, nargs="?", default=OUTPUT_PATH)

    check = sub.add_parser("check")
    check.add_argument("receipt", type=Path, nargs="?", default=OUTPUT_PATH)

    args = parser.parse_args()
    if args.mode == "derive":
        observation_path = resolve_bound_observation_path(
            args.observation.as_posix()
        )
        input_path = observation_path.relative_to(ROOT.resolve()).as_posix()
        raw = observation_path.read_bytes()
        observation = json.loads(raw.decode("utf-8"))
        if not isinstance(observation, dict):
            raise ValueError("same-witness observation is not an object")
        value = derive_timing(
            observation,
            input_path=input_path,
            input_sha256=digest(raw),
        )
        write_new(args.output, value)
    else:
        value = validate_bound_receipt(read_json(args.receipt))
    print(json.dumps(value, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
