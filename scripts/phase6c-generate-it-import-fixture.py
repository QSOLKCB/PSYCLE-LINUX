#!/usr/bin/env python3
"""Generate the redistributable Phase 6C Impulse Tracker import fixture."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

SCHEMA_VERSION = 1
TITLE = "PSYCLE IT import witness"
EXPECTED_SHA256 = "f02f5b8d1de98d4c6bcf00e2d6a04617e0714e4bcd7e299b5bc56d057526cae1"
EXPECTED_SIZE = 4439


def p16(value: int) -> bytes:
    return struct.pack("<H", value)


def p32(value: int) -> bytes:
    return struct.pack("<I", value)


def event(channel: int, *, note=None, instrument=None, volume=None, command=None, parameter=0) -> bytes:
    mask = 0
    payload = bytearray()
    if note is not None:
        mask |= 0x01
        payload.append(note & 0xFF)
    if instrument is not None:
        mask |= 0x02
        payload.append(instrument & 0xFF)
    if volume is not None:
        mask |= 0x04
        payload.append(volume & 0xFF)
    if command is not None:
        mask |= 0x08
        payload.extend((command & 0xFF, parameter & 0xFF))
    return bytes((0x80 | ((channel & 0x3F) + 1), mask)) + payload


def row(*events: bytes) -> bytes:
    return b"".join(events) + b"\x00"


def build_fixture() -> bytes:
    rows = [
        row(event(0, note=60, instrument=1)),
        row(event(0, command=5, parameter=0x10)),
        row(event(0, command=6, parameter=0x10)),
        row(event(0, note=64, instrument=1, command=7, parameter=0x08)),
        row(event(0, command=22, parameter=0x40)),
        row(event(0, command=26, parameter=0x58)),
        row(event(0, note=254)),
        row(event(0, note=67, instrument=1)),
        row(), row(), row(), row(), row(), row(), row(),
        row(event(0, command=3, parameter=0x00)),
    ]
    packed = b"".join(rows)
    pattern = p16(len(packed)) + p16(len(rows)) + b"\x00" * 4 + packed

    orders = bytes((0, 255))
    ordnum, insnum, smpnum, patnum = len(orders), 0, 1, 1
    sample_header_offset = 192 + ordnum + (4 * smpnum) + (4 * patnum)
    pattern_offset = sample_header_offset + 80
    sample_data_offset = pattern_offset + len(pattern)

    header = bytearray()
    header += b"IMPM"
    header += TITLE.encode("ascii")[:26].ljust(26, b"\x00")
    header += p16(0)
    header += p16(ordnum) + p16(insnum) + p16(smpnum) + p16(patnum)
    header += p16(0x0214) + p16(0x0214)
    header += p16(0x0009)
    header += p16(0)
    header += bytes((128, 128, 4, 140, 128, 0))
    header += p16(0) + p32(0) + p32(0)
    channel_pan = bytearray((32,) * 64)
    channel_volume = bytearray((64,) * 64)
    for index in range(1, 64):
        channel_pan[index] = 0x80
    header += channel_pan + channel_volume
    assert len(header) == 192

    sample_header = bytearray()
    sample_header += b"IMPS"
    sample_header += b"psycle.raw".ljust(13, b"\x00")
    sample_header += bytes((64, 0x11, 64))
    sample_header += b"PSYCLE deterministic pulse"[:26].ljust(26, b"\x00")
    sample_header += bytes((0x01, 32))
    sample_header += p32(4096)
    sample_header += p32(0) + p32(4096)
    sample_header += p32(8363)
    sample_header += p32(0) + p32(0)
    sample_header += p32(sample_data_offset)
    sample_header += bytes((0, 0, 0, 0))
    assert len(sample_header) == 80

    sample = bytes((((index * 13) % 127) - 63) & 0xFF for index in range(4096))
    return bytes(header) + orders + p32(sample_header_offset) + p32(pattern_offset) + bytes(sample_header) + pattern + sample


def build_decoder_fixture(
    left: bytes,
    *,
    convert: int,
    right: bytes | None = None,
) -> bytes:
    """Build a tiny noncanonical fixture that isolates uncompressed 8-bit decoding."""
    if not left:
        raise ValueError("decoder fixture left channel must be non-empty")
    if right is not None and len(right) != len(left):
        raise ValueError("decoder fixture stereo channels must have equal length")

    payload = bytearray(build_fixture())
    orders_offset = 192
    sample_header_offset = struct.unpack_from("<I", payload, orders_offset + 2)[0]
    sample_data_offset = struct.unpack_from("<I", payload, sample_header_offset + 72)[0]

    flags = 0x01 | (0x04 if right is not None else 0)
    payload[sample_header_offset + 18] = flags
    payload[sample_header_offset + 46] = convert & 0xFF
    struct.pack_into("<I", payload, sample_header_offset + 48, len(left))
    struct.pack_into("<I", payload, sample_header_offset + 52, 0)
    struct.pack_into("<I", payload, sample_header_offset + 56, 0)

    sample_bytes = left if right is None else left + right
    return bytes(payload[:sample_data_offset]) + sample_bytes


def build_decoder_fixtures() -> dict[str, bytes]:
    return {
        "signed": build_decoder_fixture(
            bytes((0x80, 0x00, 0x7F)),
            convert=0x01,
        ),
        "unsigned": build_decoder_fixture(
            bytes((0x00, 0x80, 0xFF)),
            convert=0x00,
        ),
        "signed-delta-wrap": build_decoder_fixture(
            bytes((0x7F, 0x02, 0x80)),
            convert=0x05,
        ),
        "unsigned-delta-wrap": build_decoder_fixture(
            bytes((0xFF, 0x02, 0x80)),
            convert=0x04,
        ),
        "stereo-signed-delta-reset": build_decoder_fixture(
            bytes((0x0A, 0x01, 0x01)),
            right=bytes((0x01, 0x01, 0x01)),
            convert=0x05,
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--decoder-fixtures-dir", type=Path)
    args = parser.parse_args()

    payload = build_fixture()
    digest = hashlib.sha256(payload).hexdigest()
    if len(payload) != EXPECTED_SIZE or digest != EXPECTED_SHA256:
        raise SystemExit(f"generated fixture identity changed: size={len(payload)} sha256={digest}")
    if args.check and args.output.exists() and args.output.read_bytes() != payload:
        raise SystemExit("existing fixture differs from canonical generated bytes")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(payload)

    decoder_files: list[str] = []
    if args.decoder_fixtures_dir is not None:
        args.decoder_fixtures_dir.mkdir(parents=True, exist_ok=True)
        for name, decoder_payload in build_decoder_fixtures().items():
            path = args.decoder_fixtures_dir / f"phase6c-it-decoder-{name}.it"
            path.write_bytes(decoder_payload)
            decoder_files.append(str(path))

    print(json.dumps({
        "schema_version": SCHEMA_VERSION,
        "path": str(args.output),
        "size_bytes": len(payload),
        "sha256": digest,
        "title": TITLE,
        "speed": 4,
        "tempo": 140,
        "rows": 16,
        "commands": ["E10", "F10", "G08", "V40", "Z58", "C00"],
        "note_cut": True,
        "decoder_fixtures": decoder_files,
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
