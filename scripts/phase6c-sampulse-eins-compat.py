#!/usr/bin/env python3
"""Translate C-Psycle Sampulse SMID/SMSB state into historical EINS v1.0."""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

EINS_VERSION = 0x00010000
FILTER_NONE = 4
NOTE_MAP_SIZE = 120


def u32(value: int) -> bytes:
    return struct.pack("<I", value)


def i32(value: int) -> bytes:
    return struct.pack("<i", value)


def cstring(value: str) -> bytes:
    return value.encode("utf-8") + b"\0"


def envelope_disabled() -> bytes:
    return (
        b"\0"  # enabled
        + b"\0"  # carry
        + i32(-1) * 4
        + u32(0)  # no points
    )


def historical_instrument() -> bytes:
    body = bytearray()
    body += cstring("Phase 6C click instrument")
    body += struct.pack("<H", 16)  # lines
    body += struct.pack("<f", 1.0)  # global volume
    body += struct.pack("<f", 0.0)  # fade speed
    body += struct.pack("<f", 0.5)  # initial pan
    body += b"\0"  # pan disabled
    body += struct.pack("<B", 60)  # pan center
    body += struct.pack("<b", 0)  # pan separation
    body += struct.pack("<B", 127)  # cutoff
    body += struct.pack("<B", 0)  # resonance
    body += struct.pack("<h", 0)  # filter env amount
    body += u32(FILTER_NONE)
    body += struct.pack("<ffff", 0.0, 0.0, 0.0, 0.0)
    body += u32(0) * 3  # NNA stop, DCT none, DCA stop
    for note in range(NOTE_MAP_SIZE):
        body += struct.pack("<BB", note, 0)
    body += envelope_disabled() * 4
    total = 8 + len(body)
    return b"INST" + u32(total) + bytes(body)


def parse_chunks(data: bytes) -> tuple[bytearray, list[tuple[bytes, int, bytes]]]:
    if len(data) < 20 or data[:8] != b"PSY3SONG":
        raise ValueError("Sampulse witness is not PSY3")
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


def convert(data: bytes) -> bytes:
    prefix, chunks = parse_chunks(data)
    smid = [item for item in chunks if item[0] == b"SMID"]
    smsb = [item for item in chunks if item[0] == b"SMSB"]
    if len(smid) != 1 or len(smsb) != 1:
        raise ValueError("expected exactly one SMID and one SMSB chunk")
    if smid[0][1] != 2 or smsb[0][1] != 2:
        raise ValueError("unexpected modern Sampulse chunk version")

    smid_payload = smid[0][2]
    smsb_payload = smsb[0][2]
    if len(smid_payload) < 9 or len(smsb_payload) < 10:
        raise ValueError("Sampulse state chunk is too short")
    if struct.unpack_from("<I", smid_payload, 0)[0] != 0:
        raise ValueError("expected Sampulse instrument index 0")
    if smid_payload[-5] != 0 or struct.unpack_from("<I", smid_payload, len(smid_payload) - 4)[0] != 1:
        raise ValueError("expected Sampulse instrument group 1")
    if struct.unpack_from("<I", smsb_payload, 0)[0] != 0:
        raise ValueError("expected Sampulse sample index 0")
    if smsb_payload[-5] != 0 or struct.unpack_from("<I", smsb_payload, len(smsb_payload) - 4)[0] != 0:
        raise ValueError("expected Sampulse sample subindex 0")

    # SMSB v2 is: index + the historical sample body + revert/subindex.
    sample_body = smsb_payload[4:-5]
    sample_total = 12 + len(sample_body)
    historical_sample = b"SMPD" + u32(sample_total) + u32(1) + sample_body

    eins_payload = (
        u32(1)
        + i32(0)
        + historical_instrument()
        + u32(1)
        + i32(0)
        + historical_sample
    )
    eins = (b"EINS", EINS_VERSION, eins_payload)

    rebuilt: list[tuple[bytes, int, bytes]] = []
    inserted = False
    for fourcc, version, payload in chunks:
        if fourcc in {b"SMID", b"SMSB"}:
            if not inserted:
                rebuilt.append(eins)
                inserted = True
            continue
        rebuilt.append((fourcc, version, payload))
    if not inserted:
        raise ValueError("Sampulse state replacement was not inserted")

    struct.pack_into("<I", prefix, 16, len(rebuilt))
    output = bytearray(prefix)
    for fourcc, version, payload in rebuilt:
        output += fourcc
        output += u32(version)
        output += u32(len(payload))
        output += payload

    check_prefix, check_chunks = parse_chunks(bytes(output))
    del check_prefix
    ids = [item[0] for item in check_chunks]
    if ids.count(b"EINS") != 1 or b"SMID" in ids or b"SMSB" in ids:
        raise ValueError("historical Sampulse chunk rewrite failed")
    return bytes(output)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = args.source.read_bytes()
    converted = convert(source)
    if args.output.exists():
        raise SystemExit(f"refusing existing output: {args.output}")
    args.output.write_bytes(converted)
    print(
        f"phase6c-sampulse-eins-compat: PASS "
        f"input_bytes={len(source)} output_bytes={len(converted)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
