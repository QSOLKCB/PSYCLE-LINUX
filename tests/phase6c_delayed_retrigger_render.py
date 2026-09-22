#!/usr/bin/env python3
"""Negative controls for the Phase 6C offline-render execution analyzer."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "phase6c-delayed-retrigger-render-evidence.py"

spec = importlib.util.spec_from_file_location("phase6c_render_evidence", SCRIPT)
assert spec is not None and spec.loader is not None
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def wave_pcm16(frames: list[int], channels: int = 1, rate: int = 44100) -> bytes:
    if channels == 1:
        payload = struct.pack("<" + "h" * len(frames), *frames)
    else:
        interleaved = []
        for frame in frames:
            interleaved.extend([frame] * channels)
        payload = struct.pack("<" + "h" * len(interleaved), *interleaved)
    block_align = channels * 2
    byte_rate = rate * block_align
    fmt = struct.pack("<HHIIHH", 1, channels, rate, byte_rate, block_align, 16)
    return (
        b"RIFF"
        + struct.pack("<I", 4 + (8 + len(fmt)) + (8 + len(payload)))
        + b"WAVE"
        + b"fmt "
        + struct.pack("<I", len(fmt))
        + fmt
        + b"data"
        + struct.pack("<I", len(payload))
        + payload
    )


def witness_frames(onset_beats: list[float]) -> list[int]:
    beat_frames = 44100 * 60.0 / 137.0
    length = int(4.5 * beat_frames)
    frames = [0] * length
    for beat in onset_beats:
        start = int(round(beat * beat_frames))
        frames[start : start + 4] = [16000, -16000, 8000, -8000]
    return frames


def expect_failure(data: bytes, phrase: str) -> None:
    try:
        module.analyze_wave(data)
    except ValueError as exc:
        if phrase not in str(exc):
            raise AssertionError(f"unexpected failure: {exc}") from exc
    else:
        raise AssertionError("expected analyzer failure")


valid = wave_pcm16(
    witness_frames([0.0625, 1.0, 1.0625, 1.125, 2.0, 2.0625, 3.125])
)
analysis = module.analyze_wave(valid)
assert analysis["window_onset_counts"]["retrigger_beat_1"] == 3
assert analysis["window_onset_counts"]["retr_cont_beat_2"] == 2
assert len(analysis["onset_frames"]) == 7

expect_failure(
    wave_pcm16(witness_frames([0.0625, 1.0, 2.0, 3.125])),
    "fewer than five",
)
expect_failure(
    wave_pcm16(witness_frames([0.0625, 1.0, 1.0625, 2.0, 3.125])),
    "retrigger and retrigger-continue",
)
expect_failure(
    wave_pcm16(
        witness_frames([0.0625, 1.0, 1.0625, 2.0, 2.0625, 3.125]),
        channels=2,
    ),
    "must be mono",
)

print("phase6c-delayed-retrigger-render: PASS")
