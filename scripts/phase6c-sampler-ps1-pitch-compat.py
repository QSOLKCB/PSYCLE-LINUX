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


def sound_desquash_pcm16le(packed: bytes) -> bytes:
    """Decode Psycle SoundSquash v1 without depending on either runtime."""
    if len(packed) < 5 or packed[0] != 0x01:
        raise ValueError("SMSB left sample is not SoundSquash v1")
    frames = int.from_bytes(packed[1:5], "little")
    if frames != AUTHORED_SAMPLE_FRAMES:
        raise ValueError("SoundSquash frame count changed")

    source = packed + b"\0\0\0\0"
    cursor = 5
    bitpos = 0
    prevprev = 0
    prev = 0
    values: list[int] = []
    masks = tuple((1 << bits) - 1 for bits in range(16))

    for _ in range(frames):
        if cursor + 4 > len(source):
            raise ValueError("truncated SoundSquash bitstream")
        bits = int.from_bytes(source[cursor : cursor + 4], "little") >> bitpos
        numbits = bits & 0x0F
        negative = (bits & 0x10) != 0
        magnitude = (bits >> 5) & masks[numbits]
        if negative:
            error = magnitude | ((0xFFFF << numbits) & 0xFFFF)
        else:
            error = magnitude
        if error & 0x8000:
            error -= 0x10000

        sample = (prev + (prev - prevprev) + error) & 0xFFFF
        values.append(sample)
        signed = sample if sample < 0x8000 else sample - 0x10000
        prevprev = prev
        prev = signed

        bitpos += numbits + 5
        cursor += bitpos // 8
        bitpos %= 8

    return b"".join(struct.pack("<H", value) for value in values)


def inspect_original(data: bytes) -> dict:
    _prefix, chunks = parse_chunks(data)
    insd = [item for item in chunks if item[0] == b"INSD"]
    smsb = [item for item in chunks if item[0] == b"SMSB"]
    if len(insd) != 1 or smsb:
        raise ValueError("candidate witness must contain one INSD and no SMSB")
    if insd[0][1] != 2:
        raise ValueError("candidate INSD chunk version changed")

    # The candidate load fixture differs only by omission of the unsupported
    # modern SMSB chunk. Its INSD remains the exact original instrument body
    # with numwaves=0; the candidate harness installs the separately bound PCM.
    expected_chunks = [item for item in original_chunks if item[0] != b"SMSB"]
    if chunks != expected_chunks:
        raise ValueError("candidate bridge changed PSY3 bytes beyond removing SMSB")
    if bytes(_prefix) != bytes(original_prefix):
        candidate_prefix = bytearray(_prefix)
        source_prefix = bytearray(original_prefix)
        struct.pack_into("<I", candidate_prefix, 16, 0)
        struct.pack_into("<I", source_prefix, 16, 0)
        if candidate_prefix != source_prefix:
            raise ValueError("candidate bridge changed PSY3 SONG metadata")

    payload = insd[0][2]
    offset = 4 + 1 + 4 + 1 + 12 * 4 + 4 + 3
    _name, offset = read_cstring(payload, offset)
    if offset + 4 > len(payload):
        raise ValueError("candidate INSD numwaves field is missing")
    if struct.unpack_from("<i", payload, offset)[0] != 0:
        raise ValueError("candidate INSD must remain wave-empty before harness injection")

    decoded = sound_desquash_pcm16le(sample["packed_left"])
    if pcm16le is not None and pcm16le != decoded:
        raise ValueError("candidate PCM sidecar differs from original SMSB audio")

    return {
        "authored_sample_rate": sample["sample_rate"],
        "authored_sample_frames": sample["frames"],
        "pcm_payload_identity": "decoded-SMSB-pcm16le-identical-sidecar",
        "pcm16le_bytes": len(decoded),
        "original_metadata_source": "SMSB",
        "candidate_audio_source": "hash-bound-harness-injected-pcm16le",
        "candidate_sample_rate_metadata": "not-imported-into-legacy-Instrument",
        "candidate_pre_injection_wave_state": "empty",
        "candidate_modern_sample_chunk_removed": True,
        "non_sample_psy3_chunks": "byte-identical",
    }


def inspect_pair(
    original: bytes, candidate: bytes, pcm16le: bytes | None = None
) -> dict:
    inspect_original(original)
    return inspect_candidate(candidate, original, pcm16le)

def inspect_pair(original: bytes, candidate: bytes) -> dict:
    inspect_original(original)
    return inspect_candidate(candidate, original)

def convert(data: bytes) -> bytes:
    prefix, chunks = parse_chunks(data)
    insd = [item for item in chunks if item[0] == b"INSD"]
    smsb = [item for item in chunks if item[0] == b"SMSB"]
    if len(insd) != 1 or len(smsb) != 1:
        raise ValueError("expected exactly one INSD and one SMSB chunk")
    if insd[0][1] != 2 or smsb[0][1] != 2:
        raise ValueError("unexpected modern INSD/SMSB version")

    parse_modern_sample(smsb[0][2])
    rebuilt = [item for item in chunks if item[0] != b"SMSB"]
    struct.pack_into("<I", prefix, 16, len(rebuilt))
    output = bytearray(prefix)
    for fourcc, version, payload in rebuilt:
        output += fourcc + u32(version) + u32(len(payload)) + payload
    inspect_pair(data, bytes(output))
    return bytes(output)


def extract_pcm16le(data: bytes) -> bytes:
    return sound_desquash_pcm16le(inspect_original(data)["sample"]["packed_left"])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("convert", "check"))
    parser.add_argument("original", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("pcm16le", type=Path)
    args = parser.parse_args()

    source = args.original.read_bytes()
    if args.mode == "convert":
        if args.candidate.exists() or args.pcm16le.exists():
            raise SystemExit("refusing existing candidate fixture or PCM sidecar")
        converted = convert(source)
        pcm = extract_pcm16le(source)
        args.candidate.parent.mkdir(parents=True, exist_ok=True)
        args.candidate.write_bytes(converted)
        args.pcm16le.write_bytes(pcm)
    else:
        converted = args.candidate.read_bytes()
        pcm = args.pcm16le.read_bytes()

    info = inspect_pair(source, converted, pcm)
    print(
        "phase6c-sampler-ps1-pitch-compat: PASS "
        f"sample_rate={info['authored_sample_rate']} "
        f"frames={info['authored_sample_frames']} "
        f"pcm={info['pcm_payload_identity']} "
        f"pcm_bytes={info['pcm16le_bytes']} "
        f"candidate_smsb_removed={info['candidate_modern_sample_chunk_removed']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
