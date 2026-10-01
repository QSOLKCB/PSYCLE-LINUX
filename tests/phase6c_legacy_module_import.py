#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import importlib.util
import struct
import subprocess
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
assert struct.unpack_from("<I", payload, sample_header_offset + 48)[0] == 4096
assert struct.unpack_from("<I", payload, sample_header_offset + 52)[0] == 0
assert struct.unpack_from("<I", payload, sample_header_offset + 56)[0] == 4096
assert struct.unpack_from("<I", payload, sample_header_offset + 60)[0] == 8363
assert payload[sample_header_offset + 18] & 0x10

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


builder = ROOT / "scripts/phase6c-build-legacy-it-observer.py"
base_observer = ROOT / "scripts/phase6c-original-windows-fixtures-v2.ps1"
with tempfile.TemporaryDirectory() as temporary:
    generated = Path(temporary) / "legacy-it-observer.ps1"
    result = subprocess.run(
        ["python3", str(builder), str(base_observer), str(generated)],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    observer = generated.read_text(encoding="utf-8")
    assert "[switch]$ObserveLegacyIt" in observer
    assert 'name = "legacy-it"' in observer
    assert 'candidate_receipt = "candidate-legacy-it.json"' in observer
    assert 'expected_contract = "legacy-it-import-reference"' in observer
    assert "8742bcbf24ef328a72d2a27b693cc7071e38d3bb4b9b44dec42aa3d2c8d61d92" in observer
    assert 'procedure = if ($ObserveLegacyIt -and $spec.name -eq "legacy-it") {' in observer

    assert "$env:PSYCLE_PHASE6C_VC90_RUNTIME_URL" in observer
    assert "$env:PSYCLE_PHASE6C_VC90_RUNTIME_SHA256" in observer
    assert "$env:PSYCLE_PHASE6C_VC90_RUNTIME_VERSION" in observer
    assert "$env:PSYCLE_PHASE6C_VC90_RUNTIME_RECEIPT" in observer
    assert "pinned VC90 x86 runtime required and bound for replay" in observer
    assert "vc90_runtime_url=$LegacyItVc90RuntimeUrl" in observer
    assert "vc90_runtime_sha256=$LegacyItVc90RuntimeSha256" in observer
    assert "vc90_runtime_version=$LegacyItVc90RuntimeVersion" in observer
    assert "vc90_runtime_receipt=$LegacyItVc90RuntimeReceipt" in observer

validator_source = (
    ROOT / "scripts/phase6c-validate-original-receipts-v2.py"
).read_text(encoding="utf-8")
assert '".it"' in validator_source

print("phase6c-legacy-module-import: PASS")
