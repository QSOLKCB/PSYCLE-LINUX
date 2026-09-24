#!/usr/bin/env python3
"""Build and validate the Phase 6C original-Psycle execution witness."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path, PurePosixPath
import struct

CONTRACT = "sequencer-delayed-retrigger-execution-observer"
FIXTURE = "delayed-retrigger/phase6c-delayed-retrigger-execution.psy"
CANDIDATE_RECEIPT = "candidate-delayed-retrigger-execution.json"
ORIGINAL_RECEIPT = "original-delayed-retrigger-execution.json"
RUNTIME_RECEIPT = "original-delayed-retrigger-runtime.json"
TITLE = "PSYCLE-LINUX Phase 6C delayed/retrigger execution witness"
REFERENCE_BUILD = "Psycle 1.12.0 x86"
ROOT = Path(__file__).resolve().parents[1]
EXPECTED_LAYOUT = {
    "bpm": 137,
    "lpb": 8,
    "tpb": 24,
    "pattern_beats": 4.0,
    "sample_rate": 44100,
    "commands": [
        {"beat": 0.0, "command": "FD", "parameter": "7F", "role": "note-delay"},
        {"beat": 1.0, "command": "FB", "parameter": "3F", "role": "retrigger"},
        {"beat": 2.0, "command": "FA", "parameter": "42", "role": "retrigger-continue"},
        {"beat": 3.0, "command": "FE", "parameter": "04", "role": "set-lpb"},
        {"beat": 3.125, "command": "00", "parameter": "00", "role": "post-FE marker"},
    ],
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def child(root: Path, relative: str) -> Path:
    if not isinstance(relative, str):
        raise ValueError("unsafe artifact path")
    pure = PurePosixPath(relative)
    if pure.is_absolute() or ".." in pure.parts or "\\" in relative:
        raise ValueError("unsafe artifact path")
    result = (root / relative).resolve()
    result.relative_to(root.resolve())
    return result


def load_module(name: str):
    path = ROOT / "scripts" / name
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ValueError("could not load evidence validator: " + name)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def write_new(path: Path, value: dict) -> None:
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def binding(root: Path, path: Path) -> dict:
    return {
        "path": path.relative_to(root).as_posix(),
        "sha256": digest(path.read_bytes()),
    }


def bound_bytes(root: Path, value: object) -> bytes:
    if (
        not isinstance(value, dict)
        or set(value) != {"path", "sha256"}
        or not isinstance(value.get("path"), str)
        or not isinstance(value.get("sha256"), str)
    ):
        raise ValueError("invalid artifact binding")
    data = child(root, value["path"]).read_bytes()
    if digest(data) != value["sha256"]:
        raise ValueError("artifact hash mismatch: " + value["path"])
    return data


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    fixture = child(root, FIXTURE)
    if not fixture.is_file():
        raise ValueError("execution witness fixture is missing")
    raw = fixture.read_bytes()
    if not raw.startswith(b"PSY3SONG"):
        raise ValueError("execution witness is not PSY3")
    receipt = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-fixture",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "fixture": FIXTURE,
        "fixture_sha256": digest(raw),
        "song_title": TITLE,
        "witness_layout": EXPECTED_LAYOUT,
        "purpose": (
            "additive execution-only witness for pinned original Psycle; "
            "does not replace or mutate the frozen delayed/retrigger comparison fixture"
        ),
        "parity_status": "UNKNOWN",
    }
    write_new(root / CANDIDATE_RECEIPT, receipt)
    return receipt


def validate_candidate(root: Path) -> dict:
    root = root.resolve()
    receipt = read_json(root / CANDIDATE_RECEIPT)
    expected = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-fixture",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "fixture": FIXTURE,
        "song_title": TITLE,
        "witness_layout": EXPECTED_LAYOUT,
        "parity_status": "UNKNOWN",
    }
    for key, value in expected.items():
        if receipt.get(key) != value:
            raise ValueError("candidate execution witness identity mismatch: " + key)
    fixture = child(root, FIXTURE)
    if digest(fixture.read_bytes()) != receipt.get("fixture_sha256"):
        raise ValueError("candidate execution witness hash mismatch")
    if not fixture.read_bytes().startswith(b"PSY3SONG"):
        raise ValueError("candidate execution witness is not PSY3")
    return receipt


def parse_pcm16_wave(data: bytes) -> dict:
    if len(data) < 44 or data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError("render is not a RIFF/WAVE file")
    riff_size = struct.unpack_from("<I", data, 4)[0]
    riff_end = 8 + riff_size
    if riff_end != len(data):
        if riff_end < len(data):
            raise ValueError("WAV has trailing bytes beyond declared RIFF extent")
        raise ValueError("truncated RIFF/WAVE file")
    if riff_end < 12:
        raise ValueError("invalid RIFF/WAVE extent")

    offset = 12
    fmt = None
    payload = None
    while offset < riff_end:
        if offset + 8 > riff_end:
            raise ValueError("truncated WAV chunk header")
        chunk = data[offset : offset + 4]
        size = struct.unpack_from("<I", data, offset + 4)[0]
        start = offset + 8
        end = start + size
        padded_end = end + (size & 1)
        if end > riff_end or padded_end > riff_end:
            raise ValueError("truncated WAV chunk")
        if chunk == b"fmt ":
            if fmt is not None:
                raise ValueError("duplicate WAV fmt chunk")
            fmt = data[start:end]
        elif chunk == b"data":
            if payload is not None:
                raise ValueError("duplicate WAV data chunk")
            payload = data[start:end]
        offset = padded_end
    if offset != riff_end:
        raise ValueError("WAV chunk table does not end at declared RIFF extent")
    if fmt is None or len(fmt) < 16 or payload is None:
        raise ValueError("WAV fmt/data chunk missing")
    audio_format, channels, sample_rate, _byte_rate, block_align, bits = struct.unpack_from(
        "<HHIIHH", fmt, 0
    )
    if audio_format != 1 or channels != 1 or sample_rate != 44100 or bits != 16:
        raise ValueError(
            "execution witness render must be mono 44100 Hz 16-bit PCM "
            f"(got format={audio_format} channels={channels} rate={sample_rate} bits={bits})"
        )
    if block_align != 2 or len(payload) % 2:
        raise ValueError("invalid PCM16 data alignment")
    frames = list(struct.unpack("<" + "h" * (len(payload) // 2), payload))
    return {
        "audio_format": audio_format,
        "channels": channels,
        "sample_rate": sample_rate,
        "bits_per_sample": bits,
        "frames": frames,
    }


def onset_frames(frames: list[int]) -> list[int]:
    active = [index for index, value in enumerate(frames) if abs(value) >= 512]
    if not active:
        return []
    groups = [active[0]]
    previous = active[0]
    for index in active[1:]:
        if index - previous > 128:
            groups.append(index)
        previous = index
    return groups


def analyze_wave(data: bytes) -> dict:
    parsed = parse_pcm16_wave(data)
    onsets = onset_frames(parsed["frames"])
    if len(onsets) < 5:
        raise ValueError(
            "execution witness produced fewer than five distinct impulse onsets; "
            "no retrigger-family runtime effect is established"
        )
    beat_frames = parsed["sample_rate"] * 60.0 / EXPECTED_LAYOUT["bpm"]
    counts = [0, 0, 0, 0]
    beat_positions = []
    for frame in onsets:
        beat = frame / beat_frames
        beat_positions.append(beat)
        bucket = min(3, max(0, int(math.floor(beat + 1e-9))))
        counts[bucket] += 1
    # Keep this observer conservative: it proves deterministic command-bearing
    # execution output, but does not turn timing differences into parity.
    if counts[1] < 2 or counts[2] < 2:
        raise ValueError(
            "retrigger and retrigger-continue windows did not each expose "
            "multiple execution impulses"
        )
    return {
        "sample_rate": parsed["sample_rate"],
        "channels": parsed["channels"],
        "bits_per_sample": parsed["bits_per_sample"],
        "frame_count": len(parsed["frames"]),
        "onset_frames": onsets,
        "onset_beats": [round(value, 9) for value in beat_positions],
        "window_onset_counts": {
            "note_delay_beat_0": counts[0],
            "retrigger_beat_1": counts[1],
            "retr_cont_beat_2": counts[2],
            "extended_marker_beat_3": counts[3],
        },
    }


def analyze_wave_observation(data: bytes) -> dict:
    """Describe a valid PCM render without claiming command-bearing execution."""
    parsed = parse_pcm16_wave(data)
    onsets = onset_frames(parsed["frames"])
    beat_frames = parsed["sample_rate"] * 60.0 / EXPECTED_LAYOUT["bpm"]
    counts = [0, 0, 0, 0]
    beat_positions = []
    for frame in onsets:
        beat = frame / beat_frames
        beat_positions.append(beat)
        bucket = min(3, max(0, int(math.floor(beat + 1e-9))))
        counts[bucket] += 1
    return {
        "sample_rate": parsed["sample_rate"],
        "channels": parsed["channels"],
        "bits_per_sample": parsed["bits_per_sample"],
        "frame_count": len(parsed["frames"]),
        "onset_frames": onsets,
        "onset_beats": [round(value, 9) for value in beat_positions],
        "window_onset_counts": {
            "note_delay_beat_0": counts[0],
            "retrigger_beat_1": counts[1],
            "retr_cont_beat_2": counts[2],
            "extended_marker_beat_3": counts[3],
        },
    }


def validate_render_event_binding(attempt: dict) -> None:
    runtime_id = attempt.get("selected_render_dialog_runtime_id")
    preexisting_count = attempt.get("preexisting_render_dialog_count")
    post_dispatch_count = attempt.get("render_dialog_post_dispatch_event_count")
    observed_window_count = attempt.get(
        "render_dialog_post_dispatch_observed_window_event_count"
    )
    unresolved_event_count = attempt.get(
        "render_dialog_unresolved_post_dispatch_event_count"
    )
    boundary_tick = attempt.get("render_dialog_dispatch_boundary_tick")
    selected_handle = attempt.get("selected_render_dialog_native_handle")
    if (
        attempt.get("render_dialog_native_event_hook_armed") is not True
        or attempt.get("render_dialog_event_message_pump_started") is not True
        or attempt.get("render_dialog_dispatch_boundary_set") is not True
        or not isinstance(boundary_tick, int)
        or isinstance(boundary_tick, bool)
        or boundary_tick < 0
        or boundary_tick > 0xFFFFFFFF
        or attempt.get("dialog_discovery")
        != "pumped-win-event-object-show-strictly-after-dispatch-tick"
        or not isinstance(preexisting_count, int)
        or isinstance(preexisting_count, bool)
        or preexisting_count < 0
        or not isinstance(observed_window_count, int)
        or isinstance(observed_window_count, bool)
        or observed_window_count != 1
        or not isinstance(unresolved_event_count, int)
        or isinstance(unresolved_event_count, bool)
        or unresolved_event_count != 0
        or not isinstance(post_dispatch_count, int)
        or isinstance(post_dispatch_count, bool)
        or post_dispatch_count != 1
        or not isinstance(selected_handle, int)
        or isinstance(selected_handle, bool)
        or selected_handle <= 0
        or not isinstance(runtime_id, list)
        or not runtime_id
        or any(
            not isinstance(value, int) or isinstance(value, bool)
            for value in runtime_id
        )
    ):
        raise ValueError(
            "original render attempt lacks bound post-dispatch dialog evidence"
        )


def validate_render_attempt(attempt: object, expected_outcome: str) -> dict:
    if not isinstance(attempt, dict):
        raise ValueError("original render attempt must be an object")
    required_flags = {
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": True,
        "save_invoked": True,
    }
    for key, value in required_flags.items():
        if attempt.get(key) is not value:
            raise ValueError("original render attempt did not reach verified Save Wave: " + key)
    validate_render_event_binding(attempt)
    if attempt.get("outcome") != expected_outcome:
        raise ValueError("unexpected original render attempt outcome")
    return attempt


def validate_observed_output(original_root: Path, value: object) -> dict:
    if (
        not isinstance(value, dict)
        or set(value) != {"path", "size_bytes", "sha256"}
        or not isinstance(value.get("path"), str)
        or not isinstance(value.get("size_bytes"), int)
        or isinstance(value.get("size_bytes"), bool)
        or value["size_bytes"] < 0
        or not isinstance(value.get("sha256"), str)
        or len(value["sha256"]) != 64
    ):
        raise ValueError("invalid observed render output")
    relative = "delayed-retrigger-execution/" + value["path"]
    data = child(original_root, relative).read_bytes()
    if len(data) != value["size_bytes"] or digest(data) != value["sha256"]:
        raise ValueError("observed render output binding mismatch")
    return {
        "path": relative,
        "size_bytes": value["size_bytes"],
        "sha256": value["sha256"],
    }


def validate_completed_render_attempt(
    original_root: Path,
    attempt: object,
    value: object,
    attempt_number: int,
    *,
    allow_post_completion_exit: bool = False,
    allow_process_inspection_failure: bool = False,
) -> bytes:
    validated = validate_render_attempt(attempt, "rendered")
    exit_code = validated.get("process_exit_code")
    if allow_post_completion_exit:
        if (
            validated.get("process_exited") is not True
            or not isinstance(exit_code, int)
            or isinstance(exit_code, bool)
        ):
            raise ValueError(
                "post-completion render exit lacks an integer process exit code"
            )
    else:
        if validated.get("process_exited") is not False:
            raise ValueError("reference exited during a supposedly successful render")
        if exit_code is not None:
            raise ValueError("successful render unexpectedly recorded a process exit code")

    stable_output_polls = validated.get("stable_output_polls")
    diagnostics = validated.get("diagnostics")
    if not isinstance(diagnostics, list) or any(
        not isinstance(value, str) for value in diagnostics
    ):
        raise ValueError("successful render diagnostics are malformed")
    inspection_diagnostics = [
        value
        for value in diagnostics
        if value.startswith(PROCESS_INSPECTION_FAILURE_PREFIX)
    ]
    if allow_process_inspection_failure:
        if len(inspection_diagnostics) != 1:
            raise ValueError(
                "post-render process-inspection failure evidence is missing"
            )
    elif inspection_diagnostics:
        raise ValueError(
            "successful render unexpectedly contains process-inspection failure"
        )
    terminal_diagnostics = [
        value
        for value in diagnostics
        if not value.startswith(PROCESS_INSPECTION_FAILURE_PREFIX)
    ]
    teardown_diagnostic = (
        "render output finalized and Close control was verified, "
        "but dialog teardown did not complete"
    )
    completed_and_closed = (
        validated.get("dialog_closed") is True
        and validated.get("close_control_seen") is True
        and validated.get("close_uia_invoked") is True
        and terminal_diagnostics == []
    )
    completed_with_teardown_failure = (
        validated.get("dialog_closed") is False
        and validated.get("close_control_seen") is True
        and validated.get("close_uia_invoked") is True
        and terminal_diagnostics == [teardown_diagnostic]
    )
    if (
        not isinstance(stable_output_polls, int)
        or isinstance(stable_output_polls, bool)
        or stable_output_polls < 4
        or not (completed_and_closed or completed_with_teardown_failure)
    ):
        raise ValueError(
            "successful render lacks terminal completion evidence"
        )

    output = validated.get("output")
    expected_name = f"original-delayed-retrigger-execution-{attempt_number}.wav"
    if (
        not isinstance(output, dict)
        or set(output) != {"path", "sha256"}
        or output.get("path") != expected_name
        or not isinstance(output.get("sha256"), str)
    ):
        raise ValueError("successful render attempt output binding is invalid")
    data = bound_bytes(original_root, value)
    if (
        value.get("path") != "delayed-retrigger-execution/" + expected_name
        or value.get("sha256") != output["sha256"]
        or digest(data) != output["sha256"]
    ):
        raise ValueError("successful render attempt does not match retained render binding")
    return data


def require_clean_completed_attempt_before_later_attempt(
    attempt: object, context: str
) -> None:
    if (
        not isinstance(attempt, dict)
        or attempt.get("dialog_closed") is not True
        or attempt.get("diagnostics") != []
    ):
        raise ValueError(f"{context} follows a non-clean completed render")


def validate_process_exit_runtime(
    original_root: Path,
    receipt: dict,
    runtime: dict,
    attempts: list[object],
) -> dict:
    if (
        receipt.get("load_result") != "inconclusive"
        or receipt.get("observation")
        != "reference-process-exited-before-harness-termination"
        or runtime.get("deterministic") is not False
    ):
        raise ValueError("original render process-exit observation is inconsistent")

    renders = runtime.get("renders")
    if (
        not isinstance(renders, list)
        or len(renders) not in {0, 1}
        or len(attempts) not in {1, 2}
        or len(attempts) != len(renders) + 1
    ):
        raise ValueError("original render process-exit partial-render shape is inconsistent")

    completed_hashes = []
    for index, value in enumerate(renders, start=1):
        data = validate_completed_render_attempt(
            original_root, attempts[index - 1], value, index
        )
        require_clean_completed_attempt_before_later_attempt(
            attempts[index - 1], "primary process-exit later attempt"
        )
        completed_hashes.append(digest(data))

    attempt_number = len(attempts)
    attempt = validate_render_attempt(attempts[-1], "inconclusive")
    if attempt.get("process_exited") is not True:
        raise ValueError("render process-exit outcome lacks process-exit evidence")
    if attempt.get("output") is not None:
        raise ValueError("failed render attempt unexpectedly records a completed output")
    exit_code = attempt.get("process_exit_code")
    if (
        not isinstance(exit_code, int)
        or isinstance(exit_code, bool)
        or receipt.get("exit_code_before_termination") != exit_code
    ):
        raise ValueError("render process-exit code is missing or inconsistent")
    diagnostics = attempt.get("diagnostics")
    if diagnostics != ["reference exited during offline render"]:
        raise ValueError("render process-exit diagnostic is unexpected")

    expected_output = (
        "delayed-retrigger-execution/"
        f"original-delayed-retrigger-execution-{attempt_number}.wav"
    )
    observed_value = attempt.get("observed_output")
    if observed_value is None:
        if child(original_root, expected_output).exists():
            raise ValueError("failed render attempt created an unbound output file")
        observed_output = None
    else:
        observed_output = validate_observed_output(
            original_root, observed_value
        )
        if observed_output["path"] != expected_output:
            raise ValueError("failed render attempt is bound to the wrong output path")

    return {
        "attempt_number": attempt_number,
        "completed_render_count": len(completed_hashes),
        "completed_render_sha256": completed_hashes,
        "process_exit_code": exit_code,
        "observed_output": observed_output,
    }


OBSERVER_INITIALIZATION_FAILURE_PREFIX = (
    "could not initialize render-dialog observer:"
)
OBSERVER_SEALING_FAILURE_PREFIX = "could not seal render-dialog observer:"
PROCESS_INSPECTION_FAILURE_PREFIX = (
    "could not inspect reference process after render attempt:"
)


def has_diagnostic_prefix(attempt: object, prefix: str) -> bool:
    if not isinstance(attempt, dict):
        return False
    diagnostics = attempt.get("diagnostics")
    return (
        isinstance(diagnostics, list)
        and any(
            isinstance(value, str) and value.startswith(prefix)
            for value in diagnostics
        )
    )


def validate_presave_render_quarantine(attempt: object) -> dict:
    if not isinstance(attempt, dict):
        raise ValueError("pre-Save render quarantine attempt is not an object")
    diagnostics = attempt.get("diagnostics")
    exit_code = attempt.get("process_exit_code")
    process_state_valid = (
        (attempt.get("process_exited") is False and exit_code is None)
        or (
            attempt.get("process_exited") is True
            and isinstance(exit_code, int)
            and not isinstance(exit_code, bool)
        )
    )
    sealing_failure = has_diagnostic_prefix(
        attempt, OBSERVER_SEALING_FAILURE_PREFIX
    )
    if (
        attempt.get("outcome") != "inconclusive"
        or attempt.get("command_verified") is not True
        or attempt.get("command_dispatched") is not True
        or not isinstance(attempt.get("dialog_verified"), bool)
        or not isinstance(attempt.get("controls_configured"), bool)
        or attempt.get("save_invoked") is not False
        or attempt.get("output") is not None
        or attempt.get("observed_output") is not None
        or not process_state_valid
        or not isinstance(diagnostics, list)
        or not diagnostics
        or any(not isinstance(value, str) for value in diagnostics)
        or (
            sealing_failure
            and (
                attempt.get("render_dialog_native_event_hook_armed") is not True
                or attempt.get("render_dialog_event_message_pump_started") is not True
                or attempt.get("render_dialog_dispatch_boundary_set") is not True
            )
        )
    ):
        raise ValueError("pre-Save render quarantine shape is invalid")
    return {
        "inconclusive_reason": (
            "render-observer-sealing-failure"
            if sealing_failure
            else "pre-save-render-automation-failure"
        ),
        "completed_renders": [],
        "completed_render_analyses": [],
        "binding_error": None,
        "diagnostics": diagnostics,
        "process_exit_code": exit_code,
        "observed_output": None,
    }


def validate_predispatch_observer_failure(attempt: object) -> dict:
    if not isinstance(attempt, dict):
        raise ValueError("pre-dispatch observer failure attempt is not an object")
    diagnostics = attempt.get("diagnostics")
    if (
        attempt.get("outcome") != "inconclusive"
        or attempt.get("command_verified") is not True
        or attempt.get("command_dispatched") is not False
        or attempt.get("dialog_verified") is not False
        or attempt.get("controls_configured") is not False
        or attempt.get("save_invoked") is not False
        or attempt.get("output") is not None
        or attempt.get("observed_output") is not None
        or attempt.get("process_exited") is not False
        or attempt.get("process_exit_code") is not None
        or not isinstance(diagnostics, list)
        or len(diagnostics) != 1
        or not isinstance(diagnostics[0], str)
        or not diagnostics[0].startswith(
            OBSERVER_INITIALIZATION_FAILURE_PREFIX
        )
    ):
        raise ValueError(
            "pre-dispatch observer initialization failure shape is invalid"
        )
    return {
        "inconclusive_reason": "render-observer-initialization-failure",
        "completed_renders": [],
        "completed_render_analyses": [],
        "binding_error": None,
        "diagnostics": diagnostics,
        "process_exit_code": None,
        "observed_output": None,
    }


def validate_inconclusive_runtime(
    original_root: Path,
    receipt: dict,
    runtime: dict,
    attempts: list[object],
) -> dict:
    """Retain non-evidentiary primary render uncertainty without promotion."""
    if runtime.get("deterministic") is not False:
        raise ValueError("inconclusive primary runtime cannot be deterministic")
    renders = runtime.get("renders")
    if (
        not isinstance(renders, list)
        or len(renders) > 2
        or len(attempts) not in {1, 2}
        or len(renders) > len(attempts)
    ):
        raise ValueError("inconclusive primary render shape is invalid")

    completed = []
    completed_analyses = []
    completed_diagnostics = []
    post_completion_exit_code = None
    post_completion_observed_output = None
    process_inspection_failure = False
    process_inspection_observed_output = None
    for index, value in enumerate(renders, start=1):
        completed_attempt = attempts[index - 1]
        allow_post_completion_exit = (
            index == len(renders)
            and len(renders) == len(attempts)
            and isinstance(completed_attempt, dict)
            and completed_attempt.get("outcome") == "rendered"
            and completed_attempt.get("process_exited") is True
        )
        allow_process_inspection_failure = (
            index == len(renders)
            and len(renders) == len(attempts)
            and has_diagnostic_prefix(
                completed_attempt, PROCESS_INSPECTION_FAILURE_PREFIX
            )
        )
        data = validate_completed_render_attempt(
            original_root,
            completed_attempt,
            value,
            index,
            allow_post_completion_exit=allow_post_completion_exit,
            allow_process_inspection_failure=allow_process_inspection_failure,
        )
        if index < len(attempts):
            require_clean_completed_attempt_before_later_attempt(
                completed_attempt, "primary later render"
            )
        completed.append(value)
        completed_analyses.append(analyze_wave_observation(data))
        completed_diagnostics.extend(completed_attempt.get("diagnostics", []))
        if allow_process_inspection_failure:
            process_inspection_failure = True
            process_inspection_observed_output = validate_observed_output(
                original_root, completed_attempt.get("observed_output")
            )
            expected_path = (
                "delayed-retrigger-execution/"
                f"original-delayed-retrigger-execution-{index}.wav"
            )
            if process_inspection_observed_output["path"] != expected_path:
                raise ValueError(
                    "post-render process-inspection observed-output path mismatch"
                )
        if allow_post_completion_exit:
            post_completion_exit_code = completed_attempt.get("process_exit_code")
            if receipt.get("exit_code_before_termination") != post_completion_exit_code:
                raise ValueError(
                    "post-completion primary process-exit code is inconsistent"
                )
            post_completion_observed_output = validate_observed_output(
                original_root, completed_attempt.get("observed_output")
            )
            expected_path = (
                "delayed-retrigger-execution/"
                f"original-delayed-retrigger-execution-{index}.wav"
            )
            if post_completion_observed_output["path"] != expected_path:
                raise ValueError(
                    "post-completion primary render observed-output path mismatch"
                )

    if process_inspection_failure:
        return {
            "inconclusive_reason": "post-render-process-inspection-failure",
            "completed_renders": completed,
            "completed_render_analyses": completed_analyses,
            "binding_error": None,
            "diagnostics": completed_diagnostics,
            "process_exit_code": (
                attempts[-1].get("process_exit_code")
                if isinstance(attempts[-1], dict)
                else None
            ),
            "observed_output": process_inspection_observed_output,
        }

    if len(attempts) == len(renders) + 1:
        final_attempt = attempts[-1]
        final_diagnostics = (
            final_attempt.get("diagnostics")
            if isinstance(final_attempt, dict)
            else None
        )
        if (
            isinstance(final_diagnostics, list)
            and len(final_diagnostics) == 1
            and isinstance(final_diagnostics[0], str)
            and final_diagnostics[0].startswith(
                OBSERVER_INITIALIZATION_FAILURE_PREFIX
            )
        ):
            quarantine = validate_predispatch_observer_failure(
                final_attempt
            )
            if completed:
                previous_attempt = attempts[len(renders) - 1]
                if (
                    not isinstance(previous_attempt, dict)
                    or previous_attempt.get("dialog_closed") is not True
                    or previous_attempt.get("diagnostics") != []
                ):
                    raise ValueError(
                        "pre-dispatch second-render failure follows "
                        "a non-clean first render"
                    )
            quarantine["completed_renders"] = completed
            quarantine["completed_render_analyses"] = completed_analyses
            quarantine["diagnostics"] = (
                completed_diagnostics + quarantine["diagnostics"]
            )
            return quarantine

    if post_completion_exit_code is not None:
        return {
            "inconclusive_reason": "process-exit-after-completed-render",
            "completed_renders": completed,
            "completed_render_analyses": completed_analyses,
            "binding_error": None,
            "diagnostics": completed_diagnostics,
            "process_exit_code": post_completion_exit_code,
            "observed_output": post_completion_observed_output,
        }

    # Two completed, byte-different renders are valid nondeterminism evidence.
    if len(renders) == 2:
        if len(attempts) != 2 or renders[0].get("sha256") == renders[1].get("sha256"):
            raise ValueError("inconclusive primary render-pair identity is invalid")
        return {
            "inconclusive_reason": "nondeterministic-completed-render-pair",
            "completed_renders": completed,
            "completed_render_analyses": completed_analyses,
            "binding_error": None,
            "diagnostics": completed_diagnostics,
            "process_exit_code": None,
            "observed_output": None,
        }

    if len(attempts) == len(renders):
        # A completed render may be retained even if no further modal attempt
        # could safely be started.
        return {
            "inconclusive_reason": "incomplete-repeated-render-procedure",
            "completed_renders": completed,
            "completed_render_analyses": completed_analyses,
            "binding_error": None,
            "diagnostics": completed_diagnostics,
            "process_exit_code": None,
            "observed_output": None,
        }

    attempt_number = len(attempts)
    attempt = attempts[-1]
    if not isinstance(attempt, dict):
        raise ValueError("inconclusive primary render attempt is not an object")
    for key in ("command_verified", "command_dispatched"):
        if attempt.get(key) is not True:
            raise ValueError(
                "inconclusive primary render did not verify command dispatch: " + key
            )
    for key in ("dialog_verified", "controls_configured", "save_invoked"):
        if not isinstance(attempt.get(key), bool):
            raise ValueError("inconclusive primary render has invalid boolean: " + key)
    diagnostics = attempt.get("diagnostics")
    process_exited = attempt.get("process_exited")
    process_exit_code = attempt.get("process_exit_code")
    alive = process_exited is False and process_exit_code is None
    exited = (
        process_exited is True
        and isinstance(process_exit_code, int)
        and not isinstance(process_exit_code, bool)
    )
    if (
        attempt.get("outcome") != "inconclusive"
        or attempt.get("output") is not None
        or not (alive or exited)
        or not isinstance(diagnostics, list)
        or not diagnostics
    ):
        raise ValueError("inconclusive primary render attempt shape is invalid")

    try:
        validate_render_event_binding(attempt)
    except ValueError as exc:
        binding_error = str(exc)
    else:
        binding_error = None

    expected_name = f"original-delayed-retrigger-execution-{attempt_number}.wav"
    expected_path = child(
        original_root, "delayed-retrigger-execution/" + expected_name
    )
    observed = attempt.get("observed_output")
    if observed is None:
        if expected_path.exists():
            raise ValueError("inconclusive primary render created unbound output")
        observed_output = None
    else:
        observed_output = validate_observed_output(original_root, observed)
        if observed_output["path"] != "delayed-retrigger-execution/" + expected_name:
            raise ValueError("inconclusive primary render observed-output path mismatch")

    if (
        exited
        and receipt.get("exit_code_before_termination") != process_exit_code
    ):
        raise ValueError("inconclusive primary process-exit code is inconsistent")

    return {
        "inconclusive_reason": (
            "process-exit-before-completed-save"
            if exited
            else (
                "ambiguous-render-dialog-binding"
                if binding_error is not None
                else "post-binding-render-automation-failure"
            )
        ),
        "completed_renders": completed,
        "completed_render_analyses": completed_analyses,
        "binding_error": binding_error,
        "diagnostics": diagnostics,
        "process_exit_code": process_exit_code,
        "observed_output": observed_output,
    }


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    candidate = validate_candidate(candidate_root)
    original_gate = load_module(
        "phase6c-delayed-retrigger-original.py"
    ).delayed_validator()
    generic_load_result = original_gate.validate_pair(
        "delayed-retrigger-execution",
        CONTRACT,
        candidate_root,
        original_root,
    )
    receipt = read_json(original_root / ORIGINAL_RECEIPT)
    if receipt.get("load_result") != generic_load_result:
        raise ValueError("generic original identity gate load-result mismatch")
    checks = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "original-observation",
        "contract": CONTRACT,
        "evidence_role": "original",
        "reference_build": REFERENCE_BUILD,
        "candidate_fixture": FIXTURE,
        "fixture_sha256": candidate["fixture_sha256"],
        "original_psycle_observed": True,
        "parity_status": "UNKNOWN",
    }
    for key, value in checks.items():
        if receipt.get(key) != value:
            raise ValueError("original execution witness identity mismatch: " + key)

    runtime = receipt.get("runtime_execution")
    expected_settings = {
        "sample_rate": 44100,
        "bits_per_sample": 16,
        "channels": "mono-mix",
        "dither": False,
        "range": "entire-song",
    }
    if (
        not isinstance(runtime, dict)
        or runtime.get("schema_version") != 1
        or runtime.get("settings") != expected_settings
    ):
        raise ValueError("original runtime execution receipt identity mismatch")

    pre_render = runtime.get("pre_render_load")
    if (
        not isinstance(pre_render, dict)
        or pre_render.get("schema_version") != 1
        or pre_render.get("clean_accepted_load") is not True
        or pre_render.get("load_warning_dismissed") is not True
        or pre_render.get("process_running_before_render") is not True
        or pre_render.get("matched_marker") != Path(FIXTURE).name
        or not isinstance(pre_render.get("stable_marker_polls"), int)
        or isinstance(pre_render.get("stable_marker_polls"), bool)
        or pre_render["stable_marker_polls"] < 4
    ):
        raise ValueError("original pre-render clean-load evidence is incomplete")

    attempts = runtime.get("attempts")
    if not isinstance(attempts, list) or not attempts:
        raise ValueError("original runtime execution attempt is missing")

    outcome = runtime.get("outcome")
    if outcome == "rendered-twice":
        if receipt.get("load_result") != "accepted":
            raise ValueError("successful render witness requires accepted final load result")
        if runtime.get("deterministic") is not True:
            raise ValueError("successful render witness is not deterministic")
        renders = runtime.get("renders")
        if not isinstance(renders, list) or len(renders) != 2 or len(attempts) != 2:
            raise ValueError("expected exactly two successful original offline renders")
        first = validate_completed_render_attempt(
            original_root, attempts[0], renders[0], 1
        )
        require_clean_completed_attempt_before_later_attempt(
            attempts[0], "primary repeated render"
        )
        second = validate_completed_render_attempt(
            original_root, attempts[1], renders[1], 2
        )
        if first != second or digest(first) != digest(second):
            raise ValueError("repeated original offline renders are not byte-identical")

        first_analysis = analyze_wave(first)
        second_analysis = analyze_wave(second)
        if first_analysis != second_analysis:
            raise ValueError("repeated original offline render traces differ")

        result = {
            "schema_version": 1,
            "phase": "6C",
            "scope": "original-runtime-execution-observation",
            "contract": CONTRACT,
            "evidence_role": "original-runtime",
            "reference_build": REFERENCE_BUILD,
            "fixture": receipt["fixture"],
            "fixture_sha256": receipt["fixture_sha256"],
            "runtime_execution_trace": "deterministic-offline-waveform",
            "runtime_command_execution_observed": True,
            "offline_renderer": "Psycle 1.12.0 Render as Wav File",
            "repeat_count": 2,
            "render_sha256": digest(first),
            "analysis": first_analysis,
            "interpretation_boundary": (
                "multiple impulses are required in both FB and FA witness windows, "
                "establishing command-bearing runtime execution; exact timing parity "
                "against the frozen candidate remains deliberately unclassified"
            ),
            "parity_status": "UNKNOWN",
        }
    elif outcome == "reference-process-exited-during-render":
        exit_evidence = validate_process_exit_runtime(
            original_root, receipt, runtime, attempts
        )
        result = {
            "schema_version": 1,
            "phase": "6C",
            "scope": "original-runtime-execution-observation",
            "contract": CONTRACT,
            "evidence_role": "original-runtime",
            "reference_build": REFERENCE_BUILD,
            "fixture": receipt["fixture"],
            "fixture_sha256": receipt["fixture_sha256"],
            "runtime_execution_trace": "reference-process-exit-during-render",
            "runtime_command_execution_observed": False,
            "offline_renderer": "Psycle 1.12.0 Render as Wav File",
            "render_attempt_verified": True,
            "failed_render_attempt": exit_evidence["attempt_number"],
            "completed_render_count": exit_evidence["completed_render_count"],
            "completed_render_sha256": exit_evidence["completed_render_sha256"],
            "process_exit_code": exit_evidence["process_exit_code"],
            "observed_output": exit_evidence["observed_output"],
            "interpretation_boundary": (
                "the exact fixture reached the clean accepted-load gate and each "
                "completed render attempt is hash-bound; the final source-pinned "
                "Render as Wav attempt reached verified Save Wave dispatch but the "
                "pinned reference process exited before that attempt produced a valid "
                "waveform; this is retained as original-runtime failure evidence and "
                "does not establish delayed/retrigger command execution or parity"
            ),
            "parity_status": "UNKNOWN",
        }
    elif outcome == "inconclusive":
        uncertainty = validate_inconclusive_runtime(
            original_root, receipt, runtime, attempts
        )
        result = {
            "schema_version": 1,
            "phase": "6C",
            "scope": "original-runtime-execution-observation",
            "contract": CONTRACT,
            "evidence_role": "original-runtime",
            "reference_build": REFERENCE_BUILD,
            "fixture": receipt["fixture"],
            "fixture_sha256": receipt["fixture_sha256"],
            "runtime_execution_trace": "inconclusive-offline-render",
            "runtime_command_execution_observed": False,
            "offline_renderer": "Psycle 1.12.0 Render as Wav File",
            "inconclusive_reason": uncertainty["inconclusive_reason"],
            "completed_renders": uncertainty["completed_renders"],
            "completed_render_analyses": uncertainty["completed_render_analyses"],
            "fresh_render_event_binding": (
                "not-dispatched"
                if uncertainty["inconclusive_reason"]
                == "render-observer-initialization-failure"
                else (
                    "accepted"
                    if uncertainty["binding_error"] is None
                    else "rejected"
                )
            ),
            "fresh_render_event_binding_error": uncertainty["binding_error"],
            "process_exit_code": uncertainty["process_exit_code"],
            "observed_output": uncertainty["observed_output"],
            "diagnostics": uncertainty["diagnostics"],
            "interpretation_boundary": (
                "the exact fixture reached the clean accepted-load gate, but the "
                "pinned original did not yield a complete deterministic command-bearing "
                "render observation. Any completed WAVs, process exit, dialog-binding "
                "uncertainty, diagnostics and observed output are retained without "
                "promoting command execution; parity remains UNKNOWN"
            ),
            "parity_status": "UNKNOWN",
        }
    else:
        raise ValueError("unexpected original runtime execution outcome")

    target = original_root / RUNTIME_RECEIPT
    if target.exists():
        raise ValueError("refusing stale runtime execution receipt")
    write_new(target, result)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("candidate", "candidate-check", "original"))
    parser.add_argument("root", type=Path)
    parser.add_argument("other", type=Path, nargs="?")
    args = parser.parse_args()
    if args.mode == "candidate":
        if args.other is not None:
            parser.error("candidate accepts only ROOT")
        value = collect_candidate(args.root)
    elif args.mode == "candidate-check":
        if args.other is not None:
            parser.error("candidate-check accepts only ROOT")
        value = validate_candidate(args.root)
    else:
        if args.other is None:
            parser.error("original requires ORIGINAL_ROOT")
        value = validate_original(args.root, args.other)
    print(json.dumps(value, sort_keys=True))


if __name__ == "__main__":
    main()
