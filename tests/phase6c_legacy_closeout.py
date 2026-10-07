#!/usr/bin/env python3
"""Closeout cannot erase pending observations or promote donor evidence to parity."""
from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("closeout_test", ROOT / "scripts/phase6c-legacy-closeout.py")
helper = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(helper)
ledger = helper.read_json(helper.LEDGER)


def rejected(operation, message):
    try:
        operation()
    except (ValueError, SystemExit) as exc:
        assert message in str(exc), str(exc)
    else:
        raise AssertionError(f"expected rejection: {message}")


matrix = (ROOT / "phase6c/compatibility-matrix.json").read_bytes()
report = helper.closeout_report(ledger, {})
assert report["implementation_status"] == "COMPLETE"
assert report["observation_summaries_revalidated"] is False
assert report["priority_hold"] == "AWAITING_OBSERVATION_REVIEW"
assert report["parity_status"] == "UNKNOWN"
assert all(item["status"] == "PENDING_PRIVATE_DISPATCH" for item in report["observations"].values())

for field, value, message in (
    ("schema_version", True, "schema_version"),
    ("parity_status", "PASS", "parity_status"),
    ("preserved_inputs", {}, "identities"),
    ("dispositions", ledger["dispositions"][:-1], "incomplete"),
    ("pending_observations", {}, "observation set"),
    ("resume_contract", "phase-7", "resume_contract"),
):
    altered = copy.deepcopy(ledger)
    altered[field] = value
    rejected(lambda: helper.validate_ledger(altered), message)

for mutation, message in (
    (lambda data: data["dispositions"][0].update(status="PASS"), "invalid disposition"),
    (lambda data: data["dispositions"][1].update(id=data["dispositions"][0]["id"]), "duplicate"),
    (lambda data: data["dispositions"][0].update(references=["../outside"]), "escapes root"),
    (lambda data: data["dispositions"][0].update(references=["missing-source"]), "missing"),
    (lambda data: data["pending_observations"]["historical-sickmaate"].update(status="OBSERVED"), "boundary"),
):
    altered = copy.deepcopy(ledger)
    mutation(altered)
    rejected(lambda: helper.validate_ledger(altered), message)

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    duplicate = root / "duplicate.json"
    duplicate.write_text('{"parity_status":"UNKNOWN","parity_status":"PASS"}')
    rejected(lambda: helper.read_json(duplicate), "duplicate JSON")
    duplicate.unlink()

    # A plausible summary without component evidence must not count as dispatch.
    (root / "historical-sickmaate-three-way.json").write_text('{"parity_status":"UNKNOWN"}')
    rejected(lambda: helper.revalidate_summary(root, "historical-sickmaate"), "missing")
    (root / "midi-real-world-corpus-summary.json").write_text('{"parity_status":"UNKNOWN"}')
    rejected(lambda: helper.revalidate_summary(root, "midi-real-world-corpus"), "missing")
    (root / "link").symlink_to(ROOT / "README.md")
    rejected(lambda: helper.audit_artifact(root), "symlinks")
    (root / "link").unlink()
    for name, payload in (
        ("renamed.log", b"IMPMprivate module"),
        ("audio.wav", b"audio"),
        ("renamed-audio.log", b"RIFF\x04\x00\x00\x00WAVE"),
    ):
        path = root / name
        path.write_bytes(payload)
        rejected(lambda: helper.audit_artifact(root), "raw IT/audio")
        path.unlink()
    midi = helper.load_helper("phase6c-midi-corpus.py")
    midi.write_synthetic_fixture(root / "renamed.log")
    rejected(lambda: helper.audit_artifact(root), "raw corpus")

# Positive summary validation uses project-owned metadata/support bytes only.
# It proves the component validator is invoked, not original execution.
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    historical = helper.load_helper("phase6c-historical-it.py")
    source = historical.manifest_source()
    for name in ("donor", "candidate", "original"):
        (root / name).mkdir()
    common = {"schema_version": 1, "phase": "6C", "contract": historical.EXPECTED_CONTRACT, "parity_status": "UNKNOWN"}

    def support(directory, name):
        path = root / directory / name
        path.write_text("project-owned support fixture\n")
        return name, helper.digest(path)

    donor_source, donor_sha = support("donor", "source.c")
    probe, probe_sha = support("donor", "probe")
    player, player_sha = support("candidate", "player")
    player_path = root / "candidate" / player
    player_path.write_bytes(b"\x7fELFproject-owned candidate fixture")
    player_sha = helper.digest(player_path)
    midi = helper.load_helper("phase6c-midi-corpus.py")
    identity = {
        "candidate_baseline_sha256": midi.EXPECTED_CANDIDATE_BASELINE,
        "source_revision": midi.EXPECTED_CANDIDATE_SOURCE_REVISION,
        "repository_commit": midi.repository_commit(),
        "build_target": "psycle-player",
        "plugin_interface_git_blob": midi.EXPECTED_CANDIDATE_PLUGIN_BLOB,
        "diversalis_revision": f"SourceForge SVN r{midi.DIVERSALIS_REVISION}",
        "diversalis_file_count": midi.EXPECTED_DIVERSALIS_FILE_COUNT,
        "diversalis_manifest_sha256": midi.EXPECTED_DIVERSALIS_MANIFEST_SHA256,
        "clean_rebuild_sha256": player_sha,
    }
    attestation_path = root / "candidate/candidate-player-attestation.json"
    attestation = midi.candidate_attestation_document(identity, player_sha)
    attestation_path.write_text(json.dumps(attestation))
    donor = dict(common, evidence_role="cpsycle-donor-observation", source_sha256=source["sha256"], source_size_bytes=source["size_bytes"], load_result="accepted", non_silent_playback=True, fixture_redistributed=False, private_input_required=True, source_file=donor_source, source_file_sha256=donor_sha, probe_executable=probe, probe_executable_sha256=probe_sha, raw_output_sha256="1" * 64)
    candidate = dict(common, evidence_role="candidate-observation", source_sha256=source["sha256"], source_size_bytes=source["size_bytes"], direct_it_load="rejected", diagnostic_could_not_load_song_file=True, exit_code=2, private_input_required=True, player=player, player_sha256=player_sha, raw_output_sha256="2" * 64)
    candidate.update(identity, build_attestation=attestation_path.name, build_attestation_sha256=helper.digest(attestation_path))
    original = dict(common, evidence_role="original", fixture_sha256=source["sha256"], fixture_distribution="external-hash-bound", fixture_redistributed=False, original_psycle_observed=True, reference_build=historical.EXPECTED_REFERENCE_BUILD, reference_file=historical.EXPECTED_REFERENCE_FILE, reference_installer_sha256=historical.EXPECTED_REFERENCE_INSTALLER_SHA256, reference_installer_size_bytes=historical.EXPECTED_REFERENCE_INSTALLER_SIZE, reference_executable_sha256=historical.EXPECTED_REFERENCE_EXECUTABLE_SHA256, load_result="inconclusive", observation="project-owned summary validation fixture")
    for field in ("stdout", "stderr", "ui_evidence", "loaded_vc90_runtime", "machine_plugin_inventory"):
        name, sha = support("original", field + ".json")
        mapping = {"path": name, "sha256": sha}
        if field in ("loaded_vc90_runtime", "machine_plugin_inventory"):
            original.setdefault("environment", {})[field] = mapping
        else:
            original[field] = mapping
    donor_path = root / "donor/cpsycle-historical-it.json"
    candidate_path = root / "candidate/candidate-historical-it.json"
    original_path = root / "original/original-historical-legacy-it.json"
    for path, data in ((donor_path, donor), (candidate_path, candidate), (original_path, original)):
        path.write_text(json.dumps(data))
    summary = root / "historical-sickmaate-three-way.json"
    historical.command_summary(argparse.Namespace(donor=donor_path, candidate=candidate_path, original=original_path, output=summary))
    one_lane = helper.closeout_report(ledger, {"historical-sickmaate": root})
    assert one_lane["observations"]["historical-sickmaate"]["status"] == "COMPONENTS_REVALIDATED"
    assert one_lane["observation_summaries_revalidated"] is False
    assert one_lane["priority_hold"] == "AWAITING_OBSERVATION_REVIEW"

    for field, bad in (
        ("repository_commit", "0" * 40),
        ("candidate_baseline_sha256", "0" * 64),
        ("clean_rebuild_sha256", "0" * 64),
        ("plugin_interface_git_blob", "0" * 40),
    ):
        changed_attestation = dict(attestation)
        changed_attestation[field] = bad
        attestation_path.write_text(json.dumps(changed_attestation))
        changed_candidate = dict(candidate, **{field: bad})
        changed_candidate["build_attestation_sha256"] = helper.digest(attestation_path)
        candidate_path.write_text(json.dumps(changed_candidate))
        rejected(lambda: helper.revalidate_summary(root, "historical-sickmaate"), "attestation is incomplete or invalid")
    attestation_path.write_text(json.dumps(attestation))
    candidate_path.write_text(json.dumps(candidate))
    changed_candidate = dict(candidate, repository_commit="0" * 40)
    candidate_path.write_text(json.dumps(changed_candidate))
    rejected(lambda: helper.revalidate_summary(root, "historical-sickmaate"), "differs from its frozen build identity")
    candidate_path.write_text(json.dumps(candidate))
    original_player = player_path.read_bytes()
    player_path.write_bytes(b"\x7fELFsubstituted candidate")
    changed_candidate = dict(candidate, player_sha256=helper.digest(player_path))
    candidate_path.write_text(json.dumps(changed_candidate))
    rejected(lambda: helper.revalidate_summary(root, "historical-sickmaate"), "attestation is incomplete or invalid")
    player_path.write_bytes(original_player)
    candidate_path.write_text(json.dumps(candidate))
    attestation_path.unlink()
    rejected(lambda: helper.revalidate_summary(root, "historical-sickmaate"), "support file is missing")
    attestation_path.write_text(json.dumps(attestation))
    changed = helper.read_json(summary)
    changed["observations"]["candidate"]["direct_it_load"] = "accepted"
    summary.write_text(json.dumps(changed))
    rejected(lambda: helper.revalidate_summary(root, "historical-sickmaate"), "differs")
    historical.command_summary(argparse.Namespace(donor=donor_path, candidate=candidate_path, original=original_path, output=summary))
    (root / "donor/source.c").write_text("tampered support")
    rejected(lambda: helper.revalidate_summary(root, "historical-sickmaate"), "hash mismatch")

assert (ROOT / "phase6c/compatibility-matrix.json").read_bytes() == matrix
print("phase6c-legacy-closeout: PASS")
