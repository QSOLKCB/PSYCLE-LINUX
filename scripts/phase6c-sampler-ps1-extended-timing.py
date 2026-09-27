#!/usr/bin/env python3
"""Collect, validate and compare Phase 6C PS1 E-D3/E-C3 runtime timing."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import struct
import wave

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = "sampler-ps1-extended-timing-runtime"
COMPARISON_CONTRACT = "sampler-ps1-extended-timing-runtime-comparison"
SNAPSHOT = "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
VARIANTS = ("delay", "noteoff")
TITLES = {
    "delay": "PSYCLE-LINUX Phase 6C Sampler PS1 E-D3 timing witness",
    "noteoff": "PSYCLE-LINUX Phase 6C Sampler PS1 E-C3 timing witness",
}
COMMANDS = {"delay": "E-D3", "noteoff": "E-C3"}
PARAMETERS = {"delay": 0xD3, "noteoff": 0xC3}
EXPECTED_TRIGGER = 2756
OUTPUT_RATE = 44100
ACTIVE_THRESHOLD = 64
RENDER_FRAMES = 22050

_pitch_spec = importlib.util.spec_from_file_location(
    "phase6c_ps1_pitch", ROOT / "scripts/phase6c-sampler-ps1-pitch.py"
)
assert _pitch_spec is not None and _pitch_spec.loader is not None
pitch = importlib.util.module_from_spec(_pitch_spec)
_pitch_spec.loader.exec_module(pitch)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def write_new(path: Path, value: dict) -> None:
    with path.open("x", encoding="utf-8") as handle:
        json.dump(value, handle, indent=2, sort_keys=True)
        handle.write("\n")


def fixture_paths(root: Path, variant: str) -> tuple[Path, Path, Path]:
    base = root / "fixture"
    return (
        base / f"phase6c-sampler-ps1-extended-{variant}-original.psy",
        base / f"phase6c-sampler-ps1-extended-{variant}-candidate.psy",
        base / f"phase6c-sampler-ps1-extended-{variant}-candidate.pcm16le",
    )


def canonical_source_sha256(path: Path) -> str:
    data = path.read_bytes().replace(b"\r\n", b"\n")
    if b"\r" in data:
        raise ValueError(f"source contains unsupported carriage returns: {path}")
    return hashlib.sha256(data).hexdigest()


def source_hashes() -> dict[str, str]:
    paths = (
        "tests/phase6c_sampler_ps1_extended_timing_fixture.c",
        "tests/phase6c_sampler_ps1_extended_timing.cpp",
        "tests/phase6c_sampler_ps1_extended_timing.pro",
        "scripts/phase6c-sampler-ps1-extended-timing-compat.py",
        "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.cpp",
        "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.h",
        "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/playertimeinfo.cpp",
        "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/player.cpp",
        "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sequencer.cpp",
        "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/machine.cpp",
        "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/internal_machines.cpp",
        "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/constants.h",
        "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/psy3filter.cpp",
    )
    return {path: canonical_source_sha256(ROOT / path) for path in paths}


def binding(root: Path, path: Path) -> dict:
    return {
        "path": path.relative_to(root).as_posix(),
        "sha256": sha256(path),
        "size_bytes": path.stat().st_size,
    }


def expected_probe(variant: str) -> dict:
    base = {
        "schema_version": 1,
        "variant": variant,
        "load_returned": True,
        "bpm": 120,
        "tick_speed": 4,
        "is_ticks": True,
        "sample_rate": 44100,
        "samples_per_beat": 22050,
        "samples_per_tick": 5512.5,
        "timing_parameter": 3,
        "trigger_samples": EXPECTED_TRIGGER,
        "command": COMMANDS[variant],
        "processing_path": "sequencer-player-production-blocks",
        "player_max_work_block_samples": 256,
        "render_frames": RENDER_FRAMES,
        "active_threshold_abs_float": ACTIVE_THRESHOLD,
    }
    if variant == "delay":
        base.update(
            event_position_beats=0,
            semantic_boundary_frame=EXPECTED_TRIGGER,
            absolute_trigger_beats=EXPECTED_TRIGGER / 22050.0,
        )
    else:
        command_frame = int(0.25 * 22050)
        base.update(
            command_position_beats=0.25,
            command_position_frame=command_frame,
            semantic_boundary_frame=command_frame + EXPECTED_TRIGGER,
            absolute_trigger_beats=0.25 + EXPECTED_TRIGGER / 22050.0,
        )
    return base


def validate_probe(value: object, variant: str) -> dict:
    if not isinstance(value, dict):
        raise ValueError("candidate timing probe output is not an object")
    expected = expected_probe(variant)
    for key, wanted in expected.items():
        actual = value.get(key)
        if isinstance(wanted, float):
            if type(actual) not in (int, float) or isinstance(actual, bool):
                raise ValueError(f"candidate timing probe field type changed: {key}")
            if not math.isclose(float(actual), wanted, rel_tol=0.0, abs_tol=1e-9):
                raise ValueError(
                    f"candidate {variant} timing probe field changed: {key}; "
                    f"actual={actual!r}, expected={wanted!r}, "
                    f"delta={float(actual) - wanted:.17g}"
                )
        elif actual != wanted or type(actual) is not type(wanted):
            raise ValueError(f"candidate timing probe field changed: {key}")

    final_beat = value.get("final_play_beat")
    if (
        type(final_beat) not in (int, float)
        or isinstance(final_beat, bool)
        or not math.isclose(float(final_beat), 1.0, rel_tol=0.0, abs_tol=1e-9)
    ):
        raise ValueError("candidate timing production render duration changed")

    active = value.get("audio_active")
    count = value.get("active_frame_count")
    first = value.get("first_active_frame")
    last = value.get("last_active_frame")
    if type(active) is not bool or type(count) is not int or count < 0:
        raise ValueError("candidate timing audio activity fields are invalid")
    if active:
        if (
            type(first) is not int
            or type(last) is not int
            or not (0 <= first <= last < RENDER_FRAMES)
            or count <= 0
        ):
            raise ValueError("candidate timing active-frame bounds are invalid")
    elif first is not None or last is not None or count != 0:
        raise ValueError("candidate timing silent render retained active-frame bounds")
    return value


def collect_candidate(root: Path, probe_build: Path) -> dict:
    root = root.resolve()
    probe_dir = root / "probe"
    probe_dir.mkdir(parents=True, exist_ok=True)
    probe = probe_dir / "phase6c-sampler-ps1-extended-timing-probe"
    if probe.exists():
        raise ValueError("refusing stale timing probe")
    shutil.copy2(probe_build, probe)

    results = {}
    for variant in VARIANTS:
        original, candidate, pcm = fixture_paths(root, variant)
        for path in (original, candidate, pcm):
            if not path.is_file():
                raise ValueError(f"missing timing fixture input: {path}")
        raw = probe_dir / f"{variant}.json"
        log = probe_dir / f"{variant}.log"
        process = subprocess.run(
            [str(probe), variant, str(candidate), str(pcm)],
            cwd=ROOT,
            env={**os.environ, "PSYCLE_THREADS": "1"},
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=30,
            check=False,
        )
        raw.write_bytes(process.stdout)
        log.write_bytes(process.stderr)
        if process.returncode != 0:
            stderr = process.stderr.decode("utf-8", errors="replace").strip()
            stdout = process.stdout.decode("utf-8", errors="replace").strip()
            raise ValueError(
                f"candidate {variant} timing probe failed: "
                f"exit={process.returncode}\n"
                f"stderr:\n{stderr or '(empty)'}\n"
                f"stdout:\n{stdout or '(empty)'}"
            )
        value = validate_probe(json.loads(process.stdout), variant)
        receipt = {
            "schema_version": 1,
            "phase": "6C",
            "scope": "candidate-observation",
            "contract": CONTRACT,
            "evidence_role": "candidate",
            "variant": variant,
            "command": COMMANDS[variant],
            "parameter": PARAMETERS[variant],
            "snapshot": SNAPSHOT,
            "fixture": original.relative_to(root).as_posix(),
            "fixture_sha256": sha256(original),
            "candidate_fixture": candidate.relative_to(root).as_posix(),
            "candidate_fixture_sha256": sha256(candidate),
            "candidate_pcm": pcm.relative_to(root).as_posix(),
            "candidate_pcm_sha256": sha256(pcm),
            "probe": binding(root, probe),
            "raw_probe": binding(root, raw),
            "probe_log": binding(root, log),
            "source_sha256": source_hashes(),
            "runtime_execution": value,
            "original_psycle_observed": False,
            "parity_status": "UNKNOWN",
            "interpretation_boundary": (
                "This establishes frozen-candidate PS1 runtime trigger timing only. "
                "A scoped parity result requires the same authored fixture under "
                "pinned original Psycle."
            ),
        }
        write_new(root / f"candidate-sampler-ps1-extended-{variant}.json", receipt)
        results[variant] = receipt
    return {"variants": results}


def validate_candidate(root: Path) -> dict:
    root = root.resolve()
    results = {}
    expected_sources = source_hashes()
    for variant in VARIANTS:
        receipt_path = root / f"candidate-sampler-ps1-extended-{variant}.json"
        receipt = read_json(receipt_path)
        checks = {
            "schema_version": 1,
            "phase": "6C",
            "scope": "candidate-observation",
            "contract": CONTRACT,
            "evidence_role": "candidate",
            "variant": variant,
            "command": COMMANDS[variant],
            "parameter": PARAMETERS[variant],
            "snapshot": SNAPSHOT,
            "original_psycle_observed": False,
            "parity_status": "UNKNOWN",
            "source_sha256": expected_sources,
        }
        for key, expected in checks.items():
            if not pitch.exact_equal(receipt.get(key), expected):
                raise ValueError(f"candidate timing identity changed: {variant}:{key}")

        original, candidate, pcm = fixture_paths(root, variant)
        expected_paths = {
            "fixture": original,
            "candidate_fixture": candidate,
            "candidate_pcm": pcm,
        }
        for key, path in expected_paths.items():
            if receipt.get(key) != path.relative_to(root).as_posix():
                raise ValueError(f"candidate timing path changed: {variant}:{key}")
            if not path.is_file() or receipt.get(key + "_sha256") != sha256(path):
                raise ValueError(f"candidate timing digest changed: {variant}:{key}")

        probe = receipt.get("probe")
        raw = receipt.get("raw_probe")
        log = receipt.get("probe_log")
        for item in (probe, raw, log):
            if not isinstance(item, dict):
                raise ValueError("candidate timing binding missing")
            path = (root / item.get("path", "")).resolve()
            path.relative_to(root)
            if (
                not path.is_file()
                or item.get("sha256") != sha256(path)
                or item.get("size_bytes") != path.stat().st_size
            ):
                raise ValueError("candidate timing binding mismatch")
        value = validate_probe(json.loads((root / raw["path"]).read_bytes()), variant)
        if not pitch.exact_equal(receipt.get("runtime_execution"), value):
            raise ValueError("candidate timing runtime receipt drift")
        results[variant] = receipt
    return {"variants": results}


def analyze_timing_wave(path: Path) -> dict:
    with wave.open(str(path), "rb") as handle:
        channels = handle.getnchannels()
        width = handle.getsampwidth()
        rate = handle.getframerate()
        frames = handle.getnframes()
        payload = handle.readframes(frames)
    if channels != 1 or width != 2 or rate != OUTPUT_RATE:
        raise ValueError("timing witness WAV format changed")
    if len(payload) != frames * 2:
        raise ValueError("timing witness WAV payload is truncated")
    samples = struct.unpack("<" + "h" * frames, payload) if frames else ()
    active = [
        index for index, sample in enumerate(samples)
        if abs(sample) > ACTIVE_THRESHOLD
    ]
    if not active:
        return {
            "sample_rate": rate,
            "channels": channels,
            "bits_per_sample": width * 8,
            "frame_count": frames,
            "active_threshold_abs_pcm16": ACTIVE_THRESHOLD,
            "audio_active": False,
            "first_active_frame": None,
            "last_active_frame": None,
            "active_frame_count": 0,
            "active_span_frames": 0,
            "active_span_seconds": 0.0,
        }
    first = active[0]
    last = active[-1]
    return {
        "sample_rate": rate,
        "channels": channels,
        "bits_per_sample": width * 8,
        "frame_count": frames,
        "active_threshold_abs_pcm16": ACTIVE_THRESHOLD,
        "audio_active": True,
        "first_active_frame": first,
        "last_active_frame": last,
        "active_frame_count": len(active),
        "active_span_frames": last - first + 1,
        "active_span_seconds": (last - first + 1) / float(rate),
    }


def clean_load_gate(receipt: dict, runtime: dict) -> None:
    pre = runtime.get("pre_render_load")
    outcome = runtime.get("outcome")
    load_result = receipt.get("load_result")
    if outcome == "rendered-twice":
        load_ok = load_result == "accepted"
    elif outcome in ("reference-process-exited-during-render", "inconclusive"):
        load_ok = load_result in ("accepted", "inconclusive")
    else:
        load_ok = False
    if (
        not load_ok
        or receipt.get("ui_automation_diagnostics") != []
        or receipt.get("runtime_identity_diagnostics") != []
        or receipt.get("error_marker") is not None
        or receipt.get("application_error_marker") is not None
        or receipt.get("main_window_seen") is not True
        or not isinstance(pre, dict)
        or pre.get("clean_accepted_load") is not True
        or pre.get("load_warning_dismissed") is not True
        or pre.get("process_running_before_render") is not True
    ):
        raise ValueError("original timing witness lacks clean pre-render load gate")
    stable = pre.get("stable_marker_polls")
    if (
        type(stable) is not int
        or stable < 4
        or stable != receipt.get("stable_marker_polls")
        or pre.get("matched_marker") != receipt.get("load_evidence_marker")
    ):
        raise ValueError("original timing stable-load evidence changed")


def validate_original_variant(
    candidate_root: Path, original_root: Path, variant: str
) -> dict:
    candidate = validate_candidate(candidate_root)["variants"][variant]
    receipt_path = original_root / f"original-sampler-ps1-extended-{variant}.json"
    receipt = read_json(receipt_path)
    identity = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "evidence_role": "original",
        "reference_build": pitch.REFERENCE_BUILD,
        "reference_file": pitch.REFERENCE_FILE,
        "reference_installer_sha256": pitch.REFERENCE_INSTALLER_SHA256,
        "reference_installer_size_bytes": pitch.REFERENCE_INSTALLER_SIZE_BYTES,
        "reference_executable_sha256": pitch.REFERENCE_EXECUTABLE_SHA256,
        "candidate_fixture": candidate["fixture"],
        "fixture_sha256": candidate["fixture_sha256"],
        "original_psycle_observed": True,
        "parity_status": "UNKNOWN",
    }
    for key, expected in identity.items():
        actual = receipt.get(key)
        if actual != expected or type(actual) is not type(expected):
            raise ValueError(f"original timing identity changed: {variant}:{key}")

    runtime = receipt.get("runtime_execution")
    if not isinstance(runtime, dict) or runtime.get("schema_version") != 1:
        raise ValueError("original timing runtime observation missing")
    if runtime.get("deterministic") not in (True, False):
        raise ValueError("original timing deterministic field invalid")
    clean_load_gate(receipt, runtime)

    attempts = runtime.get("attempts")
    renders = runtime.get("renders")
    if not isinstance(attempts, list) or not isinstance(renders, list):
        raise ValueError("original timing render bindings invalid")
    outcome = runtime.get("outcome")
    hashes: list[str] = []
    analyses: list[dict] = []

    if outcome == "rendered-twice":
        if runtime.get("deterministic") is not True or len(attempts) != 2 or len(renders) != 2:
            raise ValueError("completed original timing pair is not deterministic")
        for index, render in enumerate(renders, 1):
            expected = (
                f"sampler-ps1-extended-{variant}/"
                f"original-sampler-ps1-extended-{variant}-{index}.wav"
            )
            if not isinstance(render, dict) or render.get("path") != expected:
                raise ValueError("original timing render path changed")
            path = original_root / expected
            digest = sha256(path)
            if render.get("sha256") != digest:
                raise ValueError("original timing render digest changed")
            pitch.validate_completed_render_attempt(
                attempts[index - 1], expected, digest, original_root
            )
            hashes.append(digest)
            analyses.append(analyze_timing_wave(path))
        if hashes[0] != hashes[1] or analyses[0] != analyses[1]:
            raise ValueError("original timing repeated renders differ")
    elif outcome in ("reference-process-exited-during-render", "inconclusive"):
        if runtime.get("deterministic") is not False:
            raise ValueError("blocked original timing witness claims determinism")
        hashes = pitch.validate_retained_render_bindings(
            renders,
            attempts,
            original_root,
            allow_incomplete_last_teardown=(outcome == "inconclusive"),
            expected_path_prefix=(
                f"sampler-ps1-extended-{variant}/"
                f"original-sampler-ps1-extended-{variant}"
            ),
        )
        if outcome == "reference-process-exited-during-render":
            if not attempts or not isinstance(attempts[-1], dict):
                raise ValueError("original timing process-exit attempt missing")
            last = attempts[-1]
            for field in (
                "command_verified",
                "command_dispatched",
                "dialog_verified",
                "controls_configured",
                "save_invoked",
            ):
                if last.get(field) is not True:
                    raise ValueError(
                        f"original timing process-exit lacks verified {field}"
                    )
            if last.get("process_exited") is not True or type(
                last.get("process_exit_code")
            ) is not int:
                raise ValueError("original timing process-exit evidence invalid")
    else:
        raise ValueError(f"unexpected original timing outcome: {outcome!r}")

    return {
        "variant": variant,
        "outcome": outcome,
        "deterministic": runtime["deterministic"],
        "render_sha256s": hashes,
        "analysis": analyses[0] if analyses else None,
        "identity": {
            "receipt_sha256": sha256(receipt_path),
            "reference_build": receipt["reference_build"],
            "reference_executable_sha256": receipt["reference_executable_sha256"],
        },
        "process_exit_code": (
            attempts[-1].get("process_exit_code") if attempts else None
        ),
    }


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    return {
        "variants": {
            variant: validate_original_variant(
                candidate_root.resolve(), original_root.resolve(), variant
            )
            for variant in VARIANTS
        }
    }


def compare(candidate_root: Path, original_root: Path) -> dict:
    candidate = validate_candidate(candidate_root)
    original = validate_original(candidate_root, original_root)
    candidate_runtime = {
        variant: candidate["variants"][variant]["runtime_execution"]
        for variant in VARIANTS
    }
    result = {
        "schema_version": 1,
        "phase": "6C",
        "contract": COMPARISON_CONTRACT,
        "scope": "ps1-e-dx-e-cx-runtime-timing",
        "candidate_snapshot": SNAPSHOT,
        "commands": ["E-D3", "E-C3"],
        "candidate_trigger_samples": {
            variant: candidate_runtime[variant]["trigger_samples"]
            for variant in VARIANTS
        },
        "candidate_processing_path": {
            variant: candidate_runtime[variant]["processing_path"]
            for variant in VARIANTS
        },
        "candidate_audio_observations": {
            variant: {
                "audio_active": candidate_runtime[variant]["audio_active"],
                "first_active_frame": candidate_runtime[variant][
                    "first_active_frame"
                ],
                "last_active_frame": candidate_runtime[variant][
                    "last_active_frame"
                ],
                "semantic_boundary_frame": candidate_runtime[variant][
                    "semantic_boundary_frame"
                ],
            }
            for variant in VARIANTS
        },
        "original_outcomes": {
            variant: original["variants"][variant]["outcome"]
            for variant in VARIANTS
        },
        "original_receipt_sha256s": {
            variant: original["variants"][variant]["identity"]["receipt_sha256"]
            for variant in VARIANTS
        },
        "original_render_sha256s": {
            variant: original["variants"][variant]["render_sha256s"]
            for variant in VARIANTS
        },
        "sampler_ps1_status": "UNKNOWN",
        "scoped_timing_status": "UNKNOWN",
        "comparison_ready": False,
        "workflow": {
            "github_run_id": os.environ.get("GITHUB_RUN_ID"),
            "github_sha": os.environ.get("GITHUB_SHA"),
        },
    }

    if any(
        original["variants"][variant]["outcome"] != "rendered-twice"
        for variant in VARIANTS
    ):
        result["blocker"] = (
            "Pinned original Psycle did not yield deterministic completed renders "
            "for both E-D3 and E-C3 fixtures; retain exact runtime blocker evidence "
            "without classifying PS1 extended timing."
        )
        return result

    delay = original["variants"]["delay"]["analysis"]
    noteoff = original["variants"]["noteoff"]["analysis"]
    if delay is None or noteoff is None:
        raise ValueError("comparison-ready timing observation lacks WAV analysis")

    candidate_delay = candidate_runtime["delay"]
    candidate_noteoff = candidate_runtime["noteoff"]
    result["original_audio_observations"] = {
        "delay": delay,
        "noteoff": noteoff,
    }

    pairs = (
        ("delay", candidate_delay, delay, "first_active_frame", 4),
        ("noteoff", candidate_noteoff, noteoff, "last_active_frame", 6),
    )
    boundary_results: dict[str, bool | None] = {}
    for variant, candidate_audio, original_audio, field, tolerance in pairs:
        candidate_active = candidate_audio["audio_active"]
        original_active = original_audio["audio_active"]
        if candidate_active != original_active:
            boundary_results[variant] = False
            continue
        if not candidate_active:
            boundary_results[variant] = None
            continue
        boundary_results[variant] = (
            abs(candidate_audio[field] - original_audio[field]) <= tolerance
        )

    result["boundary_match"] = boundary_results
    if any(value is False for value in boundary_results.values()):
        result["scoped_timing_status"] = "DIFFERENT"
        result["comparison_ready"] = True
        result["result"] = (
            "The deterministic candidate and pinned-original renders differ in "
            "audible E-D3/E-C3 timing or activity for the exact paired fixtures."
        )
    elif all(value is True for value in boundary_results.values()):
        result["scoped_timing_status"] = "PASS"
        result["comparison_ready"] = True
        result["result"] = (
            "The deterministic candidate and pinned-original renders agree on "
            "the measured audible E-D3 onset and E-C3 note-off boundaries."
        )
    else:
        result["blocker"] = (
            "Both sides rendered deterministically, but at least one paired "
            "fixture was silent on both sides, so an audible timing boundary "
            "cannot be classified."
        )
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)
    collect = sub.add_parser("collect")
    collect.add_argument("root", type=Path)
    collect.add_argument("probe", type=Path)
    cand = sub.add_parser("candidate-check")
    cand.add_argument("root", type=Path)
    orig = sub.add_parser("original-check")
    orig.add_argument("candidate_root", type=Path)
    orig.add_argument("original_root", type=Path)
    comp = sub.add_parser("compare")
    comp.add_argument("candidate_root", type=Path)
    comp.add_argument("original_root", type=Path)
    comp.add_argument("output", type=Path)
    args = parser.parse_args()

    if args.mode == "collect":
        value = collect_candidate(args.root, args.probe)
    elif args.mode == "candidate-check":
        value = validate_candidate(args.root)
    elif args.mode == "original-check":
        value = validate_original(args.candidate_root, args.original_root)
    else:
        value = compare(args.candidate_root, args.original_root)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write_new(args.output, value)
    print(json.dumps(value, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
