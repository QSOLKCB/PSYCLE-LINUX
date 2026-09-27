#!/usr/bin/env python3
"""Negative controls for the Phase 6C Sampler PS1 pitch witness."""
from __future__ import annotations

import importlib.util
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


hybrid = compat.convert(synthetic_psy3())
info = compat.inspect_hybrid(hybrid)
assert info["authored_sample_rate"] == 22050
assert info["authored_sample_frames"] == 11025
assert info["pcm_payload_identity"] == "byte-identical-compressed-left-channel"

try:
    compat.convert(synthetic_psy3(rate=44100))
except ValueError as exc:
    assert "modern SMSB sample identity changed" in str(exc)
else:
    raise AssertionError("44.1-kHz sample must not satisfy non-44.1 witness")

prefix, chunks = compat.parse_chunks(hybrid)
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
    compat.inspect_hybrid(bytes(mutated))
except ValueError as exc:
    assert "PCM payloads differ" in str(exc)
else:
    raise AssertionError("mismatched legacy/modern PCM must fail")


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

candidate = {
    "fixture_sha256": "a" * 64,
    "runtime_pitch_observation": "fixed-44100-basis-compatible-duration",
    "renders": [{"analysis": {"active_span_frames": 11025}}],
}
original = {
    "outcome": "rendered-twice",
    "analysis": {"active_span_frames": 22050},
}
comparison = pitch.compare_observations(candidate, original)
assert comparison["scoped_pitch_status"] == "DIFFERENT"
assert comparison["comparison_ready"] is True
assert comparison["sampler_ps1_status"] == "UNKNOWN"

blocked = pitch.compare_observations(
    candidate,
    {
        "outcome": "reference-process-exited-during-render",
        "process_exit_code": 0xC0000005,
    },
)
assert blocked["scoped_pitch_status"] == "UNKNOWN"
assert blocked["comparison_ready"] is False

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
