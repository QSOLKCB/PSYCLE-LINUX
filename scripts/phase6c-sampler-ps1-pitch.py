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
import subprocess
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
RENDERER_BUILD_MANIFEST = "renderer-build-inputs.json"
RENDERER_EXECUTABLE = "renderer/phase6c-sampler-ps1-pitch-render"
RENDERER_BUILD_SCOPE = "phase6c-sampler-ps1-pitch-render-build-closure"
ORIGINAL_FIXTURE = "fixture/phase6c-sampler-ps1-pitch-original.psy"
CANDIDATE_FIXTURE = "fixture/phase6c-sampler-ps1-pitch-candidate.psy"
CANDIDATE_PCM = "fixture/phase6c-sampler-ps1-pitch-candidate.pcm16le"
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


def renderer_provenance_sources() -> dict[str, Path]:
    return {
        "renderer_source_sha256": ROOT / "tests/phase6c_sampler_ps1_pitch_render.cpp",
        "renderer_project_sha256": ROOT / "tests/phase6c_sampler_ps1_pitch_render.pro",
        "engine_sequencer_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sequencer.cpp"
        ),
        "engine_song_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/song.cpp"
        ),
        "engine_sequence_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sequence.cpp"
        ),
        "engine_pattern_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/pattern.cpp"
        ),
        "engine_machinefactory_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/machinefactory.cpp"
        ),
        "engine_sampler_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.cpp"
        ),
        "engine_instrument_sha256": (
            ROOT
            / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/instrument.cpp"
        ),
        "compat_script_sha256": COMPAT_PATH,
        "fixture_generator_sha256": ROOT / "tests/phase6c_sampler_ps1_pitch_fixture.c",
    }


def expected_renderer_provenance() -> dict:
    return {
        "schema_version": 1,
        **{
            name: sha256(path)
            for name, path in renderer_provenance_sources().items()
        },
    }


def renderer_build_input_selectors() -> tuple[str, ...]:
    return (
        "tests/phase6c_sampler_ps1_pitch_render.cpp",
        "tests/phase6c_sampler_ps1_pitch_render.pro",
        "psycle-cpp-r12005-sanitized/build-systems",
        "psycle-cpp-r12005-sanitized/diversalis",
        "psycle-cpp-r12005-sanitized/universalis",
        "psycle-cpp-r12005-sanitized/psycle-helpers",
        "psycle-cpp-r12005-sanitized/psycle-audiodrivers",
        "psycle-cpp-r12005-sanitized/psycle-core",
        "psycle-cpp-r12005-sanitized/psycle-plugins/src",
    )


def renderer_build_input_paths() -> list[Path]:
    paths: set[Path] = set()
    for selector in renderer_build_input_selectors():
        path = ROOT / selector
        if not path.exists():
            raise ValueError(f"candidate renderer build input is missing: {selector}")
        if path.is_file():
            paths.add(path)
            continue
        for candidate in path.rglob("*"):
            if not candidate.is_file():
                continue
            relative_parts = candidate.relative_to(ROOT).parts
            if (
                "++qmake" in relative_parts
                or ".git" in relative_parts
                or "__pycache__" in relative_parts
            ):
                continue
            paths.add(candidate)
    return sorted(paths, key=lambda item: item.relative_to(ROOT).as_posix())


def renderer_build_input_records() -> list[dict]:
    return [
        {
            "path": path.relative_to(ROOT).as_posix(),
            "sha256": sha256(path),
        }
        for path in renderer_build_input_paths()
    ]


def git_head() -> str:
    result = subprocess.run(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    value = result.stdout.strip()
    if (
        result.returncode != 0
        or len(value) != 40
        or any(char not in "0123456789abcdef" for char in value)
    ):
        raise ValueError("candidate renderer build could not resolve Git HEAD")
    return value


def ensure_renderer_build_inputs_clean() -> None:
    selectors = list(renderer_build_input_selectors())
    for args in (
        ["diff", "--quiet", "HEAD", "--", *selectors],
        ["diff", "--cached", "--quiet", "HEAD", "--", *selectors],
    ):
        result = subprocess.run(
            ["git", "-C", str(ROOT), *args],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        if result.returncode != 0:
            raise ValueError("candidate renderer tracked build inputs are dirty")

    untracked = subprocess.run(
        [
            "git",
            "-C",
            str(ROOT),
            "ls-files",
            "--others",
            "--exclude-standard",
            "-z",
            "--",
            *selectors,
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if untracked.returncode != 0:
        raise ValueError("candidate renderer untracked-input audit failed")
    allowed_file = (
        "psycle-cpp-r12005-sanitized/psycle-plugins/src/"
        "psycle/plugin_interface.hpp"
    )
    allowed_prefix = "psycle-cpp-r12005-sanitized/diversalis/"
    unexpected = []
    for raw in untracked.stdout.split(b"\0"):
        if not raw:
            continue
        relative = raw.decode("utf-8", errors="strict")
        if "++qmake/" in relative:
            continue
        if relative == allowed_file or relative.startswith(allowed_prefix):
            continue
        unexpected.append(relative)
    if unexpected:
        raise ValueError(
            "candidate renderer has unexpected untracked build inputs: "
            + ", ".join(sorted(unexpected)[:5])
        )


def expected_renderer_build_manifest(root: Path) -> dict:
    executable = child(root, RENDERER_EXECUTABLE)
    if not executable.is_file():
        raise ValueError("candidate renderer executable is missing")
    ensure_renderer_build_inputs_clean()
    return {
        "schema_version": 1,
        "scope": RENDERER_BUILD_SCOPE,
        "git_head": git_head(),
        "renderer_executable": RENDERER_EXECUTABLE,
        "renderer_executable_sha256": sha256(executable),
        "inputs": renderer_build_input_records(),
    }


def write_renderer_build_manifest(root: Path) -> dict:
    root = root.resolve()
    manifest = expected_renderer_build_manifest(root)
    path = child(root, RENDERER_BUILD_MANIFEST)
    if path.exists():
        raise ValueError("candidate renderer build manifest already exists")
    path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return manifest


def validate_renderer_build_manifest(
    root: Path, *, verify_checkout_sources: bool = True
) -> dict:
    root = root.resolve()
    manifest_path = child(root, RENDERER_BUILD_MANIFEST)
    if not manifest_path.is_file():
        raise ValueError("candidate renderer build-input manifest is missing")
    manifest = read_json(manifest_path)
    if set(manifest) != {
        "schema_version",
        "scope",
        "git_head",
        "renderer_executable",
        "renderer_executable_sha256",
        "inputs",
    }:
        raise ValueError("candidate renderer build-input manifest field set changed")
    if type(manifest.get("schema_version")) is not int or manifest["schema_version"] != 1:
        raise ValueError("candidate renderer build-input manifest schema changed")
    if manifest.get("scope") != RENDERER_BUILD_SCOPE:
        raise ValueError("candidate renderer build-input manifest scope changed")
    head = manifest.get("git_head")
    if (
        not isinstance(head, str)
        or len(head) != 40
        or any(char not in "0123456789abcdef" for char in head)
    ):
        raise ValueError("candidate renderer build-input Git identity is invalid")
    if manifest.get("renderer_executable") != RENDERER_EXECUTABLE:
        raise ValueError("candidate renderer executable path changed")
    executable = child(root, RENDERER_EXECUTABLE)
    executable_sha = manifest.get("renderer_executable_sha256")
    if (
        not isinstance(executable_sha, str)
        or len(executable_sha) != 64
        or any(char not in "0123456789abcdef" for char in executable_sha)
        or not executable.is_file()
        or sha256(executable) != executable_sha
    ):
        raise ValueError("candidate renderer executable identity changed")

    inputs = manifest.get("inputs")
    if not isinstance(inputs, list) or not inputs:
        raise ValueError("candidate renderer build-input manifest is empty")
    previous = ""
    for item in inputs:
        if not isinstance(item, dict) or set(item) != {"path", "sha256"}:
            raise ValueError("candidate renderer build-input record changed")
        relative = item.get("path")
        digest = item.get("sha256")
        if (
            not isinstance(relative, str)
            or not relative
            or relative <= previous
            or relative.startswith("/")
            or ".." in Path(relative).parts
        ):
            raise ValueError("candidate renderer build-input path is invalid")
        if (
            not isinstance(digest, str)
            or len(digest) != 64
            or any(char not in "0123456789abcdef" for char in digest)
        ):
            raise ValueError("candidate renderer build-input digest is invalid")
        previous = relative

    if verify_checkout_sources:
        expected = expected_renderer_build_manifest(root)
        if not exact_equal(manifest, expected):
            raise ValueError(
                "candidate renderer build-input manifest does not match the "
                "clean checked-out/staged build closure"
            )
    return {
        "path": RENDERER_BUILD_MANIFEST,
        "sha256": sha256(manifest_path),
        "git_head": head,
        "renderer_executable": RENDERER_EXECUTABLE,
        "renderer_executable_sha256": executable_sha,
        "input_count": len(inputs),
    }


def validate_renderer_provenance(
    root: Path, *, verify_checkout_sources: bool = True
) -> dict:
    path = child(root, RENDERER_PROVENANCE)
    if not path.is_file():
        raise ValueError("candidate compiled-renderer provenance is missing")
    attestation = read_json(path)
    expected_keys = {"schema_version", *renderer_provenance_sources().keys()}
    if set(attestation) != expected_keys:
        raise ValueError("candidate compiled-renderer provenance field set changed")
    if type(attestation.get("schema_version")) is not int or attestation["schema_version"] != 1:
        raise ValueError("candidate compiled-renderer provenance schema changed")
    for key in renderer_provenance_sources():
        digest = attestation.get(key)
        if (
            not isinstance(digest, str)
            or len(digest) != 64
            or any(char not in "0123456789abcdef" for char in digest)
        ):
            raise ValueError(f"candidate compiled-renderer provenance digest is invalid: {key}")
    if verify_checkout_sources:
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


def read_last_json_object(path: Path) -> dict:
    if not path.is_file():
        raise ValueError(f"missing JSON log: {path}")
    values = []
    for line in path.read_text(encoding="utf-8", errors="strict").splitlines():
        stripped = line.strip()
        if not stripped.startswith("{"):
            continue
        try:
            value = json.loads(stripped)
        except json.JSONDecodeError:
            continue
        if isinstance(value, dict):
            values.append(value)
    if len(values) != 1:
        raise ValueError(f"expected exactly one JSON object in log: {path}")
    return values[0]


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


def require_fixture_pair(root: Path) -> tuple[Path, Path, Path, dict]:
    original_fixture = child(root, ORIGINAL_FIXTURE)
    candidate_fixture = child(root, CANDIDATE_FIXTURE)
    candidate_pcm = child(root, CANDIDATE_PCM)
    if (
        not original_fixture.is_file()
        or not candidate_fixture.is_file()
        or not candidate_pcm.is_file()
    ):
        raise ValueError("Sampler pitch fixture pair or PCM sidecar is missing")
    info = compat.inspect_pair(
        original_fixture.read_bytes(),
        candidate_fixture.read_bytes(),
        candidate_pcm.read_bytes(),
    )
    if (
        info["authored_sample_rate"] != AUTHORED_SAMPLE_RATE
        or info["authored_sample_frames"] != AUTHORED_SAMPLE_FRAMES
        or info["pcm16le_bytes"] != AUTHORED_SAMPLE_FRAMES * 2
        or info["candidate_pre_injection_wave_state"] != "empty"
        or info["candidate_modern_sample_chunk_removed"] is not True
    ):
        raise ValueError("Sampler pitch fixture-pair semantic identity changed")
    return original_fixture, candidate_fixture, candidate_pcm, info

def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    original_fixture, candidate_fixture, candidate_pcm, fixture_info = require_fixture_pair(root)
    candidate_fixture_sha = sha256(candidate_fixture)
    candidate_pcm_sha = sha256(candidate_pcm)
    renderer_build_identity = validate_renderer_build_manifest(root)
    renders = []
    for index in (1, 2):
        path = child(root, f"render/candidate-sampler-ps1-pitch-{index}.wav")
        if not path.is_file():
            raise ValueError("candidate pitch render is missing")
        render_sha = sha256(path)
        log_path = child(root, f"render/candidate-render-{index}.log")
        execution = read_last_json_object(log_path)
        expected_execution = {
            "schema_version": 1,
            "fixed_frame_render": True,
            "renderer_executable_sha256": renderer_build_identity[
                "renderer_executable_sha256"
            ],
            "sample_rate": OUTPUT_RATE,
            "channels": 1,
            "bits_per_sample": 16,
            "target_frames": OUTPUT_RATE,
            "threads": 1,
            "sequencer_work_calls": 1,
            "input_sha256": candidate_fixture_sha,
            "candidate_fixture_loader_bypassed": True,
            "harness_song_topology": "sampler-0-to-master-one-note",
            "harness_bpm": 120,
            "harness_lpb": 4,
            "harness_note": NOTE,
            "harness_track": 0,
            "harness_machine": 0,
            "harness_instrument": 0,
            "pcm_size_bytes": AUTHORED_SAMPLE_FRAMES * 2,
            "pcm_sha256": candidate_pcm_sha,
            "harness_sample_injection": True,
            "pre_injection_wave_length": 0,
            "injected_wave_length": AUTHORED_SAMPLE_FRAMES,
            "injected_wave_volume": 100,
            "injected_wave_tune": 0,
            "injected_wave_finetune": 0,
            "output_size_bytes": path.stat().st_size,
            "output_sha256": render_sha,
        }
        for key, expected in expected_execution.items():
            if not exact_equal(execution.get(key), expected):
                raise ValueError(
                    f"candidate pitch renderer execution binding changed: {key}"
                )
        renders.append(
            {
                "path": str(path.relative_to(root)),
                "sha256": render_sha,
                "log": str(log_path.relative_to(root)),
                "log_sha256": sha256(log_path),
                "execution": execution,
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
        "renderer_build_identity": renderer_build_identity,
        "fixture": ORIGINAL_FIXTURE,
        "fixture_sha256": sha256(original_fixture),
        "candidate_fixture": CANDIDATE_FIXTURE,
        "candidate_fixture_sha256": candidate_fixture_sha,
        "candidate_pcm": CANDIDATE_PCM,
        "candidate_pcm_sha256": candidate_pcm_sha,
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


def validate_candidate(
    root: Path,
    value: dict | None = None,
    *,
    verify_checkout_sources: bool = True,
) -> dict:
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
        "candidate_pcm": CANDIDATE_PCM,
        "parity_status": "UNKNOWN",
        "deterministic": True,
        "repeat_count": 2,
        "runtime_pitch_observation": "fixed-44100-basis-compatible-duration",
    }
    for key, expected in checks.items():
        if value.get(key) != expected or type(value.get(key)) is not type(expected):
            raise ValueError(f"candidate pitch receipt identity changed: {key}")
    provenance = validate_renderer_provenance(
        root, verify_checkout_sources=verify_checkout_sources
    )
    if not exact_equal(value.get("renderer_provenance"), provenance):
        raise ValueError("candidate renderer provenance binding changed")
    renderer_build_identity = validate_renderer_build_manifest(
        root, verify_checkout_sources=verify_checkout_sources
    )
    if not exact_equal(value.get("renderer_build_identity"), renderer_build_identity):
        raise ValueError("candidate renderer executable/build-input binding changed")
    original_fixture, candidate_fixture, candidate_pcm, info = require_fixture_pair(root)
    if value.get("fixture_sha256") != sha256(original_fixture):
        raise ValueError("original-generation pitch fixture digest mismatch")
    if value.get("candidate_fixture_sha256") != sha256(candidate_fixture):
        raise ValueError("candidate-generation pitch fixture digest mismatch")
    if value.get("candidate_pcm_sha256") != sha256(candidate_pcm):
        raise ValueError("candidate pitch PCM sidecar digest mismatch")
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
        expected_log = f"render/candidate-render-{index}.log"
        if render.get("path") != expected_path or render.get("log") != expected_log:
            raise ValueError("candidate pitch render/log path changed")
        path = child(root, expected_path)
        log_path = child(root, expected_log)
        render_sha = sha256(path)
        if render.get("sha256") != render_sha:
            raise ValueError("candidate pitch render digest mismatch")
        if render.get("log_sha256") != sha256(log_path):
            raise ValueError("candidate pitch render log digest mismatch")
        execution = read_last_json_object(log_path)
        if not exact_equal(render.get("execution"), execution):
            raise ValueError("candidate pitch renderer execution record changed")
        expected_execution = {
            "schema_version": 1,
            "fixed_frame_render": True,
            "renderer_executable_sha256": renderer_build_identity[
                "renderer_executable_sha256"
            ],
            "sample_rate": OUTPUT_RATE,
            "channels": 1,
            "bits_per_sample": 16,
            "target_frames": OUTPUT_RATE,
            "threads": 1,
            "sequencer_work_calls": 1,
            "input_sha256": sha256(candidate_fixture),
            "candidate_fixture_loader_bypassed": True,
            "harness_song_topology": "sampler-0-to-master-one-note",
            "harness_bpm": 120,
            "harness_lpb": 4,
            "harness_note": NOTE,
            "harness_track": 0,
            "harness_machine": 0,
            "harness_instrument": 0,
            "pcm_size_bytes": AUTHORED_SAMPLE_FRAMES * 2,
            "pcm_sha256": sha256(candidate_pcm),
            "harness_sample_injection": True,
            "pre_injection_wave_length": 0,
            "injected_wave_length": AUTHORED_SAMPLE_FRAMES,
            "injected_wave_volume": 100,
            "injected_wave_tune": 0,
            "injected_wave_finetune": 0,
            "output_size_bytes": path.stat().st_size,
            "output_sha256": render_sha,
        }
        for key, expected in expected_execution.items():
            if not exact_equal(execution.get(key), expected):
                raise ValueError(
                    f"candidate pitch renderer execution binding changed: {key}"
                )
        analysis = analyze_wave(path)
        if analysis["frame_count"] != OUTPUT_RATE:
            raise ValueError(
                "candidate fixed-frame render does not contain exactly 44100 frames"
            )
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
    # Candidate build inputs were verified on the Linux builder before sealing the
    # artifact. Cross-platform consumers verify the sealed provenance shape and
    # hashes without re-hashing a checkout whose line-ending policy may differ.
    candidate = validate_candidate(
        candidate_root, verify_checkout_sources=False
    )
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
        if (
            not isinstance(pre_render, dict)
            or type(pre_render.get("schema_version")) is not int
            or pre_render.get("schema_version") != 1
        ):
            raise ValueError("blocked original pitch witness lacks pre-render load evidence")

        clean_gate = (
            pre_render.get("clean_accepted_load") is True
            and pre_render.get("load_warning_dismissed") is True
            and pre_render.get("process_running_before_render") is True
            and receipt.get("ui_automation_diagnostics") == []
            and receipt.get("runtime_identity_diagnostics") == []
            and receipt.get("error_marker") is None
            and receipt.get("application_error_marker") is None
            and receipt.get("main_window_seen") is True
        )
        stable = pre_render.get("stable_marker_polls")
        matched = pre_render.get("matched_marker")
        if (
            not clean_gate
            or type(stable) is not int
            or stable < 4
            or stable != receipt.get("stable_marker_polls")
            or not isinstance(matched, str)
            or not matched
            or matched != receipt.get("load_evidence_marker")
        ):
            raise ValueError(
                "blocked original pitch witness lacks a clean accepted-load gate"
            )

        if not attempts:
            raise ValueError(
                "clean accepted original pitch witness has no retained render attempt"
            )
        last = attempts[-1]
        if not isinstance(last, dict):
            raise ValueError("blocked original pitch attempt is invalid")

        if outcome == "reference-process-exited-during-render":
            required_dispatch = (
                "command_verified",
                "command_dispatched",
                "dialog_verified",
                "controls_configured",
                "save_invoked",
                "render_dialog_native_event_hook_armed",
                "render_dialog_event_message_pump_started",
                "render_dialog_dispatch_boundary_set",
            )
            for field in required_dispatch:
                if last.get(field) is not True:
                    raise ValueError(
                        f"original pitch process-exit outcome lacks verified {field}"
                    )
            if (
                type(last.get("schema_version")) is not int
                or last.get("schema_version") != 1
                or last.get("outcome") != "inconclusive"
            ):
                raise ValueError(
                    "original pitch process-exit attempt identity changed"
                )
            if last.get("preexisting_render_dialog_count") != 0:
                raise ValueError(
                    "original pitch process-exit attempt had a preexisting render dialog"
                )
            if last.get("dialog_discovery") != (
                "pumped-win-event-object-show-strictly-after-dispatch-tick"
            ):
                raise ValueError(
                    "original pitch process-exit render dialog attribution changed"
                )
            if (
                type(last.get("render_dialog_post_dispatch_event_count")) is not int
                or last.get("render_dialog_post_dispatch_event_count") != 1
                or type(
                    last.get(
                        "render_dialog_post_dispatch_observed_window_event_count"
                    )
                )
                is not int
                or last.get(
                    "render_dialog_post_dispatch_observed_window_event_count"
                )
                < 1
                or type(
                    last.get("render_dialog_unresolved_post_dispatch_event_count")
                )
                is not int
                or last.get("render_dialog_unresolved_post_dispatch_event_count")
                != 0
            ):
                raise ValueError(
                    "original pitch process-exit dialog attribution is ambiguous"
                )
            handle = last.get("selected_render_dialog_native_handle")
            runtime_id = last.get("selected_render_dialog_runtime_id")
            if type(handle) is not int or handle <= 0:
                raise ValueError(
                    "original pitch process-exit render dialog handle is invalid"
                )
            if (
                not isinstance(runtime_id, list)
                or not runtime_id
                or any(type(value) is not int for value in runtime_id)
            ):
                raise ValueError(
                    "original pitch process-exit render dialog identity is invalid"
                )
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
    renderer_build_identity = candidate["renderer_build_identity"]
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
        "candidate_renderer_executable_sha256": renderer_build_identity[
            "renderer_executable_sha256"
        ],
        "candidate_build_input_manifest_sha256": renderer_build_identity["sha256"],
        "candidate_build_input_git_head": renderer_build_identity["git_head"],
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

    bm = sub.add_parser("build-manifest")
    bm.add_argument("root", type=Path)
    c = sub.add_parser("candidate")
    c.add_argument("root", type=Path)
    cc = sub.add_parser("candidate-check")
    cc.add_argument("root", type=Path)
    ccp = sub.add_parser("candidate-check-portable")
    ccp.add_argument("root", type=Path)
    o = sub.add_parser("original-check")
    o.add_argument("candidate_root", type=Path)
    o.add_argument("original_root", type=Path)
    comp = sub.add_parser("compare")
    comp.add_argument("candidate_root", type=Path)
    comp.add_argument("original_root", type=Path)
    comp.add_argument("output", type=Path)

    args = parser.parse_args()
    if args.mode == "build-manifest":
        value = write_renderer_build_manifest(args.root)
    elif args.mode == "candidate":
        value = collect_candidate(args.root)
        target = args.root / CANDIDATE_RECEIPT
        write_new(target, value)
    elif args.mode == "candidate-check":
        value = validate_candidate(args.root)
    elif args.mode == "candidate-check-portable":
        value = validate_candidate(args.root, verify_checkout_sources=False)
    elif args.mode == "original-check":
        value = original_observation(args.candidate_root, args.original_root)
    else:
        candidate = validate_candidate(
            args.candidate_root, verify_checkout_sources=False
        )
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
