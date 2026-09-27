#!/usr/bin/env python3
"""Negative controls for the Phase 6C Sampler PS1 pitch witness."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import wave

ROOT = Path(__file__).resolve().parents[1]


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


compat = load(
    "phase6c_sampler_ps1_pitch_compat",
    ROOT / "scripts/phase6c-sampler-ps1-pitch-compat.py",
)
pitch = load(
    "phase6c_sampler_ps1_pitch",
    ROOT / "scripts/phase6c-sampler-ps1-pitch.py",
)


def modern_sample_body(rate: int = 22050, frames: int = 11025) -> bytes:
    packed = b"packed-identical-audio"
    body = bytearray()
    body += b"Phase 6C test\0"
    body += struct.pack("<IfH", frames, 1.0, 128)
    body += struct.pack("<IIi", 0, 0, 0)
    body += struct.pack("<IIi", 0, 0, 0)
    body += struct.pack("<Ihh", rate, 0, 0)
    body += struct.pack("<BBf", 0, 0, 0.5)
    body += struct.pack("<BBBBB", 0, 0, 0, 0, 0)
    body += struct.pack("<I", len(packed)) + packed
    return bytes(body)


def insd_payload() -> bytes:
    body = bytearray()
    body += struct.pack("<i", 0)
    body += struct.pack("<B", 0)
    body += struct.pack("<iB", 16, 0)
    body += struct.pack("<" + "i" * 12, *([1] * 12))
    body += struct.pack("<iBBB", 128, 0, 0, 0)
    body += b"\0"
    body += struct.pack("<i", 0)
    body += struct.pack("<ii", -1, 0)
    return bytes(body)


def synthetic_psy3(rate: int = 22050) -> bytes:
    sample_body = modern_sample_body(rate=rate)
    smsb = (
        struct.pack("<I", 0)
        + sample_body
        + struct.pack("<BI", 0, 0)
    )
    chunks = [
        (b"INSD", 2, insd_payload()),
        (b"SMSB", 2, smsb),
    ]
    prefix = bytearray(b"PSY3SONG")
    prefix += struct.pack("<I", 10)
    prefix += struct.pack("<I", 4)
    prefix += struct.pack("<I", len(chunks))
    output = bytearray(prefix)
    for fourcc, version, payload in chunks:
        output += fourcc + struct.pack("<II", version, len(payload)) + payload
    return bytes(output)


original_fixture = synthetic_psy3()
candidate_fixture = compat.convert(original_fixture)
info = compat.inspect_pair(original_fixture, candidate_fixture)
assert info["authored_sample_rate"] == 22050
assert info["authored_sample_frames"] == 11025
assert info["pcm_payload_identity"] == "byte-identical-compressed-left-channel"
assert info["candidate_modern_sample_chunk_removed"] is True

_original_prefix, original_chunks = compat.parse_chunks(original_fixture)
_candidate_prefix, candidate_chunks = compat.parse_chunks(candidate_fixture)
assert any(fourcc == b"SMSB" for fourcc, _version, _payload in original_chunks)
assert not any(fourcc == b"SMSB" for fourcc, _version, _payload in candidate_chunks)

try:
    compat.convert(synthetic_psy3(rate=44100))
except ValueError as exc:
    assert "modern SMSB sample identity changed" in str(exc)
else:
    raise AssertionError("44.1-kHz sample must not satisfy non-44.1 witness")

prefix, chunks = compat.parse_chunks(candidate_fixture)
mutated = bytearray(prefix)
for fourcc, version, payload in chunks:
    if fourcc == b"INSD":
        payload = payload.replace(
            b"packed-identical-audio",
            b"packed-different-audio",
            1,
        )
    mutated += fourcc + struct.pack("<II", version, len(payload)) + payload
try:
    compat.inspect_pair(original_fixture, bytes(mutated))
except ValueError as exc:
    assert "PCM payloads differ" in str(exc)
else:
    raise AssertionError("mismatched candidate/original PCM must fail")


def write_wave(path: Path, active_frames: int, total_frames: int = 44100) -> None:
    values = [12000] * active_frames + [0] * (total_frames - active_frames)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(44100)
        handle.writeframes(struct.pack("<" + "h" * len(values), *values))


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    candidate_wave = root / "candidate.wav"
    original_wave = root / "original.wav"
    write_wave(candidate_wave, 11025)
    write_wave(original_wave, 22050)
    candidate_analysis = pitch.analyze_wave(candidate_wave)
    original_analysis = pitch.analyze_wave(original_wave)
    assert candidate_analysis["active_span_frames"] == 11025
    assert original_analysis["active_span_frames"] == 22050

candidate_render_sha = "c" * 64
original_render_sha = "d" * 64
candidate = {
    "fixture_sha256": "a" * 64,
    "candidate_fixture_sha256": "b" * 64,
    "snapshot": pitch.CANDIDATE_SNAPSHOT,
    "renderer_provenance": {
        "path": pitch.RENDERER_PROVENANCE,
        "sha256": "e" * 64,
        "attestation": {
            "schema_version": 1,
            "renderer_source_sha256": "1" * 64,
            "renderer_project_sha256": "2" * 64,
            "engine_sequencer_sha256": "3" * 64,
            "engine_psy3_loader_sha256": "4" * 64,
            "engine_sampler_sha256": "5" * 64,
        },
    },
    "fixture_semantics": {
        "pcm_payload_identity": "byte-identical-compressed-left-channel",
    },
    "runtime_pitch_observation": "fixed-44100-basis-compatible-duration",
    "renders": [
        {"sha256": candidate_render_sha, "analysis": {"active_span_frames": 11025}},
        {"sha256": candidate_render_sha, "analysis": {"active_span_frames": 11025}},
    ],
}
original_identity = {
    "reference_build": pitch.REFERENCE_BUILD,
    "reference_file": pitch.REFERENCE_FILE,
    "reference_installer_sha256": pitch.REFERENCE_INSTALLER_SHA256,
    "reference_installer_size_bytes": pitch.REFERENCE_INSTALLER_SIZE_BYTES,
    "reference_executable_sha256": pitch.REFERENCE_EXECUTABLE_SHA256,
    "receipt_sha256": "f" * 64,
}
original = {
    "outcome": "rendered-twice",
    "identity": original_identity,
    "render_sha256s": [original_render_sha, original_render_sha],
    "analysis": {"active_span_frames": 22050},
}
comparison = pitch.compare_observations(candidate, original)
assert comparison["scoped_pitch_status"] == "DIFFERENT"
assert comparison["comparison_ready"] is True
assert comparison["sampler_ps1_status"] == "UNKNOWN"
assert comparison["candidate_snapshot"] == pitch.CANDIDATE_SNAPSHOT
assert comparison["candidate_render_sha256s"] == [candidate_render_sha] * 2
assert comparison["original_reference_build"] == pitch.REFERENCE_BUILD
assert comparison["original_executable_sha256"] == pitch.REFERENCE_EXECUTABLE_SHA256
assert comparison["original_render_sha256s"] == [original_render_sha] * 2

blocked = pitch.compare_observations(
    candidate,
    {
        "outcome": "reference-process-exited-during-render",
        "identity": original_identity,
        "render_sha256s": [],
        "process_exit_code": 0xC0000005,
    },
)
assert blocked["scoped_pitch_status"] == "UNKNOWN"
assert blocked["comparison_ready"] is False

with tempfile.TemporaryDirectory() as temporary:
    evidence = Path(temporary)
    provenance = pitch.expected_renderer_provenance()
    provenance_path = evidence / pitch.RENDERER_PROVENANCE
    provenance_path.write_text(
        json.dumps(provenance, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    validated = pitch.validate_renderer_provenance(evidence)
    assert validated["attestation"] == provenance
    assert validated["sha256"] == pitch.sha256(provenance_path)

    corrupted = dict(provenance)
    corrupted["engine_sampler_sha256"] = "0" * 64
    provenance_path.write_text(
        json.dumps(corrupted, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    try:
        pitch.validate_renderer_provenance(evidence)
    except ValueError as exc:
        assert "does not match checked-out" in str(exc)
    else:
        raise AssertionError("modified compiled-renderer provenance must fail")

with tempfile.TemporaryDirectory() as temporary:
    evidence = Path(temporary)
    render_dir = evidence / "sampler-ps1-pitch"
    render_dir.mkdir()
    render_path = render_dir / "original-sampler-ps1-pitch-1.wav"
    write_wave(render_path, 22050)
    render_sha = pitch.sha256(render_path)
    attempt = {
        "schema_version": 1,
        "outcome": "rendered",
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": True,
        "save_invoked": True,
        "stable_output_polls": 4,
        "dialog_closed": True,
        "close_control_seen": True,
        "preexisting_render_dialog_count": 0,
        "render_dialog_native_event_hook_armed": True,
        "render_dialog_event_message_pump_started": True,
        "render_dialog_dispatch_boundary_set": True,
        "render_dialog_post_dispatch_observed_window_event_count": 1,
        "render_dialog_unresolved_post_dispatch_event_count": 0,
        "render_dialog_post_dispatch_event_count": 1,
        "selected_render_dialog_native_handle": 1234,
        "selected_render_dialog_runtime_id": [42, 7],
        "dialog_discovery": "pumped-win-event-object-show-strictly-after-dispatch-tick",
        "process_exited": False,
        "process_exit_code": None,
        "output": {
            "path": render_path.name,
            "sha256": render_sha,
        },
        "observed_output": {
            "path": render_path.name,
            "size_bytes": render_path.stat().st_size,
            "sha256": render_sha,
        },
        "diagnostics": [],
    }
    pitch.validate_completed_render_attempt(
        attempt,
        "sampler-ps1-pitch/original-sampler-ps1-pitch-1.wav",
        render_sha,
        evidence,
    )

    weakened = dict(attempt)
    weakened["controls_configured"] = False
    try:
        pitch.validate_completed_render_attempt(
            weakened,
            "sampler-ps1-pitch/original-sampler-ps1-pitch-1.wav",
            render_sha,
            evidence,
        )
    except ValueError as exc:
        assert "controls_configured" in str(exc)
    else:
        raise AssertionError("unconfigured original render controls must fail")

    diagnostic = dict(attempt)
    diagnostic["diagnostics"] = ["ambiguous dialog attribution"]
    try:
        pitch.validate_completed_render_attempt(
            diagnostic,
            "sampler-ps1-pitch/original-sampler-ps1-pitch-1.wav",
            render_sha,
            evidence,
        )
    except ValueError as exc:
        assert "retained diagnostics" in str(exc)
    else:
        raise AssertionError("diagnostic-bearing original render must fail")

print("phase6c-sampler-ps1-pitch: PASS")


with tempfile.TemporaryDirectory() as temporary:
    generated = Path(temporary) / "observer.ps1"
    result = subprocess.run(
        [
            "python3",
            str(ROOT / "scripts/phase6c-build-sampler-ps1-pitch-observer.py"),
            str(ROOT / "scripts/phase6c-original-windows-fixtures-v2.ps1"),
            str(generated),
        ],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    observer = generated.read_text(encoding="utf-8")
    assert "[switch]$ObserveSamplerPs1Pitch" in observer
    assert 'name = "sampler-ps1-pitch"' in observer
    assert "Invoke-Phase6cAudioRender" in observer
    assert "runtime_execution = $runtimeExecution" in observer

print("phase6c-sampler-ps1-pitch-observer: PASS")
