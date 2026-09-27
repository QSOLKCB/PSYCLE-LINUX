#!/usr/bin/env python3
"""Build and validate the dual-generation PSY3 Sampler PS1 pitch witness."""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

AUTHORED_SAMPLE_RATE = 22050
AUTHORED_SAMPLE_FRAMES = 11025
AUTHORED_NOTE = 60


def u32(value: int) -> bytes:
    return struct.pack("<I", value)


def i32(value: int) -> bytes:
    return struct.pack("<i", value)


def cstring(value: str) -> bytes:
    return value.encode("utf-8") + b"\0"


def parse_chunks(data: bytes) -> tuple[bytearray, list[tuple[bytes, int, bytes]]]:
    if len(data) < 20 or data[:8] != b"PSY3SONG":
        raise ValueError("Sampler pitch witness is not PSY3")
    song_size = struct.unpack_from("<I", data, 12)[0]
    count = struct.unpack_from("<I", data, 16)[0]
    chunk_start = 16 + song_size
    if chunk_start > len(data):
        raise ValueError("PSY3 SONG header size is invalid")
    prefix = bytearray(data[:chunk_start])
    chunks: list[tuple[bytes, int, bytes]] = []
    offset = chunk_start
    for _ in range(count):
        if offset + 12 > len(data):
            raise ValueError("truncated PSY3 chunk header")
        fourcc = data[offset : offset + 4]
        version, size = struct.unpack_from("<II", data, offset + 4)
        end = offset + 12 + size
        if end > len(data):
            raise ValueError("truncated PSY3 chunk payload")
        chunks.append((fourcc, version, data[offset + 12 : end]))
        offset = end
    if offset != len(data):
        raise ValueError("trailing bytes after PSY3 chunk table")
    return prefix, chunks


def read_cstring(data: bytes, offset: int) -> tuple[str, int]:
    end = data.find(b"\0", offset)
    if end < 0:
        raise ValueError("unterminated PSY3 string")
    return data[offset:end].decode("utf-8"), end + 1


def parse_modern_sample(payload: bytes) -> dict:
    if len(payload) < 16:
        raise ValueError("SMSB payload is too short")
    index = struct.unpack_from("<I", payload, 0)[0]
    if index != 0:
        raise ValueError("expected SMSB sample index 0")
    if payload[-5] != 0 or struct.unpack_from("<I", payload, len(payload) - 4)[0] != 0:
        raise ValueError("expected SMSB sample group/subindex 0")
    body = payload[4:-5]
    offset = 0
    name, offset = read_cstring(body, offset)

    def take(fmt: str):
        nonlocal offset
        size = struct.calcsize(fmt)
        if offset + size > len(body):
            raise ValueError("truncated SMSB sample body")
        values = struct.unpack_from(fmt, body, offset)
        offset += size
        return values[0] if len(values) == 1 else values

    frames = take("<I")
    global_volume = take("<f")
    default_volume = take("<H")
    loop_start = take("<I")
    loop_end = take("<I")
    loop_type = take("<i")
    sustain_start = take("<I")
    sustain_end = take("<I")
    sustain_type = take("<i")
    sample_rate = take("<I")
    tune = take("<h")
    finetune = take("<h")
    stereo = take("<B")
    pan_enabled = take("<B")
    panning = take("<f")
    surround = take("<B")
    vibrato_attack = take("<B")
    vibrato_speed = take("<B")
    vibrato_depth = take("<B")
    vibrato_type = take("<B")
    packed_left_size = take("<I")
    if offset + packed_left_size > len(body):
        raise ValueError("truncated SMSB left sample data")
    packed_left = body[offset : offset + packed_left_size]
    offset += packed_left_size
    packed_right = None
    if stereo:
        packed_right_size = take("<I")
        if offset + packed_right_size > len(body):
            raise ValueError("truncated SMSB right sample data")
        packed_right = body[offset : offset + packed_right_size]
        offset += packed_right_size
    if offset != len(body):
        raise ValueError("unexpected trailing SMSB sample bytes")

    result = {
        "name": name,
        "frames": frames,
        "global_volume": global_volume,
        "default_volume": default_volume,
        "loop_start": loop_start,
        "loop_end": loop_end,
        "loop_type": loop_type,
        "sustain_start": sustain_start,
        "sustain_end": sustain_end,
        "sustain_type": sustain_type,
        "sample_rate": sample_rate,
        "tune": tune,
        "finetune": finetune,
        "stereo": stereo,
        "pan_enabled": pan_enabled,
        "panning": panning,
        "surround": surround,
        "vibrato_attack": vibrato_attack,
        "vibrato_speed": vibrato_speed,
        "vibrato_depth": vibrato_depth,
        "vibrato_type": vibrato_type,
        "packed_left": packed_left,
        "packed_right": packed_right,
    }
    if (
        frames != AUTHORED_SAMPLE_FRAMES
        or sample_rate != AUTHORED_SAMPLE_RATE
        or tune != 0
        or finetune != 0
        or stereo != 0
        or loop_type != 0
    ):
        raise ValueError("modern SMSB sample identity changed")
    return result


def legacy_wave_chunk(sample: dict) -> bytes:
    payload = bytearray()
    payload += u32(0)  # legacy wave index
    payload += u32(sample["frames"])
    payload += struct.pack("<H", 100)  # 100 -> 1.0 legacy global volume
    payload += u32(sample["loop_start"])
    payload += u32(sample["loop_end"])
    payload += i32(sample["tune"])
    payload += i32(0)  # legacy +/-256 finetune units; witness is exactly zero
    payload += struct.pack("<B", 1 if sample["loop_type"] == 1 else 0)
    payload += struct.pack("<B", sample["stereo"])
    payload += cstring(sample["name"])
    payload += u32(len(sample["packed_left"]))
    payload += sample["packed_left"]
    if sample["stereo"]:
        assert sample["packed_right"] is not None
        payload += u32(len(sample["packed_right"]))
        payload += sample["packed_right"]
    return b"WAVE" + u32(0) + u32(len(payload)) + bytes(payload)


def augment_insd(payload: bytes, wave: bytes) -> bytes:
    if len(payload) < 78:
        raise ValueError("INSD payload is too short")
    if struct.unpack_from("<i", payload, 0)[0] != 0:
        raise ValueError("expected INSD instrument index 0")

    offset = 4
    offset += 1  # loop bool
    offset += 4  # lines
    offset += 1  # NNA
    offset += 12 * 4  # amplitude/filter integer fields
    offset += 4  # legacy pan
    offset += 3  # RPAN/RCUT/RRES
    _name, offset = read_cstring(payload, offset)
    if offset + 4 > len(payload):
        raise ValueError("INSD numwaves field is missing")
    numwaves = struct.unpack_from("<i", payload, offset)[0]
    if numwaves != 0:
        raise ValueError("modern INSD unexpectedly contains legacy waves")
    tail = payload[offset + 4 :]
    if len(tail) != 8:
        raise ValueError("modern INSD sampler-lock tail changed")
    return payload[:offset] + i32(1) + wave + tail


def inspect_original(data: bytes) -> dict:
    _prefix, chunks = parse_chunks(data)
    insd = [item for item in chunks if item[0] == b"INSD"]
    smsb = [item for item in chunks if item[0] == b"SMSB"]
    if len(insd) != 1 or len(smsb) != 1:
        raise ValueError("original witness must contain exactly one INSD and one SMSB chunk")
    if insd[0][1] != 2 or smsb[0][1] != 2:
        raise ValueError("unexpected original INSD/SMSB chunk version")
    sample = parse_modern_sample(smsb[0][2])
    return {
        "sample": sample,
        "chunk_ids": [item[0] for item in chunks],
    }


def inspect_candidate(data: bytes, original: bytes) -> dict:
    _prefix, chunks = parse_chunks(data)
    original_prefix, original_chunks = parse_chunks(original)
    original_info = inspect_original(original)
    sample = original_info["sample"]

    insd = [item for item in chunks if item[0] == b"INSD"]
    smsb = [item for item in chunks if item[0] == b"SMSB"]
    if len(insd) != 1 or smsb:
        raise ValueError("candidate witness must contain one legacy INSD and no SMSB")
    if insd[0][1] != 2:
        raise ValueError("candidate INSD chunk version changed")

    # Every non-sample chunk must remain byte-identical and in the same order.
    expected_other = [
        item for item in original_chunks if item[0] not in {b"INSD", b"SMSB"}
    ]
    actual_other = [item for item in chunks if item[0] != b"INSD"]
    if actual_other != expected_other:
        raise ValueError("candidate bridge changed non-sample PSY3 chunks")
    if bytes(_prefix) != bytes(original_prefix):
        # The only permitted prefix mutation is the top-level chunk count.
        candidate_prefix = bytearray(_prefix)
        source_prefix = bytearray(original_prefix)
        struct.pack_into("<I", candidate_prefix, 16, 0)
        struct.pack_into("<I", source_prefix, 16, 0)
        if candidate_prefix != source_prefix:
            raise ValueError("candidate bridge changed PSY3 SONG metadata")

    payload = insd[0][2]
    offset = 4 + 1 + 4 + 1 + 12 * 4 + 4 + 3
    _name, offset = read_cstring(payload, offset)
    numwaves = struct.unpack_from("<i", payload, offset)[0]
    offset += 4
    if numwaves != 1:
        raise ValueError("candidate INSD must contain exactly one legacy WAVE")
    if payload[offset : offset + 4] != b"WAVE":
        raise ValueError("candidate legacy WAVE header is missing")
    version, size = struct.unpack_from("<II", payload, offset + 4)
    if version != 0 or offset + 12 + size > len(payload):
        raise ValueError("candidate legacy WAVE chunk is invalid")
    wave_payload = payload[offset + 12 : offset + 12 + size]
    if struct.unpack_from("<I", wave_payload, 4)[0] != AUTHORED_SAMPLE_FRAMES:
        raise ValueError("candidate legacy WAVE frame count changed")

    # Legacy WAVE deliberately has no sample-rate field. Its packed PCM must be
    # byte-identical to the modern SMSB source that carries 22.05-kHz metadata.
    wave_cursor = 4 + 4 + 2 + 4 + 4 + 4 + 4 + 1 + 1
    _wave_name, wave_cursor = read_cstring(wave_payload, wave_cursor)
    packed_size = struct.unpack_from("<I", wave_payload, wave_cursor)[0]
    wave_cursor += 4
    packed = wave_payload[wave_cursor : wave_cursor + packed_size]
    if packed != sample["packed_left"]:
        raise ValueError("candidate WAVE and original SMSB PCM payloads differ")

    # The candidate INSD is the original legacy-compatible instrument body plus
    # one WAVE. The modern SMSB is intentionally absent so the r12005 loader
    # cannot scan unsupported compressed sample metadata as unknown chunks.
    original_insd = [item for item in original_chunks if item[0] == b"INSD"][0][2]
    original_offset = 4 + 1 + 4 + 1 + 12 * 4 + 4 + 3
    _original_name, original_offset = read_cstring(original_insd, original_offset)
    if struct.unpack_from("<i", original_insd, original_offset)[0] != 0:
        raise ValueError("original INSD unexpectedly contains legacy waves")
    original_tail = original_insd[original_offset + 4 :]
    candidate_tail = payload[offset + 12 + size :]
    if candidate_tail != original_tail:
        raise ValueError("candidate bridge changed the INSD sampler-lock tail")

    return {
        "authored_sample_rate": sample["sample_rate"],
        "authored_sample_frames": sample["frames"],
        "legacy_wave_frame_count": AUTHORED_SAMPLE_FRAMES,
        "pcm_payload_identity": "byte-identical-compressed-left-channel",
        "original_metadata_source": "SMSB",
        "candidate_audio_source": "INSD/WAVE",
        "candidate_sample_rate_metadata": "absent-from-legacy-WAVE",
        "candidate_modern_sample_chunk_removed": True,
        "non_sample_psy3_chunks": "byte-identical",
    }


def inspect_pair(original: bytes, candidate: bytes) -> dict:
    inspect_original(original)
    return inspect_candidate(candidate, original)

def convert(data: bytes) -> bytes:
    prefix, chunks = parse_chunks(data)
    insd_indexes = [i for i, item in enumerate(chunks) if item[0] == b"INSD"]
    smsb = [item for item in chunks if item[0] == b"SMSB"]
    if len(insd_indexes) != 1 or len(smsb) != 1:
        raise ValueError("expected exactly one INSD and one SMSB chunk")
    if chunks[insd_indexes[0]][1] != 2 or smsb[0][1] != 2:
        raise ValueError("unexpected modern INSD/SMSB version")

    sample = parse_modern_sample(smsb[0][2])
    wave = legacy_wave_chunk(sample)
    rebuilt: list[tuple[bytes, int, bytes]] = []
    for fourcc, version, payload in chunks:
        if fourcc == b"INSD":
            rebuilt.append((fourcc, version, augment_insd(payload, wave)))
        elif fourcc == b"SMSB":
            # r12005 has no SMSB handler. Do not leave the unsupported modern
            # compressed sample chunk in its chunk stream.
            continue
        else:
            rebuilt.append((fourcc, version, payload))

    struct.pack_into("<I", prefix, 16, len(rebuilt))
    output = bytearray(prefix)
    for fourcc, version, payload in rebuilt:
        output += fourcc + u32(version) + u32(len(payload)) + payload
    inspect_pair(data, bytes(output))
    return bytes(output)

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("convert", "check"))
    parser.add_argument("original", type=Path)
    parser.add_argument("candidate", type=Path)
    args = parser.parse_args()

    if args.mode == "convert":
        if args.candidate.exists():
            raise SystemExit(f"refusing existing output: {args.candidate}")
        source = args.original.read_bytes()
        converted = convert(source)
        args.candidate.parent.mkdir(parents=True, exist_ok=True)
        args.candidate.write_bytes(converted)
        info = inspect_pair(source, converted)
    else:
        info = inspect_pair(
            args.original.read_bytes(),
            args.candidate.read_bytes(),
        )

    print(
        "phase6c-sampler-ps1-pitch-compat: PASS "
        f"sample_rate={info['authored_sample_rate']} "
        f"frames={info['authored_sample_frames']} "
        f"pcm={info['pcm_payload_identity']} "
        f"candidate_smsb_removed={info['candidate_modern_sample_chunk_removed']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
