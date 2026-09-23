#!/usr/bin/env python3
"""Collect and compare the Phase 6C same-witness Sampulse runtime renders."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
BASE_ANALYZER = ROOT / "scripts" / "phase6c-delayed-retrigger-render-evidence.py"
ORIGINAL_GATE = ROOT / "scripts" / "phase6c-delayed-retrigger-original.py"

CONTRACT = "sequencer-delayed-retrigger-same-witness-render"
NAME = "delayed-retrigger-sampulse-runtime"
FIXTURE = "delayed-retrigger/phase6c-delayed-retrigger-sampulse-execution.psy"
TITLE = "PSYCLE-LINUX Phase 6C delayed/retrigger Sampulse execution witness"
CANDIDATE_RECEIPT = "candidate-delayed-retrigger-sampulse-runtime.json"
ORIGINAL_RECEIPT = "original-delayed-retrigger-sampulse-runtime.json"
ORIGINAL_ANALYSIS = "original-delayed-retrigger-sampulse-runtime-analysis.json"
COMPARISON = "delayed-retrigger-sampulse-runtime-comparison.json"
REFERENCE_BUILD = "Psycle 1.12.0 x86"
ORIGINAL_RENDER_SETTINGS = {
    "sample_rate": 44100,
    "bits_per_sample": 16,
    "channels": "mono-mix",
    "dither": False,
    "range": "entire-song",
}
CANDIDATE_TARGET_BEATS = 4.25
CANDIDATE_BEAT_FRAMES = 44100.0 * 60.0 / 137.0
CANDIDATE_TARGET_FRAMES = math.ceil(
    CANDIDATE_TARGET_BEATS * CANDIDATE_BEAT_FRAMES
)

CANDIDATE_RENDER_PROCEDURE = {
    "engine": "frozen SourceForge SVN r12005 C++ candidate",
    "sample_rate": 44100,
    "bits_per_sample": 16,
    "channels": "mono-mix",
    "dither": False,
    "fixed_frame_render": True,
    "target_beats": CANDIDATE_TARGET_BEATS,
    "threads": 1,
    "repeat_count": 2,
    "sequencer_single_work": True,
    "player_work_direct": False,
    "harness_owned_master_stereo_buffer": True,
    "master_buffer_float_count": "2 * target_frames",
    "loader_preallocation": (
        "project-owned harness preallocates empty XM instrument/sample slot 0 "
        "because retained r12005 LoadEINSv1 loads into existing vectors; "
        "post-load validation requires the EINS bytes to overwrite both slots"
    ),
}


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ValueError(f"could not load module: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


base = load_module(BASE_ANALYZER, "phase6c_render_base")
original_gate_module = load_module(ORIGINAL_GATE, "phase6c_original_gate")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def write_new(path: Path, value: dict) -> None:
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def child(root: Path, relative: str) -> Path:
    if not isinstance(relative, str):
        raise ValueError("unsafe artifact path")
    pure = PurePosixPath(relative)
    if pure.is_absolute() or ".." in pure.parts or "\\" in relative:
        raise ValueError("unsafe artifact path")
    result = (root / relative).resolve()
    result.relative_to(root.resolve())
    return result


def render_binding(root: Path, relative: str) -> tuple[dict, bytes]:
    path = child(root, relative)
    data = path.read_bytes()
    return {"path": relative, "sha256": digest(data)}, data


def artifact_binding(root: Path, relative: str) -> dict:
    binding, _ = render_binding(root, relative)
    return binding


def validate_artifact_binding(root: Path, value: object, expected_path: str) -> dict:
    if (
        not isinstance(value, dict)
        or set(value) != {"path", "sha256"}
        or value.get("path") != expected_path
        or not isinstance(value.get("sha256"), str)
        or len(value["sha256"]) != 64
    ):
        raise ValueError("invalid retained artifact binding: " + expected_path)
    actual = artifact_binding(root, expected_path)
    if actual != value:
        raise ValueError("retained artifact binding mismatch: " + expected_path)
    return actual


def validate_pair_of_waves(
    root: Path, prefix: str, role: str
) -> tuple[list[dict], bytes, dict]:
    bindings: list[dict] = []
    waves: list[bytes] = []
    analyses: list[dict] = []
    for index in (1, 2):
        relative = f"{NAME}/{prefix}-{index}.wav"
        binding, data = render_binding(root, relative)
        analysis = base.analyze_wave(data)
        bindings.append(binding)
        waves.append(data)
        analyses.append(analysis)
    if waves[0] != waves[1] or bindings[0]["sha256"] != bindings[1]["sha256"]:
        raise ValueError(f"{role} repeated renders are not byte-identical")
    if analyses[0] != analyses[1]:
        raise ValueError(f"{role} repeated onset analyses differ")
    return bindings, waves[0], analyses[0]


def validate_candidate_render_log(root: Path, index: int) -> dict:
    relative = f"delayed-retrigger/sampulse-candidate-render-{index}.log"
    path = child(root, relative)
    lines = [
        line.strip()
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    summaries: list[dict] = []
    for line in lines:
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(value, dict) and value.get("schema_version") == 1:
            summaries.append(value)
    if len(summaries) != 1:
        raise ValueError(
            "same-witness candidate render log lacks one renderer JSON summary"
        )

    summary = summaries[0]
    final_play_beat = summary.get("final_play_beat")
    if (
        summary.get("fixed_frame_render") is not True
        or summary.get("sample_rate") != 44100
        or summary.get("channels") != 1
        or summary.get("bits_per_sample") != 16
        or summary.get("target_beats") != CANDIDATE_TARGET_BEATS
        or summary.get("target_frames") != CANDIDATE_TARGET_FRAMES
        or summary.get("threads") != 1
        or summary.get("sequencer_work_calls") != 1
        or summary.get("player_work_direct") is not False
        or summary.get("master_buffer_float_count")
        != 2 * CANDIDATE_TARGET_FRAMES
        or not isinstance(final_play_beat, (int, float))
        or isinstance(final_play_beat, bool)
        or not math.isfinite(float(final_play_beat))
        or abs(float(final_play_beat) - CANDIDATE_TARGET_BEATS) > 0.001
    ):
        raise ValueError(
            "same-witness candidate render log does not substantiate "
            "the fixed-frame single-thread procedure"
        )
    return summary


def validate_candidate_render_logs(root: Path) -> list[dict]:
    observations = [
        validate_candidate_render_log(root, index)
        for index in (1, 2)
    ]
    if observations[0] != observations[1]:
        raise ValueError(
            "same-witness candidate renderer procedure observations differ"
        )
    return observations


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    fixture = child(root, FIXTURE)
    raw = fixture.read_bytes()
    if not raw.startswith(b"PSY3SONG"):
        raise ValueError("same-witness Sampulse fixture is not PSY3")

    renders, wave, analysis = validate_pair_of_waves(
        root, "candidate-delayed-retrigger-sampulse-runtime", "candidate"
    )
    render_observations = validate_candidate_render_logs(root)
    receipt = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-runtime-execution-observation",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "fixture": FIXTURE,
        "fixture_sha256": digest(raw),
        "song_title": TITLE,
        "machine_substrate": "XMSampler/Sampulse",
        "command_layout": base.EXPECTED_LAYOUT["commands"],
        "render_procedure": CANDIDATE_RENDER_PROCEDURE,
        "render_observations": render_observations,
        "renderer_provenance": {
            "binary": artifact_binding(
                root,
                f"{NAME}/phase6c-delayed-retrigger-sampulse-render",
            ),
            "source": artifact_binding(root, f"{NAME}/render-probe.cpp"),
            "project": artifact_binding(root, f"{NAME}/render-probe.pro"),
            "fixture_generator_source": artifact_binding(
                root, f"{NAME}/fixture-generator.c"
            ),
            "eins_converter_source": artifact_binding(root, f"{NAME}/eins-compat.py"),
            "eins_converter_log": artifact_binding(
                root, "delayed-retrigger/sampulse-eins-compat.log"
            ),
            "render_logs": [
                artifact_binding(
                    root, "delayed-retrigger/sampulse-candidate-render-1.log"
                ),
                artifact_binding(
                    root, "delayed-retrigger/sampulse-candidate-render-2.log"
                ),
            ],
        },
        "renders": renders,
        "render_sha256": digest(wave),
        "analysis": analysis,
        "runtime_command_execution_observed": True,
        "timing_interpretation": "deferred",
        "parity_status": "UNKNOWN",
    }
    write_new(root / CANDIDATE_RECEIPT, receipt)
    return receipt


def validate_candidate(root: Path) -> dict:
    root = root.resolve()
    receipt = read_json(root / CANDIDATE_RECEIPT)
    fixture = child(root, FIXTURE)
    if (
        receipt.get("schema_version") != 1
        or receipt.get("phase") != "6C"
        or receipt.get("contract") != CONTRACT
        or receipt.get("evidence_role") != "candidate"
        or receipt.get("fixture") != FIXTURE
        or receipt.get("fixture_sha256") != digest(fixture.read_bytes())
        or receipt.get("song_title") != TITLE
        or receipt.get("machine_substrate") != "XMSampler/Sampulse"
        or receipt.get("command_layout") != base.EXPECTED_LAYOUT["commands"]
        or receipt.get("render_procedure") != CANDIDATE_RENDER_PROCEDURE
        or receipt.get("runtime_command_execution_observed") is not True
        or receipt.get("timing_interpretation") != "deferred"
        or receipt.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("same-witness candidate receipt identity mismatch")
    provenance = receipt.get("renderer_provenance")
    if not isinstance(provenance, dict):
        raise ValueError("same-witness candidate renderer provenance is missing")
    expected_single = {
        "binary": f"{NAME}/phase6c-delayed-retrigger-sampulse-render",
        "source": f"{NAME}/render-probe.cpp",
        "project": f"{NAME}/render-probe.pro",
        "fixture_generator_source": f"{NAME}/fixture-generator.c",
        "eins_converter_source": f"{NAME}/eins-compat.py",
        "eins_converter_log": "delayed-retrigger/sampulse-eins-compat.log",
    }
    for key, expected_path in expected_single.items():
        validate_artifact_binding(root, provenance.get(key), expected_path)
    logs = provenance.get("render_logs")
    if not isinstance(logs, list) or len(logs) != 2:
        raise ValueError("same-witness candidate render-log provenance is invalid")
    for index, value in enumerate(logs, start=1):
        validate_artifact_binding(
            root,
            value,
            f"delayed-retrigger/sampulse-candidate-render-{index}.log",
        )
    render_observations = validate_candidate_render_logs(root)
    if receipt.get("render_observations") != render_observations:
        raise ValueError(
            "same-witness candidate renderer observation binding mismatch"
        )

    renders, wave, analysis = validate_pair_of_waves(
        root, "candidate-delayed-retrigger-sampulse-runtime", "candidate"
    )
    if (
        receipt.get("renders") != renders
        or receipt.get("render_sha256") != digest(wave)
        or receipt.get("analysis") != analysis
    ):
        raise ValueError("same-witness candidate render binding mismatch")
    return receipt


def validate_original_event_binding(attempt: dict) -> None:
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
            "same-witness original render lacks bound post-dispatch dialog evidence"
        )

AMBIGUITY_DIAGNOSTICS = (
    "unresolved post-dispatch Psycle window-show event observed",
    "multiple post-dispatch Psycle window-show events observed",
    "multiple post-dispatch Render as Wav File windows observed",
)


def validate_original_ambiguous_event_binding(attempt: dict) -> None:
    observed_window_count = attempt.get(
        "render_dialog_post_dispatch_observed_window_event_count"
    )
    unresolved_event_count = attempt.get(
        "render_dialog_unresolved_post_dispatch_event_count"
    )
    post_dispatch_count = attempt.get("render_dialog_post_dispatch_event_count")
    boundary_tick = attempt.get("render_dialog_dispatch_boundary_tick")
    preexisting_count = attempt.get("preexisting_render_dialog_count")
    diagnostics = attempt.get("diagnostics")
    if (
        attempt.get("render_dialog_native_event_hook_armed") is not True
        or attempt.get("render_dialog_event_message_pump_started") is not True
        or attempt.get("render_dialog_dispatch_boundary_set") is not True
        or not isinstance(boundary_tick, int)
        or isinstance(boundary_tick, bool)
        or boundary_tick < 0
        or boundary_tick > 0xFFFFFFFF
        or not isinstance(preexisting_count, int)
        or isinstance(preexisting_count, bool)
        or preexisting_count < 0
        or not isinstance(observed_window_count, int)
        or isinstance(observed_window_count, bool)
        or observed_window_count < 0
        or not isinstance(unresolved_event_count, int)
        or isinstance(unresolved_event_count, bool)
        or unresolved_event_count < 0
        or not isinstance(post_dispatch_count, int)
        or isinstance(post_dispatch_count, bool)
        or post_dispatch_count < 0
        or not isinstance(diagnostics, list)
        or not diagnostics
        or any(not isinstance(value, str) for value in diagnostics)
    ):
        raise ValueError(
            "same-witness inconclusive render lacks ambiguity evidence"
        )

    ambiguous = (
        observed_window_count > 1
        or unresolved_event_count > 0
        or post_dispatch_count > 1
    )
    diagnostic_matches = any(
        marker in diagnostic
        for diagnostic in diagnostics
        for marker in AMBIGUITY_DIAGNOSTICS
    )
    if not ambiguous or not diagnostic_matches:
        raise ValueError(
            "same-witness inconclusive render does not prove "
            "ambiguous post-dispatch event binding"
        )


def validate_original_inconclusive_runtime(
    original_root: Path, runtime: object
) -> dict:
    if not isinstance(runtime, dict):
        raise ValueError("same-witness original runtime receipt is missing")
    attempts = runtime.get("attempts")
    pre = runtime.get("pre_render_load")
    if (
        runtime.get("schema_version") != 1
        or runtime.get("outcome") != "inconclusive"
        or runtime.get("deterministic") is not False
        or runtime.get("settings") != ORIGINAL_RENDER_SETTINGS
        or runtime.get("renders") != []
        or not isinstance(attempts, list)
        or len(attempts) != 1
        or not isinstance(pre, dict)
        or pre.get("schema_version") != 1
        or pre.get("clean_accepted_load") is not True
        or pre.get("load_warning_dismissed") is not True
        or pre.get("process_running_before_render") is not True
        or pre.get("matched_marker") != Path(FIXTURE).name
        or not isinstance(pre.get("stable_marker_polls"), int)
        or isinstance(pre.get("stable_marker_polls"), bool)
        or pre["stable_marker_polls"] < 4
    ):
        raise ValueError("same-witness inconclusive original runtime mismatch")

    attempt = attempts[0]
    if not isinstance(attempt, dict):
        raise ValueError("same-witness inconclusive render attempt is not an object")
    for key in ("command_verified", "command_dispatched"):
        if attempt.get(key) is not True:
            raise ValueError(
                f"same-witness inconclusive render did not verify {key}"
            )
    for key in ("dialog_verified", "controls_configured", "save_invoked"):
        if not isinstance(attempt.get(key), bool):
            raise ValueError(
                f"same-witness inconclusive render has invalid {key}"
            )
    diagnostics = attempt.get("diagnostics")
    if (
        attempt.get("outcome") != "inconclusive"
        or attempt.get("process_exited") is not False
        or attempt.get("process_exit_code") is not None
        or attempt.get("output") is not None
        or not isinstance(diagnostics, list)
        or not diagnostics
    ):
        raise ValueError("same-witness inconclusive render shape mismatch")

    # Ambiguity can be discovered immediately after dialog discovery, before
    # later UI/render flags have had any opportunity to become true.
    validate_original_ambiguous_event_binding(attempt)

    try:
        validate_original_event_binding(attempt)
    except ValueError as exc:
        binding_error = str(exc)
    else:
        raise ValueError(
            "same-witness inconclusive render has valid post-dispatch dialog binding"
        )

    expected_name = "original-delayed-retrigger-sampulse-runtime-1.wav"
    expected_relative = f"{NAME}/{expected_name}"
    expected_path = child(original_root, expected_relative)
    observed = attempt.get("observed_output")
    if observed is None:
        if expected_path.exists():
            raise ValueError(
                "same-witness inconclusive render created an unbound output file"
            )
        observed_binding = None
    else:
        if (
            not isinstance(observed, dict)
            or set(observed) != {"path", "size_bytes", "sha256"}
            or observed.get("path") != expected_name
            or not isinstance(observed.get("size_bytes"), int)
            or isinstance(observed.get("size_bytes"), bool)
            or observed["size_bytes"] < 0
            or not isinstance(observed.get("sha256"), str)
            or len(observed["sha256"]) != 64
        ):
            raise ValueError(
                "same-witness inconclusive observed output binding is invalid"
            )
        data = expected_path.read_bytes()
        if (
            len(data) != observed["size_bytes"]
            or digest(data) != observed["sha256"]
        ):
            raise ValueError(
                "same-witness inconclusive observed output binding mismatch"
            )
        observed_binding = {
            "path": expected_relative,
            "size_bytes": observed["size_bytes"],
            "sha256": observed["sha256"],
        }

    return {
        "binding_error": binding_error,
        "diagnostics": diagnostics,
        "observed_output": observed_binding,
    }


def validate_original_runtime_procedure(runtime: object) -> list[dict]:
    if not isinstance(runtime, dict):
        raise ValueError("same-witness original runtime receipt is missing")
    attempts = runtime.get("attempts")
    pre = runtime.get("pre_render_load")
    if (
        runtime.get("schema_version") != 1
        or runtime.get("outcome") != "rendered-twice"
        or runtime.get("deterministic") is not True
        or runtime.get("settings") != ORIGINAL_RENDER_SETTINGS
        or not isinstance(attempts, list)
        or len(attempts) != 2
        or not isinstance(pre, dict)
        or pre.get("schema_version") != 1
        or pre.get("clean_accepted_load") is not True
        or pre.get("load_warning_dismissed") is not True
        or pre.get("process_running_before_render") is not True
        or pre.get("matched_marker") != Path(FIXTURE).name
        or not isinstance(pre.get("stable_marker_polls"), int)
        or isinstance(pre.get("stable_marker_polls"), bool)
        or pre["stable_marker_polls"] < 4
    ):
        raise ValueError("same-witness original render procedure mismatch")
    return attempts


def validate_original_attempt(
    original_root: Path, attempt: object, index: int
) -> tuple[dict, bytes]:
    if not isinstance(attempt, dict):
        raise ValueError("same-witness original render attempt is not an object")
    for key in (
        "command_verified",
        "command_dispatched",
        "dialog_verified",
        "controls_configured",
        "save_invoked",
    ):
        if attempt.get(key) is not True:
            raise ValueError(f"same-witness original render did not verify {key}")

    validate_original_event_binding(attempt)

    diagnostics = attempt.get("diagnostics")
    teardown_diagnostic = (
        "render output finalized and Close control was verified, "
        "but dialog teardown did not complete"
    )
    completed_and_closed = (
        attempt.get("dialog_closed") is True and diagnostics == []
    )
    completed_with_teardown_failure = (
        attempt.get("dialog_closed") is False
        and attempt.get("close_control_seen") is True
        and attempt.get("close_uia_invoked") is True
        and diagnostics == [teardown_diagnostic]
    )
    if (
        attempt.get("outcome") != "rendered"
        or attempt.get("process_exited") is not False
        or attempt.get("process_exit_code") is not None
        or not isinstance(attempt.get("stable_output_polls"), int)
        or isinstance(attempt.get("stable_output_polls"), bool)
        or attempt["stable_output_polls"] < 4
        or not (completed_and_closed or completed_with_teardown_failure)
    ):
        raise ValueError("same-witness original render did not complete cleanly")
    expected_name = f"original-delayed-retrigger-sampulse-runtime-{index}.wav"
    output = attempt.get("output")
    if (
        not isinstance(output, dict)
        or output.get("path") != expected_name
        or not isinstance(output.get("sha256"), str)
    ):
        raise ValueError("same-witness original output binding is invalid")
    relative = f"{NAME}/{expected_name}"
    data = child(original_root, relative).read_bytes()
    if digest(data) != output["sha256"]:
        raise ValueError("same-witness original output hash mismatch")
    return {"path": relative, "sha256": output["sha256"]}, data


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    candidate = validate_candidate(candidate_root)

    gate = original_gate_module.delayed_validator()
    generic_result = gate.validate_pair(
        NAME, CONTRACT, candidate_root, original_root
    )
    receipt = read_json(original_root / ORIGINAL_RECEIPT)
    if (
        generic_result != "accepted"
        or receipt.get("load_result") != "accepted"
        or receipt.get("schema_version") != 1
        or receipt.get("phase") != "6C"
        or receipt.get("contract") != CONTRACT
        or receipt.get("evidence_role") != "original"
        or receipt.get("reference_build") != REFERENCE_BUILD
        or receipt.get("fixture_sha256") != candidate["fixture_sha256"]
        or receipt.get("original_psycle_observed") is not True
        or receipt.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("same-witness original receipt identity mismatch")

    runtime = receipt.get("runtime_execution")
    if isinstance(runtime, dict) and runtime.get("outcome") == "inconclusive":
        quarantine = validate_original_inconclusive_runtime(
            original_root, runtime
        )
        original_analysis = {
            "schema_version": 1,
            "phase": "6C",
            "contract": CONTRACT,
            "evidence_role": "original-runtime",
            "reference_build": REFERENCE_BUILD,
            "fixture": candidate["fixture"],
            "fixture_sha256": candidate["fixture_sha256"],
            "machine_substrate": "XMSampler/Sampulse",
            "outcome": "inconclusive",
            "renders": [],
            "render_sha256": None,
            "analysis": None,
            "runtime_command_execution_observed": False,
            "fresh_render_event_binding": "rejected",
            "fresh_render_event_binding_error": quarantine["binding_error"],
            "observed_output": quarantine["observed_output"],
            "diagnostics": quarantine["diagnostics"],
            "timing_interpretation": "deferred",
            "parity_status": "UNKNOWN",
        }
        write_new(original_root / ORIGINAL_ANALYSIS, original_analysis)

        comparison = {
            "schema_version": 1,
            "phase": "6C",
            "contract": CONTRACT,
            "fixture_sha256": candidate["fixture_sha256"],
            "same_fixture_bytes": True,
            "same_onset_analyzer": (
                "phase6c-delayed-retrigger-render-evidence.py::analyze_wave"
            ),
            "candidate": {
                "render_sha256": candidate["render_sha256"],
                "analysis": candidate["analysis"],
                "runtime_command_execution_observed": True,
            },
            "original": {
                "reference_build": REFERENCE_BUILD,
                "outcome": "inconclusive",
                "render_sha256": None,
                "analysis": None,
                "runtime_command_execution_observed": False,
                "fresh_render_event_binding": "rejected",
                "fresh_render_event_binding_error": quarantine["binding_error"],
                "observed_output": quarantine["observed_output"],
            },
            "command_bearing_runtime_pair_observed": False,
            "exact_onset_timing_interpretation": "deferred",
            "classification_allowed": False,
            "parity_status": "UNKNOWN",
            "interpretation_boundary": (
                "The candidate rendered the exact four-beat Sampulse/XMSampler "
                "witness, but the fresh pinned-original render attempt was "
                "quarantined because its post-dispatch dialog evidence was "
                "ambiguous. No original command-bearing runtime execution or "
                "cross-side runtime pair is claimed from this attempt, and "
                "delayed/retrigger parity remains unclassified."
            ),
        }
        write_new(original_root / COMPARISON, comparison)
        return comparison

    attempts = validate_original_runtime_procedure(runtime)

    first_binding, first = validate_original_attempt(original_root, attempts[0], 1)
    second_binding, second = validate_original_attempt(original_root, attempts[1], 2)
    if first != second or first_binding["sha256"] != second_binding["sha256"]:
        raise ValueError("same-witness original renders are not byte-identical")
    expected_runtime_renders = [first_binding, second_binding]
    if runtime.get("renders") != expected_runtime_renders:
        raise ValueError("same-witness original runtime render bindings mismatch")
    analysis = base.analyze_wave(first)
    if analysis != base.analyze_wave(second):
        raise ValueError("same-witness original onset analyses differ")

    original_analysis = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "evidence_role": "original-runtime",
        "reference_build": REFERENCE_BUILD,
        "fixture": candidate["fixture"],
        "fixture_sha256": candidate["fixture_sha256"],
        "machine_substrate": "XMSampler/Sampulse",
        "renders": [first_binding, second_binding],
        "render_sha256": digest(first),
        "analysis": analysis,
        "runtime_command_execution_observed": True,
        "timing_interpretation": "deferred",
        "parity_status": "UNKNOWN",
    }
    write_new(original_root / ORIGINAL_ANALYSIS, original_analysis)

    comparison = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "fixture_sha256": candidate["fixture_sha256"],
        "same_fixture_bytes": True,
        "same_onset_analyzer": "phase6c-delayed-retrigger-render-evidence.py::analyze_wave",
        "candidate": {
            "render_sha256": candidate["render_sha256"],
            "analysis": candidate["analysis"],
            "runtime_command_execution_observed": True,
        },
        "original": {
            "reference_build": REFERENCE_BUILD,
            "render_sha256": original_analysis["render_sha256"],
            "analysis": original_analysis["analysis"],
            "runtime_command_execution_observed": True,
        },
        "command_bearing_runtime_pair_observed": True,
        "exact_onset_timing_interpretation": "deferred",
        "classification_allowed": False,
        "parity_status": "UNKNOWN",
        "interpretation_boundary": (
            "Both sides rendered the exact same four-beat Sampulse/XMSampler witness "
            "with the same command geometry and the same onset analyzer. This completes "
            "the command-bearing runtime evidence pair. Exact onset timing is retained "
            "for the next evidence rung and is not classified in this receipt."
        ),
    }
    write_new(original_root / COMPARISON, comparison)
    return comparison


def main() -> int:
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
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
