#!/usr/bin/env python3
"""Negative controls for hosted same-witness observation freezing."""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "phase6c-delayed-retrigger-same-witness-observation.py"
spec = importlib.util.spec_from_file_location("same_witness_observation", SCRIPT)
assert spec is not None and spec.loader is not None
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

H = "1" * 64
D = "sha256:" + ("2" * 64)
HEAD = "3" * 40


def expect_value_error(fn, phrase: str) -> None:
    try:
        fn()
    except ValueError as exc:
        assert phrase in str(exc), exc
    else:
        raise AssertionError("expected ValueError")


metadata = {
    "workflow_name": m.WORKFLOW_NAME,
    "workflow_run_id": 123,
    "workflow_event": "push",
    "workflow_head_sha": HEAD,
    "candidate_job_id": 456,
    "original_job_id": 789,
    "candidate_artifact": {
        "id": 101,
        "name": m.CANDIDATE_ARTIFACT_NAME,
        "digest": D,
    },
    "original_artifact": {
        "id": 102,
        "name": m.ORIGINAL_ARTIFACT_NAME,
        "digest": D,
    },
}
assert m.validate_run_metadata(metadata) == metadata

bad_metadata = copy.deepcopy(metadata)
bad_metadata["candidate_artifact"]["digest"] = H
expect_value_error(
    lambda: m.validate_run_metadata(bad_metadata),
    "candidate artifact metadata is invalid",
)

projection = {
    "schema_version": 1,
    "phase": "6C",
    "contract": m.CONTRACT,
    "parity_status": "UNKNOWN",
    "classification_allowed": False,
    "exact_onset_timing_interpretation": "deferred",
    "fixture": {
        "path": m.same.FIXTURE,
        "sha256": H,
    },
    "canonical_observation": {
        "workflow_name": m.WORKFLOW_NAME,
        "workflow_run_id": 123,
        "workflow_event": "push",
        "workflow_head_sha": HEAD,
        "candidate_job_id": 456,
        "original_job_id": 789,
        "candidate_artifact": {
            "id": 101,
            "name": m.CANDIDATE_ARTIFACT_NAME,
            "digest": D,
            "raw_receipt": {
                "path": m.same.CANDIDATE_RECEIPT,
                "sha256": H,
            },
        },
        "original_artifact": {
            "id": 102,
            "name": m.ORIGINAL_ARTIFACT_NAME,
            "digest": D,
            "raw_receipt": {
                "path": m.same.ORIGINAL_RECEIPT,
                "sha256": H,
            },
            "analysis_receipt": {
                "path": m.same.ORIGINAL_ANALYSIS,
                "sha256": H,
            },
            "comparison_receipt": {
                "path": m.same.COMPARISON,
                "sha256": H,
            },
        },
    },
    "candidate": {
        "runtime_command_execution_observed": True,
        "runtime_command_execution_scope": dict(m.same.RUNTIME_COMMAND_EXECUTION_SCOPE),
        "render_sha256": H,
        "analysis": {"sample_rate": 44100},
    },
    "original": {
        "outcome": "inconclusive",
        "inconclusive_reason": "synthetic infrastructure uncertainty",
        "runtime_command_execution_observed": False,
        "runtime_command_execution_scope": None,
        "render_sha256": None,
        "analysis": None,
        "process_exit_code": None,
        "fresh_render_event_binding": "accepted",
        "fresh_render_event_binding_error": None,
    },
    "comparison": {
        "same_fixture_bytes": True,
        "same_onset_analyzer": False,
        "command_bearing_runtime_pair_observed": False,
        "classification_allowed": False,
        "parity_status": "UNKNOWN",
        "interpretation_boundary": "synthetic deferred comparison",
    },
    "derived_observation": {
        "candidate_command_execution_observed": True,
        "original_command_execution_observed": False,
        "command_bearing_runtime_pair_observed": False,
        "timing_classification_permitted": False,
        "next_evidence_boundary": "obtain original command-bearing output",
    },
}
assert m.validate_projection(projection) == projection

promoted = copy.deepcopy(projection)
promoted["parity_status"] = "PASS"
expect_value_error(
    lambda: m.validate_projection(promoted),
    "projection envelope is invalid",
)

classification = copy.deepcopy(projection)
classification["classification_allowed"] = True
expect_value_error(
    lambda: m.validate_projection(classification),
    "projection envelope is invalid",
)

bad_fixture = copy.deepcopy(projection)
bad_fixture["fixture"]["sha256"] = "not-a-hash"
expect_value_error(
    lambda: m.validate_projection(bad_fixture),
    "fixture.sha256 is not a SHA-256",
)

fake_pair = copy.deepcopy(projection)
fake_pair["comparison"]["command_bearing_runtime_pair_observed"] = True
fake_pair["derived_observation"]["command_bearing_runtime_pair_observed"] = True
expect_value_error(
    lambda: m.validate_projection(fake_pair),
    "observed runtime pair lacks original command evidence",
)

valid_pair = copy.deepcopy(projection)
valid_pair["original"].update(
    {
        "outcome": "rendered-twice",
        "inconclusive_reason": None,
        "runtime_command_execution_observed": True,
        "runtime_command_execution_scope": dict(m.same.RUNTIME_COMMAND_EXECUTION_SCOPE),
        "render_sha256": "4" * 64,
        "analysis": {"sample_rate": 44100},
    }
)
valid_pair["comparison"]["same_onset_analyzer"] = True
valid_pair["comparison"]["command_bearing_runtime_pair_observed"] = True
valid_pair["derived_observation"]["original_command_execution_observed"] = True
valid_pair["derived_observation"]["command_bearing_runtime_pair_observed"] = True
assert m.validate_projection(valid_pair) == valid_pair

wrong_analyzer = copy.deepcopy(valid_pair)
wrong_analyzer["comparison"]["same_onset_analyzer"] = False
expect_value_error(
    lambda: m.validate_projection(wrong_analyzer),
    "observed runtime pair must use the same onset analyzer",
)

with tempfile.TemporaryDirectory() as temporary:
    output = Path(temporary) / "observation.json"
    m.write_projection(output, projection)
    assert json.loads(output.read_text(encoding="utf-8")) == projection
    try:
        m.write_projection(output, projection)
    except FileExistsError:
        pass
    else:
        raise AssertionError("projection write must be no-clobber")

if m.OBSERVATION_PATH.is_file():
    committed = m.read_json(m.OBSERVATION_PATH)
    assert m.validate_projection(committed) == committed

    evidence_root = m.OBSERVATION_PATH.parent
    manifest_path = evidence_root / "canonical-raw-manifest.json"
    archive_path = evidence_root / "canonical-raw.tar.gz.b64"
    manifest = m.read_json(manifest_path)
    temporary, archive_root = m.materialize_durable_archive(
        archive_path, manifest
    )
    try:
        assert (
            m.validate_archived_evidence(archive_root, manifest, committed)
            == committed
        )
    finally:
        temporary.cleanup()

    bad_manifest = copy.deepcopy(manifest)
    bad_manifest["candidate_artifact"]["id"] += 1
    temporary, archive_root = m.materialize_durable_archive(
        archive_path, manifest
    )
    try:
        expect_value_error(
            lambda: m.validate_archive_manifest(
                archive_root, bad_manifest, committed
            ),
            "candidate_artifact provenance mismatch",
        )
    finally:
        temporary.cleanup()

source = SCRIPT.read_text(encoding="utf-8")
assert "candidate = same.validate_candidate(candidate_root)" in source
assert "recomputed = same.validate_original(candidate_root, replay_root)" in source

workflow_source = (
    ROOT / ".github" / "workflows" / "phase6c-delayed-retrigger.yml"
).read_text(encoding="utf-8")
assert "canonical-raw.tar.gz.b64" in workflow_source
assert "archive-check" in workflow_source
assert "steps.canonical.outputs.run_id" not in workflow_source

print("phase6c-delayed-retrigger-same-witness-observation: PASS")
