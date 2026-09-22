#!/usr/bin/env python3
"""Build and validate the Phase 6C original-Psycle execution witness."""
from __future__ import annotations

import argparse
import hashlib
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
    offset = 12
    fmt = None
    payload = None
    while offset + 8 <= len(data):
        chunk = data[offset : offset + 4]
        size = struct.unpack_from("<I", data, offset + 4)[0]
        start = offset + 8
        end = start + size
        if end > len(data):
            raise ValueError("truncated WAV chunk")
        if chunk == b"fmt ":
            fmt = data[start:end]
        elif chunk == b"data":
            payload = data[start:end]
        offset = end + (size & 1)
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


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    candidate = validate_candidate(candidate_root)
    receipt = read_json(original_root / ORIGINAL_RECEIPT)
    checks = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "original-observation",
        "contract": CONTRACT,
        "evidence_role": "original",
        "reference_build": REFERENCE_BUILD,
        "candidate_fixture": FIXTURE,
        "fixture_sha256": candidate["fixture_sha256"],
        "load_result": "accepted",
        "original_psycle_observed": True,
        "parity_status": "UNKNOWN",
    }
    for key, value in checks.items():
        if receipt.get(key) != value:
            raise ValueError("original execution witness identity mismatch: " + key)

    runtime = receipt.get("runtime_execution")
    if (
        not isinstance(runtime, dict)
        or runtime.get("schema_version") != 1
        or runtime.get("outcome") != "rendered-twice"
        or runtime.get("deterministic") is not True
        or runtime.get("settings") != {
            "sample_rate": 44100,
            "bits_per_sample": 16,
            "channels": "mono-mix",
            "dither": False,
            "range": "entire-song",
        }
    ):
        raise ValueError("original runtime execution receipt is incomplete or inconclusive")
    renders = runtime.get("renders")
    if not isinstance(renders, list) or len(renders) != 2:
        raise ValueError("expected exactly two original offline renders")
    first = bound_bytes(original_root, renders[0])
    second = bound_bytes(original_root, renders[1])
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
