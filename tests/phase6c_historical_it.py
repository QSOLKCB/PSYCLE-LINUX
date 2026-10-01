#!/usr/bin/env python3
"""Static/negative controls for the external SickMaate observation lane."""
from __future__ import annotations

import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "phase6c/evidence/legacy-module-import/historical-sickmaate.json"
HELPER = ROOT / "scripts/phase6c-historical-it.py"
BUILDER = ROOT / "scripts/phase6c-build-historical-it-observer.py"
BASE = ROOT / "scripts/phase6c-original-windows-fixtures-v2.ps1"
VALIDATOR = ROOT / "scripts/phase6c-validate-original-receipts-v2.py"
HISTORICAL_VALIDATOR = ROOT / "scripts/phase6c-validate-historical-it-original.py"
WORKFLOW = ROOT / ".github/workflows/phase6c-historical-it-private.yml"

EXPECTED_SHA = "cab23d8f66a6815b3248457e4f38de74f0a6d61c2691c47a78e582edb63cd432"
EXPECTED_SIZE = 306462

manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
assert manifest["contract"] == "legacy-impulse-tracker-import-reference"
assert manifest["status"] == "EXTERNAL_REFERENCE"
assert manifest["source"]["sha256"] == EXPECTED_SHA
assert manifest["source"]["size_bytes"] == EXPECTED_SIZE
assert manifest["source"]["redistribution"] == "not_committed"

helper_source = HELPER.read_text(encoding="utf-8")
assert 'MANIFEST = ROOT / "phase6c/evidence/legacy-module-import/historical-sickmaate.json"' in helper_source
assert 'source = manifest_source()' in helper_source
assert 'expected_sha = source.get("sha256")' in helper_source
assert "external-hash-bound" in helper_source
assert "private/d-503_-_sickmaate.it" not in helper_source
assert "parity_status" in helper_source
assert '"UNKNOWN"' in helper_source

with tempfile.TemporaryDirectory() as temporary:
    bad = Path(temporary) / "wrong.it"
    bad.write_bytes(b"IMPM" + b"not the historical witness")
    result = subprocess.run(
        ["python3", str(HELPER), "verify", str(bad)],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "size mismatch" in (result.stdout + result.stderr)

with tempfile.TemporaryDirectory() as temporary:
    generated = Path(temporary) / "historical-observer.ps1"
    result = subprocess.run(
        ["python3", str(BUILDER), str(BASE), str(generated)],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    observer = generated.read_text(encoding="utf-8")
    assert "[switch]$ObserveHistoricalLegacyIt" in observer
    assert 'name = "historical-legacy-it"' in observer
    assert 'candidate_receipt = "candidate-historical-legacy-it.json"' in observer
    assert 'expected_contract = "legacy-impulse-tracker-import-reference"' in observer
    assert 'expected_song_title = "SickMaate"' in observer
    assert EXPECTED_SHA in observer
    assert f"-ne {EXPECTED_SIZE}" in observer
    assert '$fixtureCopy = Join-Path $workRoot' in observer
    assert '$fixtureReceiptPath = "external/' in observer
    assert 'fixture_distribution = if ($historicalExternal) { "external-hash-bound" }' in observer
    assert 'fixture_redistributed = (-not $historicalExternal)' in observer

validator_source = VALIDATOR.read_text(encoding="utf-8")
assert "external_fixture: bool = False" in validator_source
assert 'original.get("fixture_distribution") != "external-hash-bound"' in validator_source
assert 'original.get("fixture_redistributed") is not False' in validator_source

historical_validator_source = HISTORICAL_VALIDATOR.read_text(encoding="utf-8")
assert EXPECTED_SHA in historical_validator_source
assert 'external_fixture=True' in historical_validator_source
assert "historical original artifact leaked IT bytes" in historical_validator_source

assert WORKFLOW.exists()
workflow = WORKFLOW.read_text(encoding="utf-8")
assert "workflow_dispatch:" in workflow
assert "PSYCLE_PHASE6C_HISTORICAL_IT_URL" in workflow
assert "phase6c-historical-it.py prepare-private" in workflow
assert "phase6c-build-historical-it-observer.py" in workflow
assert "phase6c-validate-historical-it-original.py" in workflow
assert "tests/phase6c_historical_it_probe.c" in workflow
assert "historical-sickmaate-three-way.json" in workflow
assert "private-input-required" in workflow
assert "private-root/" not in workflow
for upload_block in workflow.split("uses: actions/upload-artifact@v4")[1:]:
    block = upload_block.split("\n      - name:", 1)[0]
    assert ".it" not in block.lower(), block

print("phase6c-historical-it: PASS")
