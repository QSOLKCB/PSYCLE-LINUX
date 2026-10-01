#!/usr/bin/env python3
"""Static/negative controls for the external SickMaate observation lane."""
from __future__ import annotations

import hashlib
import importlib.util
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

helper_spec = importlib.util.spec_from_file_location(
    "phase6c_historical_it_helper_test", HELPER
)
historical_helper = importlib.util.module_from_spec(helper_spec)
assert helper_spec.loader is not None
helper_spec.loader.exec_module(historical_helper)

assert historical_helper.MANIFEST.resolve() == MANIFEST.resolve()
production_source = historical_helper.manifest_source()
assert production_source["filename"] == "d-503_-_sickmaate.it"
assert production_source["sha256"] == EXPECTED_SHA
assert production_source["size_bytes"] == EXPECTED_SIZE
assert production_source["title"] == "SickMaate"
assert production_source["redistribution"] == "not_committed"

historical_validator_spec = importlib.util.spec_from_file_location(
    "phase6c_historical_original_validator_test", HISTORICAL_VALIDATOR
)
historical_validator = importlib.util.module_from_spec(historical_validator_spec)
assert historical_validator_spec.loader is not None
historical_validator_spec.loader.exec_module(historical_validator)

with tempfile.TemporaryDirectory() as temporary:
    temporary_root = Path(temporary)
    synthetic_title = "SyntheticWitness"
    synthetic_bytes = (
        b"IMPM"
        + synthetic_title.encode("ascii").ljust(26, b"\0")
        + bytes(range(64))
    )
    synthetic_file = temporary_root / "synthetic-witness.it"
    synthetic_file.write_bytes(synthetic_bytes)
    synthetic_sha = hashlib.sha256(synthetic_bytes).hexdigest()

    synthetic_manifest = {
        "schema_version": 1,
        "contract": "legacy-impulse-tracker-import-reference",
        "status": "EXTERNAL_REFERENCE",
        "source": {
            "filename": synthetic_file.name,
            "sha256": synthetic_sha,
            "size_bytes": len(synthetic_bytes),
            "title": synthetic_title,
            "redistribution": "not_committed",
        },
    }
    synthetic_manifest_path = temporary_root / "historical-manifest.json"
    synthetic_manifest_path.write_text(
        json.dumps(synthetic_manifest, indent=2) + "\n",
        encoding="utf-8",
    )

    original_manifest_path = historical_helper.MANIFEST
    try:
        historical_helper.MANIFEST = synthetic_manifest_path
        identity = historical_helper.validate_historical_file(synthetic_file)
        assert identity == {
            "filename": synthetic_file.name,
            "sha256": synthetic_sha,
            "size_bytes": len(synthetic_bytes),
            "title": synthetic_title,
            "redistribution": "external-hash-bound",
        }

        synthetic_manifest["source"]["sha256"] = "0" * 64
        synthetic_manifest_path.write_text(
            json.dumps(synthetic_manifest, indent=2) + "\n",
            encoding="utf-8",
        )
        try:
            historical_helper.validate_historical_file(synthetic_file)
        except SystemExit as exc:
            assert "SHA-256 mismatch" in str(exc)
        else:
            raise AssertionError(
                "historical helper must derive the expected SHA-256 from MANIFEST"
            )
    finally:
        historical_helper.MANIFEST = original_manifest_path

with tempfile.TemporaryDirectory() as temporary:
    leak_root = Path(temporary)
    renamed_payload = leak_root / "renamed-private.log"
    synthetic_payload = b"IMPM" + b"renamed historical payload bytes"
    renamed_payload.write_bytes(synthetic_payload)
    synthetic_payload_sha = hashlib.sha256(synthetic_payload).hexdigest()
    leaks = historical_validator.find_historical_payload_leaks(
        leak_root,
        expected_sha256=synthetic_payload_sha,
        expected_size=len(synthetic_payload),
    )
    assert leaks == ["renamed-private.log"]

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
    assert "# Phase 6C Original Psycle Native-Windows Historical IT Evidence" in observer
    assert "manifest-bound external SickMaate IT bytes are supplied privately" in observer
    assert "Historical fixture policy:" in observer
    assert "project-authored fixture bytes copied into this artifact" not in observer

validator_source = VALIDATOR.read_text(encoding="utf-8")
assert "external_fixture: bool = False" in validator_source
assert 'original.get("fixture_distribution") != "external-hash-bound"' in validator_source
assert 'original.get("fixture_redistributed") is not False' in validator_source

historical_validator_source = HISTORICAL_VALIDATOR.read_text(encoding="utf-8")
assert EXPECTED_SHA in historical_validator_source
assert 'external_fixture=True' in historical_validator_source
assert "find_historical_payload_leaks" in historical_validator_source
assert "manifest-bound IT bytes" in historical_validator_source

for label, expected_role, wrong_role in (
    ("donor", "cpsycle-donor-observation", "candidate-observation"),
    ("candidate", "candidate-observation", "cpsycle-donor-observation"),
    ("original", "original", "not-original"),
):
    malformed = {
        "schema_version": 1,
        "phase": "6C",
        "contract": "legacy-impulse-tracker-import-reference",
        "evidence_role": wrong_role,
        "parity_status": "UNKNOWN",
    }
    try:
        historical_helper.require_common_receipt(
            malformed,
            label=label,
            evidence_role=expected_role,
        )
    except SystemExit as exc:
        assert "evidence_role" in str(exc)
    else:
        raise AssertionError(f"{label} wrong-role receipt must be rejected")

incomplete_donor = {
    "schema_version": 1,
    "phase": "6C",
    "contract": "legacy-impulse-tracker-import-reference",
    "evidence_role": "cpsycle-donor-observation",
    "parity_status": "UNKNOWN",
    "source_sha256": EXPECTED_SHA,
    "source_size_bytes": EXPECTED_SIZE,
}
try:
    historical_helper.validate_summary_components(
        source=manifest["source"],
        donor_path=Path("donor/cpsycle-historical-it.json"),
        donor=incomplete_donor,
        candidate_path=Path("candidate/candidate-historical-it.json"),
        candidate={},
        original_path=Path("original/original-historical-legacy-it.json"),
        original={},
    )
except SystemExit as exc:
    assert "load_result=accepted" in str(exc)
else:
    raise AssertionError("summary must reject donor receipt missing observation results")

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
assert "phase6c-historical-summary/donor/" in workflow
assert "phase6c-historical-summary/candidate/" in workflow
assert "phase6c-historical-summary/original/" in workflow
assert "historical module bytes leaked into combined artifact" in workflow
for upload_block in workflow.split("uses: actions/upload-artifact@v4")[1:]:
    block = upload_block.split("\n      - name:", 1)[0]
    assert ".it" not in block.lower(), block

print("phase6c-historical-it: PASS")
