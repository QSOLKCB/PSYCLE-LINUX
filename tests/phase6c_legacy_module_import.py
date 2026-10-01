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

validator_spec = importlib.util.spec_from_file_location(
    "phase6c_original_validator",
    ROOT / "scripts/phase6c-validate-original-receipts-v2.py",
)
validator = importlib.util.module_from_spec(validator_spec)
assert validator_spec.loader is not None
validator_spec.loader.exec_module(validator)

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

decoder_fixtures = module.build_decoder_fixtures()
assert set(decoder_fixtures) == {
    "signed",
    "unsigned",
    "signed-delta-wrap",
    "unsigned-delta-wrap",
    "stereo-signed-delta-reset",
}
for name, decoder_payload in decoder_fixtures.items():
    assert decoder_payload[:4] == b"IMPM", name
    assert decoder_payload != payload, name

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

def decoder_header(name: str) -> tuple[int, int, int]:
    decoder_payload = decoder_fixtures[name]
    return (
        decoder_payload[sample_header_offset + 18],
        decoder_payload[sample_header_offset + 46],
        struct.unpack_from("<I", decoder_payload, sample_header_offset + 48)[0],
    )

assert decoder_header("signed") == (0x01, 0x01, 3)
assert decoder_header("unsigned") == (0x01, 0x00, 3)
assert decoder_header("signed-delta-wrap") == (0x01, 0x05, 3)
assert decoder_header("unsigned-delta-wrap") == (0x01, 0x04, 3)
assert decoder_header("stereo-signed-delta-reset") == (0x05, 0x05, 3)

packed_size, row_count = struct.unpack_from("<HH", payload, pattern_offset)
assert row_count == 16
packed = payload[pattern_offset + 8:pattern_offset + 8 + packed_size]
assert bytes((26, 0x58)) in packed
assert b"\xfe" in packed

with tempfile.TemporaryDirectory() as temporary:
    out = Path(temporary) / "fixture.it"
    out.write_bytes(payload)
    assert out.read_bytes() == payload

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    arbitrary = root / "unrelated-private.it"
    arbitrary.write_bytes(b"IMPM" + b"private")
    try:
        validator.validate_artifact_inventory(root)
    except SystemExit:
        pass
    else:
        raise AssertionError("global original-artifact validation must reject arbitrary .it files")

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    canonical = root / "fixtures/legacy-it/phase6c-legacy-it-import.it"
    canonical.parent.mkdir(parents=True)
    canonical.write_bytes(payload)
    validator.validate_artifact_inventory(
        root,
        allowed_it={
            "fixtures/legacy-it/phase6c-legacy-it-import.it": module.EXPECTED_SHA256
        },
    )

    misplaced = root / "phase6c-legacy-it-import.it"
    misplaced.write_bytes(payload)
    try:
        validator.validate_artifact_inventory(
            root,
            allowed_it={
                "fixtures/legacy-it/phase6c-legacy-it-import.it": module.EXPECTED_SHA256
            },
        )
    except SystemExit:
        pass
    else:
        raise AssertionError("misplaced .it evidence must fail inventory validation")
    misplaced.unlink()

    canonical.write_bytes(b"IMPM" + b"changed")
    try:
        validator.validate_artifact_inventory(
            root,
            allowed_it={
                "fixtures/legacy-it/phase6c-legacy-it-import.it": module.EXPECTED_SHA256
            },
        )
    except SystemExit:
        pass
    else:
        raise AssertionError("noncanonical .it bytes must fail inventory validation")

historical = ROOT / "phase6c/evidence/legacy-module-import/historical-sickmaate.json"
assert historical.exists()
reference = ROOT / "phase6c/reference-corpus/manifest.json"
assert reference.exists()

itmodule_source = (ROOT / "cpsycle/audio/src/itmodule2.c").read_text(encoding="utf-8")
assert "IT sample mode uses the pattern instrument byte as a sample number." in itmodule_source
assert "psy_audio_instrument_setindex(instr, i);" in itmodule_source
assert "psy_audio_instrument_set_name(instr, psy_audio_sample_name(wave));" in itmodule_source
assert "modtovirtual_set(&self->ittovirtual, i, virtualInst);" in itmodule_source
assert "it doesn't use instruments" not in itmodule_source

playback_probe = ROOT / "tests/phase6c_legacy_it_playback.c"
assert playback_probe.exists()
playback_probe_source = playback_probe.read_text(encoding="utf-8")
assert "sample-mode playback witness is silent" in playback_probe_source
assert "fresh playback witnesses are not bit-identical" in playback_probe_source
assert "psy_audio_VIRTUALGENERATOR" in playback_probe_source
assert "legacy-it-sample-mode-playback-donor" in playback_probe_source

builder = ROOT / "scripts/phase6c-build-legacy-it-observer.py"
base_observer = ROOT / "scripts/phase6c-original-windows-fixtures-v2.ps1"

base_observer_bytes = base_observer.read_bytes().replace(b"\r\n", b"\n")
assert b"\r" not in base_observer_bytes
base_observer_blob = hashlib.sha1(
    b"blob " + str(len(base_observer_bytes)).encode() + b"\0" + base_observer_bytes
).hexdigest()
assert base_observer_blob == "4a10fb12805ff10a85cfedc0118443220a7dc32a"

base_observer_source = base_observer_bytes.decode("utf-8")
assert "--ssl-revoke-best-effort" in base_observer_source
assert "--ssl-no-revoke" not in base_observer_source
assert "--insecure" not in base_observer_source
assert "f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769" in base_observer_source
assert "$ExpectedInstallerSize = 9322919" in base_observer_source

for observer_builder in (
    "scripts/phase6c-build-legacy-it-observer.py",
    "scripts/phase6c-build-timing-observer.py",
    "scripts/phase6c-build-sampler-ps1-pitch-observer.py",
    "scripts/phase6c-build-sampler-ps1-extended-timing-observer.py",
    "scripts/phase6c-build-delayed-retrigger-observer.py",
):
    builder_source = (ROOT / observer_builder).read_text(encoding="utf-8")
    expected_line = next(
        line for line in builder_source.splitlines()
        if line.startswith("EXPECTED_BLOB = ")
    )
    pinned_blob = expected_line.split('"', 2)[1]
    assert pinned_blob == base_observer_blob, observer_builder

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
    assert '$LegacyItVc90RuntimeArtifact = Join-Path $outRoot "vc90-runtime.txt"' in observer
    assert 'Copy-Item -LiteralPath $LegacyItVc90RuntimeReceipt' in observer
    assert '$environment["vc90_redistributable"] = [ordered]@{' in observer
    assert 'redistributable_version = $LegacyItVc90RuntimeVersion' in observer
    assert 'receipt = "vc90-runtime.txt"' in observer

workflow_source = (
    ROOT / ".github/workflows/phase6c-legacy-playback-import.yml"
).read_text(encoding="utf-8")
assert "github.event.pull_request.head.sha || github.sha" not in workflow_source
assert "['git', 'rev-parse', 'HEAD']" in workflow_source
assert "phase6c-generated/phase6c-legacy-it-loader" in workflow_source
assert (
    workflow_source.count("      - 'scripts/phase6c-original-windows-fixtures-v2.ps1'\n")
    == 2
)
for path in (
    "scripts/phase6-upstream-audit.sh",
    "scripts/phase6b-sanitized-manifest.sh",
    "scripts/phase6b-verify-committed-source.sh",
    "scripts/phase6b-verify-qmake-support.sh",
    "tests/phase6c_legacy_it_decoder.c",
    "tests/phase6c_legacy_it_playback.c",
):
    assert workflow_source.count(f"      - '{path}'\n") == 2, path

assert "--decoder-fixtures-dir phase6c-generated/decoder-fixtures" in workflow_source
assert "Build donor IT decoder regression probe" in workflow_source
assert "Exercise donor 8-bit IT decoder regressions" in workflow_source
for variant in decoder_fixtures:
    assert variant in workflow_source

assert "Build donor IT sample-mode playback witness" in workflow_source
assert "Prove non-silent deterministic IT sample-mode playback" in workflow_source
assert "tests/phase6c_legacy_it_playback.c" in workflow_source

donor_playback_marker = "      - name: Prove non-silent deterministic IT sample-mode playback\n"
donor_playback_start = workflow_source.index(donor_playback_marker)
donor_playback_end = workflow_source.index(
    "\n      - name: Load generated IT through C-Psycle donor runtime",
    donor_playback_start,
)
donor_playback_block = workflow_source[donor_playback_start:donor_playback_end]
assert "legacy-it-sample-mode-playback-donor" in donor_playback_block
assert "'parity_status': 'UNKNOWN'" in donor_playback_block
assert "donor sample-mode mapping plus deterministic non-silent playback only" in donor_playback_block
assert "pushd phase6c-generated >/dev/null" in donor_playback_block
assert "chmod +x phase6c-legacy-it-playback" in donor_playback_block
assert (
    "./phase6c-legacy-it-playback phase6c-legacy-it-import.it \\\n"
    "            </dev/null >cpsycle-donor-playback.log 2>&1"
) in donor_playback_block
assert "'replay_setup': ['chmod', '+x', probe.name]" in donor_playback_block
assert "'working_directory': 'artifact-root'" in donor_playback_block
assert "'stdin': '/dev/null'" in donor_playback_block
assert "'stdout_stderr': log.name" in donor_playback_block
assert (
    "'procedure': 'chmod +x phase6c-legacy-it-playback && "
    "./phase6c-legacy-it-playback phase6c-legacy-it-import.it "
    "</dev/null >cpsycle-donor-playback.log 2>&1'"
) in donor_playback_block

donor_upload_marker = "      - name: Upload donor observation\n"
donor_upload_start = workflow_source.index(donor_upload_marker)
donor_upload_end = workflow_source.index(
    "\n\n  candidate-direct-import-boundary:", donor_upload_start
)
donor_upload_block = workflow_source[donor_upload_start:donor_upload_end]
assert "phase6c-generated/cpsycle-itmodule2.c" in donor_upload_block
assert "phase6c-generated/phase6c-legacy-it-loader" in donor_upload_block
assert "phase6c-generated/phase6c-legacy-it-playback" in donor_upload_block
assert "phase6c-generated/cpsycle-donor-playback.log" in donor_upload_block
assert "phase6c-generated/cpsycle-donor-playback.json" in donor_upload_block
assert "phase6c-generated/cpsycle-donor-playback-receipt.json" in donor_upload_block
assert "cp -- cpsycle/audio/src/itmodule2.c phase6c-generated/cpsycle-itmodule2.c" in workflow_source
assert "'source_file': source_file.name" in workflow_source
assert "'source_file_origin': 'cpsycle/audio/src/itmodule2.c'" in workflow_source

candidate_upload_marker = "      - name: Upload candidate direct-import observation\n"
candidate_upload_start = workflow_source.index(candidate_upload_marker)
candidate_upload_end = workflow_source.index(
    "\n\n\n  original-legacy-it-observation:", candidate_upload_start
)
candidate_upload_block = workflow_source[candidate_upload_start:candidate_upload_end]
assert "phase6c-generated/phase6c-legacy-it-import.it" in candidate_upload_block
assert "phase6c-generated/candidate-psycle-player" in candidate_upload_block
assert "phase6c-generated/candidate-direct-import.log" in candidate_upload_block
assert "phase6c-generated/candidate-direct-import.json" in candidate_upload_block

assert "'fixture': fixture.name" in workflow_source
assert "'player': player.name" in workflow_source
assert "'log': log.name" in workflow_source
assert "'command': [" in workflow_source
assert "'./candidate-psycle-player'" in workflow_source
assert "'--output-driver'" in workflow_source
assert "'dummy'" in workflow_source
assert "'--input-file'" in workflow_source
assert "'phase6c-legacy-it-import.it'" in workflow_source
assert "'stdin': '/dev/null'" in workflow_source
assert "'stdout_stderr': log.name" in workflow_source
assert "'working_directory': 'artifact-root'" in workflow_source
assert "'replay_setup': ['chmod', '+x', 'candidate-psycle-player']" in workflow_source
assert "chmod +x candidate-psycle-player" in workflow_source
assert (
    "'procedure': 'chmod +x candidate-psycle-player && "
    "./candidate-psycle-player --output-driver dummy "
    "--input-file phase6c-legacy-it-import.it </dev/null "
    ">candidate-direct-import.log 2>&1'"
) in workflow_source
assert "'parity_status': 'UNKNOWN'" in workflow_source
assert "'parity_classification': 'NOT_CLASSIFIED'" not in workflow_source

legacy_doc = (ROOT / "phase6c/LEGACY_PLAYBACK_IMPORT.md").read_text(encoding="utf-8")
assert "size:   4,439 bytes" in legacy_doc
assert module.EXPECTED_SHA256 in legacy_doc
assert "size:   599 bytes" not in legacy_doc
assert "1670e48dc761296e9c3497f6f3c6632fbc020b2f47bf46d97e3f9571b44b4f3e" not in legacy_doc

validator_source = (
    ROOT / "scripts/phase6c-validate-original-receipts-v2.py"
).read_text(encoding="utf-8")
assert '".psy"}' in validator_source
assert '".psy", ".it"' not in validator_source

legacy_validator_source = (
    ROOT / "scripts/phase6c-validate-legacy-it-original.py"
).read_text(encoding="utf-8")
assert 'fixture_ref != "phase6c-legacy-it-import.it"' in legacy_validator_source
assert "generator.EXPECTED_SHA256" in legacy_validator_source
assert 'original_fixture_ref = f"fixtures/legacy-it/{fixture_ref}"' in legacy_validator_source
assert "allowed_it={original_fixture_ref: generator.EXPECTED_SHA256}" in legacy_validator_source

print("phase6c-legacy-module-import: PASS")
