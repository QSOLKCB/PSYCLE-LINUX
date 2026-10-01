#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import importlib.util
import struct
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "scripts/phase6c-generate-it-import-fixture.py"

spec = importlib.util.spec_from_file_location("it_fixture", GENERATOR)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)

payload = module.build_fixture()
assert len(payload) == module.EXPECTED_SIZE
assert hashlib.sha256(payload).hexdigest() == module.EXPECTED_SHA256
assert payload[:4] == b"IMPM"
assert payload[4:30].split(b"\x00", 1)[0] == module.TITLE.encode("ascii")

ordnum, insnum, smpnum, patnum = struct.unpack_from("<4H", payload, 32)
tracker_version, compatible_version, flags, special = struct.unpack_from("<4H", payload, 40)
assert (ordnum, insnum, smpnum, patnum) == (2, 0, 1, 1)
assert tracker_version == compatible_version == 0x0214
assert flags == 0x0009
assert special == 0
assert payload[48:54] == bytes((128, 128, 4, 140, 128, 0))

orders_offset = 192
assert payload[orders_offset:orders_offset + 2] == bytes((0, 255))
sample_header_offset = struct.unpack_from("<I", payload, orders_offset + 2)[0]
pattern_offset = struct.unpack_from("<I", payload, orders_offset + 6)[0]
assert payload[sample_header_offset:sample_header_offset + 4] == b"IMPS"
assert struct.unpack_from("<I", payload, sample_header_offset + 48)[0] == 256
assert struct.unpack_from("<I", payload, sample_header_offset + 60)[0] == 8363

packed_size, row_count = struct.unpack_from("<HH", payload, pattern_offset)
assert row_count == 16
packed = payload[pattern_offset + 8:pattern_offset + 8 + packed_size]
assert bytes((26, 0x58)) in packed
assert b"\xfe" in packed

with tempfile.TemporaryDirectory() as temporary:
    out = Path(temporary) / "fixture.it"
    out.write_bytes(payload)
    assert out.read_bytes() == payload

historical = ROOT / "phase6c/evidence/legacy-module-import/historical-sickmaate.json"
assert historical.exists()
reference = ROOT / "phase6c/reference-corpus/manifest.json"
assert reference.exists()

print("phase6c-legacy-module-import: PASS")
