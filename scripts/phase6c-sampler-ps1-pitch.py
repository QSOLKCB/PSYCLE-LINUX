#!/usr/bin/env python3
"""Validate and compare the Phase 6C Sampler PS1 runtime pitch witness."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import wave

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = "sampler-ps1-pitch-runtime"
COMPARISON_CONTRACT = "sampler-ps1-pitch-runtime-comparison"
CANDIDATE_SNAPSHOT = "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
REFERENCE_BUILD = "Psycle 1.12.0 x86"
REFERENCE_FILE = "PsycleInstallerx86-1.12.0.exe"
REFERENCE_INSTALLER_SHA256 = (
    "f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769"
)
REFERENCE_INSTALLER_SIZE_BYTES = 9322919
REFERENCE_EXECUTABLE_SHA256 = (
    "fdb130d2465d5b4a4acfbfe0bfb2368926380fe07a383c4774f0951591b6d6b6"
)
RENDERER_PROVENANCE = "renderer-provenance.json"
ORIGINAL_FIXTURE = "fixture/phase6c-sampler-ps1-pitch-original.psy"
CANDIDATE_FIXTURE = "fixture/phase6c-sampler-ps1-pitch-candidate.psy"
CANDIDATE_RECEIPT = "candidate-sampler-ps1-pitch.json"
ORIGINAL_RECEIPT = "original-sampler-ps1-pitch.json"
AUTHORED_SAMPLE_RATE = 22050
AUTHORED_SAMPLE_FRAMES = 11025
OUTPUT_RATE = 44100
NOTE = 60
ACTIVE_THRESHOLD = 64

COMPAT_PATH = ROOT / "scripts/phase6c-sampler-ps1-pitch-compat.py"
_spec = importlib.util.spec_from_file_location("phase6c_sampler_ps1_pitch_compat", COMPAT_PATH)
assert _spec is not None and _spec.loader is not None
compat = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(compat)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def exact_equal(actual: object, expected: object) -> bool:
    if type(actual) is not type(expected):
        return False
    if isinstance(expected, dict):
        return actual.keys() == expected.keys() and all(
            exact_equal(actual[key], value) for key, value in expected.items()
        )
    if isinstance(expected, list):
        return len(actual) == len(expected) and all(
            exact_equal(left, right) for left, right in zip(actual, expected)
        )
    return actual == expected


def expected_renderer_provenance() -> dict:
    sources = {
        "renderer_source_sha256": ROOT / "tests/phase6c_sampler_ps1_pitch_render.cpp",
        "renderer_project_sha256": ROOT / "tests/phase6c_sampler_ps1_pitch_render.pro",
        "engine_sequencer_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sequencer.cpp"
        ),
        "engine_psy3_loader_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/psy3filter.cpp"
        ),
        "engine_sampler_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.cpp"
        ),
    }
    return {
        "schema_version": 1,
        **{name: sha256(path) for name, path in sources.items()},
    }


def validate_renderer_provenance(root: Path) -> dict:
    path = child(root, RENDERER_PROVENANCE)
    if not path.is_file():
        raise ValueError("candidate compiled-renderer provenance is missing")
    attestation = read_json(path)
    expected = expected_renderer_provenance()
    if not exact_equal(attestation, expected):
        raise ValueError(
            "candidate compiled-renderer provenance does not match checked-out "
            "renderer/engine sources"
        )
    return {
        "path": RENDERER_PROVENANCE,
        "sha256": sha256(path),
        "attestation": attestation,
    }


def child(root: Path, relative: str) -> Path:
    path = (root / relative).resolve()
    root = root.resolve()
    try:
        path.relative_to(root)
    except ValueError as exc:
        raise ValueError("receipt path escapes evidence root") from exc
    return path


def analyze_wave(path: Path) -> dict:
    with wave.open(str(path), "rb") as handle:
        channels = handle.getnchannels()
        width = handle.getsampwidth()
        rate = handle.getframerate()
        frames = handle.getnframes()
        payload = handle.readframes(frames)
    if channels != 1 or width != 2 or rate != OUTPUT_RATE:
        raise ValueError("pitch witness WAV format changed")
    if len(payload) != frames * 2:
        raise ValueError("pitch witness WAV payload is truncated")
    samples = struct.unpack("<" + "h" * frames, payload)
    active = [i for i, value in enumerate(samples) if abs(value) > ACTIVE_THRESHOLD]
    if not active:
        raise ValueError("pitch witness WAV contains no active audio")
    first = active[0]
    last = active[-1]
    return {
        "sample_rate": rate,
        "channels": channels,
        "bits_per_sample": width * 8,
        "frame_count": frames,
        "active_threshold_abs_pcm16": ACTIVE_THRESHOLD,
        "first_active_frame": first,
        "last_active_frame": last,
        "active_span_frames": last - first + 1,
        "active_span_seconds": (last - first + 1) / float(rate),
    }


def require_fixture_pair(root: Path) -> tuple[Path, Path, dict]:
    original_fixture = child(root, ORIGINAL_FIXTURE)
    candidate_fixture = child(root, CANDIDATE_FIXTURE)
    if not original_fixture.is_file() or not candidate_fixture.is_file():
        raise ValueError("Sampler pitch fixture pair is missing")
    info = compat.inspect_pair(
        original_fixture.read_bytes(),
        candidate_fixture.read_bytes(),
    )
    if (
        info["authored_sample_rate"] != AUTHORED_SAMPLE_RATE
        or info["authored_sample_frames"] != AUTHORED_SAMPLE_FRAMES
        or info["candidate_modern_sample_chunk_removed"] is not True
    ):
        raise ValueError("Sampler pitch fixture-pair semantic identity changed")
    return original_fixture, candidate_fixture, info

def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    original_fixture, candidate_fixture, fixture_info = require_fixture_pair(root)
    renders = []
    for index in (1, 2):
        path = child(root, f"render/candidate-sampler-ps1-pitch-{index}.wav")
        if not path.is_file():
            raise ValueError("candidate pitch render is missing")
        renders.append(
            {
                "path": str(path.relative_to(root)),
                "sha256": sha256(path),
                "analysis": analyze_wave(path),
            }
        )
    if renders[0]["sha256"] != renders[1]["sha256"]:
        raise ValueError("candidate repeated pitch renders are not byte-identical")
    if renders[0]["analysis"] != renders[1]["analysis"]:
        raise ValueError("candidate repeated pitch analyses differ")
    span = renders[0]["analysis"]["active_span_frames"]
    if not (10000 <= span <= 12050):
        raise ValueError(
            f"candidate active duration is outside fixed-44.1-kHz witness range: {span}"
        )

    renderer_provenance = validate_renderer_provenance(root)

    value = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-runtime-pitch-observation",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "snapshot": CANDIDATE_SNAPSHOT,
        "renderer_provenance": renderer_provenance,
        "fixture": ORIGINAL_FIXTURE,
        "fixture_sha256": sha256(original_fixture),
        "candidate_fixture": CANDIDATE_FIXTURE,
        "candidate_fixture_sha256": sha256(candidate_fixture),
        "fixture_semantics": fixture_info,
        "authored_sample": {
            "sample_rate": AUTHORED_SAMPLE_RATE,
            "frames": AUTHORED_SAMPLE_FRAMES,
            "note": NOTE,
        },
        "render_settings": {
            "sample_rate": OUTPUT_RATE,
            "bits_per_sample": 16,
            "channels": "mono-mix",
            "dither": False,
        },
        "repeat_count": 2,
        "deterministic": True,
        "renders": renders,
        "runtime_pitch_observation": "fixed-44100-basis-compatible-duration",
        "parity_status": "UNKNOWN",
        "interpretation_boundary": (
            "This candidate runtime observation measures one non-44.1-kHz PS1 "
            "note only. It does not classify the broad Sampler PS1 contract."
        ),
    }
    validate_candidate(root, value)
    return value


def validate_candidate(root: Path, value: dict | None = None) -> dict:
    root = root.resolve()
    if value is None:
        value = read_json(root / CANDIDATE_RECEIPT)
    checks = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-runtime-pitch-observation",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "snapshot": CANDIDATE_SNAPSHOT,
        "fixture": ORIGINAL_FIXTURE,
        "candidate_fixture": CANDIDATE_FIXTURE,
        "parity_status": "UNKNOWN",
        "deterministic": True,
        "repeat_count": 2,
        "runtime_pitch_observation": "fixed-44100-basis-compatible-duration",
    }
    for key, expected in checks.items():
        if value.get(key) != expected or type(value.get(key)) is not type(expected):
            raise ValueError(f"candidate pitch receipt identity changed: {key}")
    provenance = validate_renderer_provenance(root)
    if not exact_equal(value.get("renderer_provenance"), provenance):
        raise ValueError("candidate renderer provenance binding changed")
    original_fixture, candidate_fixture, info = require_fixture_pair(root)
    if value.get("fixture_sha256") != sha256(original_fixture):
        raise ValueError("original-generation pitch fixture digest mismatch")
    if value.get("candidate_fixture_sha256") != sha256(candidate_fixture):
        raise ValueError("candidate-generation pitch fixture digest mismatch")
    if not exact_equal(value.get("fixture_semantics"), info):
        raise ValueError("candidate pitch fixture-pair semantics changed")
    if not exact_equal(
        value.get("authored_sample"),
        {
            "sample_rate": AUTHORED_SAMPLE_RATE,
            "frames": AUTHORED_SAMPLE_FRAMES,
            "note": NOTE,
        },
    ):
        raise ValueError("candidate authored-sample identity changed")
    if not exact_equal(
        value.get("render_settings"),
        {
            "sample_rate": OUTPUT_RATE,
            "bits_per_sample": 16,
            "channels": "mono-mix",
            "dither": False,
        },
    ):
        raise ValueError("candidate render settings changed")
    renders = value.get("renders")
    if not isinstance(renders, list) or len(renders) != 2:
        raise ValueError("candidate pitch render pair is missing")
    validated = []
    for index, render in enumerate(renders, 1):
        if not isinstance(render, dict):
            raise ValueError("candidate pitch render binding is invalid")
        expected_path = f"render/candidate-sampler-ps1-pitch-{index}.wav"
        if render.get("path") != expected_path:
            raise ValueError("candidate pitch render path changed")
        path = child(root, expected_path)
        if render.get("sha256") != sha256(path):
            raise ValueError("candidate pitch render digest mismatch")
        analysis = analyze_wave(path)
        if not exact_equal(render.get("analysis"), analysis):
            raise ValueError("candidate pitch render analysis mismatch")
        validated.append((render["sha256"], analysis))
    if validated[0] != validated[1]:
        raise ValueError("candidate pitch render pair is not deterministic")
    span = validated[0][1]["active_span_frames"]
    if not (10000 <= span <= 12050):
        raise ValueError("candidate pitch duration left the expected scoped range")
    return value


def validate_completed_render_attempt(
    attempt: dict, expected_relative_path: str, expected_sha256: str, root: Path
) -> None:
    if not isinstance(attempt, dict):
        raise ValueError("original pitch render attempt is invalid")
    if (
        type(attempt.get("schema_version")) is not int
        or attempt.get("schema_version") != 1
        or attempt.get("outcome") != "rendered"
    ):
        raise ValueError("original pitch render attempt did not complete as rendered")

    required_true = (
        "command_verified",
        "command_dispatched",
        "dialog_verified",
        "controls_configured",
        "save_invoked",
        "close_control_seen",
        "dialog_closed",
        "render_dialog_native_event_hook_armed",
        "render_dialog_event_message_pump_started",
        "render_dialog_dispatch_boundary_set",
    )
    for field in required_true:
        if attempt.get(field) is not True:
            raise ValueError(f"original pitch render attempt lacks verified {field}")

    stable = attempt.get("stable_output_polls")
    if type(stable) is not int or stable < 4:
        raise ValueError("original pitch render attempt lacks stable completed output")
    if attempt.get("process_exited") is not False:
        raise ValueError("original pitch render attempt exited the reference process")
    if attempt.get("process_exit_code") is not None:
        raise ValueError("original pitch completed render retained an exit code")
    preexisting = attempt.get("preexisting_render_dialog_count")
    if type(preexisting) is not int or preexisting != 0:
        raise ValueError("original pitch render started with a preexisting render dialog")
    if attempt.get("dialog_discovery") != (
        "pumped-win-event-object-show-strictly-after-dispatch-tick"
    ):
        raise ValueError("original pitch render dialog attribution changed")

    post_dispatch = attempt.get("render_dialog_post_dispatch_event_count")
    observed = attempt.get("render_dialog_post_dispatch_observed_window_event_count")
    unresolved = attempt.get("render_dialog_unresolved_post_dispatch_event_count")
    if (
        type(post_dispatch) is not int
        or post_dispatch != 1
        or type(unresolved) is not int
        or unresolved != 0
    ):
        raise ValueError("original pitch render dialog event attribution is ambiguous")
    if type(observed) is not int or observed < 1:
        raise ValueError("original pitch render lacks a post-dispatch window event")

    handle = attempt.get("selected_render_dialog_native_handle")
    runtime_id = attempt.get("selected_render_dialog_runtime_id")
    if type(handle) is not int or handle <= 0:
        raise ValueError("original pitch render dialog handle is invalid")
    if (
        not isinstance(runtime_id, list)
        or not runtime_id
        or any(type(value) is not int for value in runtime_id)
    ):
        raise ValueError("original pitch render dialog runtime identity is invalid")
    diagnostics = attempt.get("diagnostics")
    if diagnostics != []:
        raise ValueError("original pitch render attempt retained diagnostics")

    expected_file = Path(expected_relative_path).name
    output = attempt.get("output")
    observed_output = attempt.get("observed_output")
    if not isinstance(output, dict) or not isinstance(observed_output, dict):
        raise ValueError("original pitch render attempt output bindings are missing")
    if output.get("path") != expected_file or output.get("sha256") != expected_sha256:
        raise ValueError("original pitch render attempt output binding changed")
    if (
        observed_output.get("path") != expected_file
        or observed_output.get("sha256") != expected_sha256
    ):
        raise ValueError("original pitch render observed-output binding changed")
    rendered = child(root, expected_relative_path)
    size = observed_output.get("size_bytes")
    if type(size) is not int or size != rendered.stat().st_size or size <= 44:
        raise ValueError("original pitch render observed-output size mismatch")


def original_observation(candidate_root: Path, original_root: Path) -> dict:
    candidate = validate_candidate(candidate_root)
    receipt_path = original_root / ORIGINAL_RECEIPT
    receipt = read_json(receipt_path)
    checks = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "original-observation",
        "contract": CONTRACT,
        "evidence_role": "original",
        "reference_build": REFERENCE_BUILD,
        "reference_file": REFERENCE_FILE,
        "reference_installer_sha256": REFERENCE_INSTALLER_SHA256,
        "reference_installer_size_bytes": REFERENCE_INSTALLER_SIZE_BYTES,
        "reference_executable_sha256": REFERENCE_EXECUTABLE_SHA256,
        "candidate_fixture": candidate["fixture"],
        "fixture_sha256": candidate["fixture_sha256"],
        "original_psycle_observed": True,
        "parity_status": "UNKNOWN",
    }
    for key, expected in checks.items():
        actual = receipt.get(key)
        if actual != expected or type(actual) is not type(expected):
            raise ValueError(f"original pitch receipt identity changed: {key}")

    identity = {
        "reference_build": receipt["reference_build"],
        "reference_file": receipt["reference_file"],
        "reference_installer_sha256": receipt["reference_installer_sha256"],
        "reference_installer_size_bytes": receipt["reference_installer_size_bytes"],
        "reference_executable_sha256": receipt["reference_executable_sha256"],
        "receipt_sha256": sha256(receipt_path),
    }

    runtime = receipt.get("runtime_execution")
    if (
        not isinstance(runtime, dict)
        or type(runtime.get("schema_version")) is not int
        or runtime.get("schema_version") != 1
    ):
        raise ValueError("original pitch runtime observation is missing")
    if not exact_equal(
        runtime.get("settings"),
        {
            "sample_rate": OUTPUT_RATE,
            "bits_per_sample": 16,
            "channels": "mono-mix",
            "dither": False,
            "range": "entire-song",
        },
    ):
        raise ValueError("original pitch render settings changed")

    outcome = runtime.get("outcome")
    attempts = runtime.get("attempts")
    renders = runtime.get("renders")
    if not isinstance(attempts, list) or not isinstance(renders, list):
        raise ValueError("original pitch runtime attempt bindings are invalid")

    if outcome == "rendered-twice":
        if receipt.get("load_result") != "accepted":
            raise ValueError("completed original pitch witness requires accepted load")
        if receipt.get("ui_automation_diagnostics") != []:
            raise ValueError("completed original pitch witness has UI diagnostics")
        if receipt.get("runtime_identity_diagnostics") != []:
            raise ValueError("completed original pitch witness has runtime identity diagnostics")
        if receipt.get("error_marker") is not None:
            raise ValueError("completed original pitch witness has a fixture-load error")
        if receipt.get("application_error_marker") is not None:
            raise ValueError("completed original pitch witness has an application error")
        if receipt.get("main_window_seen") is not True:
            raise ValueError("completed original pitch witness lacks the reference window")

        pre_render = runtime.get("pre_render_load")
        if (
            not isinstance(pre_render, dict)
            or type(pre_render.get("schema_version")) is not int
            or pre_render.get("schema_version") != 1
        ):
            raise ValueError("completed original pitch witness lacks pre-render load evidence")
        if pre_render.get("clean_accepted_load") is not True:
            raise ValueError("completed original pitch witness lacks clean accepted-load evidence")
        if pre_render.get("load_warning_dismissed") is not True:
            raise ValueError("completed original pitch witness did not dismiss the load warning")
        if pre_render.get("process_running_before_render") is not True:
            raise ValueError("completed original pitch witness was not live before render")
        stable = pre_render.get("stable_marker_polls")
        if type(stable) is not int or stable < 4:
            raise ValueError("completed original pitch witness lacks stable load evidence")
        if stable != receipt.get("stable_marker_polls"):
            raise ValueError("pre-render stable load evidence disagrees with the receipt")
        matched = pre_render.get("matched_marker")
        if (
            not isinstance(matched, str)
            or not matched
            or matched != receipt.get("load_evidence_marker")
        ):
            raise ValueError("pre-render load marker disagrees with the accepted receipt")

        if runtime.get("deterministic") is not True or len(renders) != 2:
            raise ValueError("original pitch repeated render is not deterministic")
        if len(attempts) != 2:
            raise ValueError("completed original pitch witness requires exactly two attempts")

        analyses = []
        hashes = []
        for index, render in enumerate(renders, 1):
            if not isinstance(render, dict):
                raise ValueError("original pitch render binding is invalid")
            expected = f"sampler-ps1-pitch/original-sampler-ps1-pitch-{index}.wav"
            if render.get("path") != expected:
                raise ValueError("original pitch render path changed")
            path = child(original_root, expected)
            digest = sha256(path)
            if render.get("sha256") != digest:
                raise ValueError("original pitch render digest mismatch")
            validate_completed_render_attempt(
                attempts[index - 1], expected, digest, original_root
            )
            hashes.append(digest)
            analyses.append(analyze_wave(path))
        if hashes[0] != hashes[1] or analyses[0] != analyses[1]:
            raise ValueError("original pitch repeated renders differ")
        return {
            "outcome": "rendered-twice",
            "deterministic": True,
            "identity": identity,
            "render_sha256s": hashes,
            "analysis": analyses[0],
            "load_result": receipt["load_result"],
        }

    if outcome in {"reference-process-exited-during-render", "inconclusive"}:
        pre_render = runtime.get("pre_render_load")
        if not isinstance(pre_render, dict):
            raise ValueError("blocked original pitch witness lacks pre-render load evidence")
        if attempts:
            last = attempts[-1]
            if not isinstance(last, dict):
                raise ValueError("blocked original pitch attempt is invalid")
        else:
            last = {}
            if (
                pre_render.get("clean_accepted_load") is True
                and pre_render.get("load_warning_dismissed") is True
            ):
                raise ValueError(
                    "clean accepted original pitch witness has no retained render attempt"
                )
        if outcome == "reference-process-exited-during-render":
            if last.get("process_exited") is not True:
                raise ValueError("original pitch process-exit outcome lacks exit evidence")
            code = last.get("process_exit_code")
            if type(code) is not int:
                raise ValueError("original pitch process-exit code is invalid")
        return {
            "outcome": outcome,
            "deterministic": False,
            "identity": identity,
            "render_sha256s": [
                render.get("sha256")
                for render in renders
                if isinstance(render, dict) and isinstance(render.get("sha256"), str)
            ],
            "load_result": receipt.get("load_result"),
            "process_exit_code": last.get("process_exit_code"),
            "diagnostics": last.get("diagnostics", []),
        }

    raise ValueError(f"unexpected original pitch runtime outcome: {outcome!r}")


def compare_observations(
    candidate: dict, original: dict, candidate_source_receipt_sha256: str
) -> dict:
    if (
        not isinstance(candidate_source_receipt_sha256, str)
        or len(candidate_source_receipt_sha256) != 64
        or any(
            char not in "0123456789abcdef"
            for char in candidate_source_receipt_sha256
        )
    ):
        raise ValueError("candidate source receipt SHA-256 is invalid")
    candidate_span = candidate["renders"][0]["analysis"]["active_span_frames"]
    provenance = candidate["renderer_provenance"]
    original_identity = original["identity"]
    candidate_render_hashes = [render["sha256"] for render in candidate["renders"]]
    original_render_hashes = original.get("render_sha256s", [])
    result = {
        "schema_version": 1,
        "phase": "6C",
        "contract": COMPARISON_CONTRACT,
        "scope": "non-44.1-kHz-ps1-pitch",
        "fixture_sha256": candidate["fixture_sha256"],
        "candidate_fixture_sha256": candidate["candidate_fixture_sha256"],
        "candidate_snapshot": candidate["snapshot"],
        "candidate_source_receipt_sha256": candidate_source_receipt_sha256,
        "candidate_renderer_provenance_sha256": provenance["sha256"],
        "candidate_compiled_provenance": provenance["attestation"],
        "candidate_render_sha256s": candidate_render_hashes,
        "original_reference_build": original_identity["reference_build"],
        "original_reference_file": original_identity["reference_file"],
        "original_installer_sha256": original_identity["reference_installer_sha256"],
        "original_installer_size_bytes": original_identity[
            "reference_installer_size_bytes"
        ],
        "original_executable_sha256": original_identity[
            "reference_executable_sha256"
        ],
        "original_source_receipt_sha256": original_identity["receipt_sha256"],
        "original_render_sha256s": original_render_hashes,
        "candidate_active_span_frames": candidate_span,
        "candidate_runtime_pitch_observation": candidate["runtime_pitch_observation"],
        "original_outcome": original["outcome"],
        "sampler_ps1_status": "UNKNOWN",
        "scoped_pitch_status": "UNKNOWN",
        "comparison_ready": False,
        "interpretation_boundary": (
            "The broad Sampler PS1 row remains UNKNOWN regardless of this scoped "
            "pitch witness; later command/envelope/loop/state witnesses are separate."
        ),
        "workflow": {
            "github_run_id": os.environ.get("GITHUB_RUN_ID"),
            "github_sha": os.environ.get("GITHUB_SHA"),
        },
    }
    if original["outcome"] != "rendered-twice":
        result["blocker"] = (
            "Pinned original Psycle did not yield a deterministic completed render; "
            "retain the runtime failure/inconclusive evidence without classifying pitch."
        )
        return result

    if (
        len(candidate_render_hashes) != 2
        or candidate_render_hashes[0] != candidate_render_hashes[1]
        or len(original_render_hashes) != 2
        or original_render_hashes[0] != original_render_hashes[1]
    ):
        raise ValueError("comparison-ready pitch witness lacks deterministic render hashes")

    original_span = original["analysis"]["active_span_frames"]
    ratio = original_span / float(candidate_span)
    result["original_active_span_frames"] = original_span
    result["duration_ratio_original_over_candidate"] = ratio
    if 20000 <= original_span <= 24100 and 1.75 <= ratio <= 2.25:
        result["scoped_pitch_status"] = "DIFFERENT"
        result["comparison_ready"] = True
        result["result"] = (
            "The same non-44.1-kHz authored sample persists for roughly twice as "
            "many output frames under pinned original Psycle as under the frozen "
            "candidate, consistent with sample-rate-aware versus fixed-44.1-kHz "
            "pitch stepping."
        )
    else:
        result["blocker"] = (
            "Both sides rendered, but the measured duration relationship did not "
            "satisfy the predeclared scoped discriminator; pitch remains UNKNOWN."
        )
    return result


def write_new(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8") as handle:
        json.dump(value, handle, indent=2, sort_keys=True)
        handle.write("\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)

    c = sub.add_parser("candidate")
    c.add_argument("root", type=Path)
    cc = sub.add_parser("candidate-check")
    cc.add_argument("root", type=Path)
    o = sub.add_parser("original-check")
    o.add_argument("candidate_root", type=Path)
    o.add_argument("original_root", type=Path)
    comp = sub.add_parser("compare")
    comp.add_argument("candidate_root", type=Path)
    comp.add_argument("original_root", type=Path)
    comp.add_argument("output", type=Path)

    args = parser.parse_args()
    if args.mode == "candidate":
        value = collect_candidate(args.root)
        target = args.root / CANDIDATE_RECEIPT
        write_new(target, value)
    elif args.mode == "candidate-check":
        value = validate_candidate(args.root)
    elif args.mode == "original-check":
        value = original_observation(args.candidate_root, args.original_root)
    else:
        candidate = validate_candidate(args.candidate_root)
        original = original_observation(args.candidate_root, args.original_root)
        value = compare_observations(
            candidate,
            original,
            sha256(args.candidate_root / CANDIDATE_RECEIPT),
        )
        write_new(args.output, value)
    print(json.dumps(value, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
