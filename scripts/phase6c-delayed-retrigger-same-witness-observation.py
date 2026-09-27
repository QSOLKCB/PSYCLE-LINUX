#!/usr/bin/env python3
"""Freeze and validate hosted Phase 6C same-witness render evidence."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SAME_WITNESS_SCRIPT = ROOT / "scripts" / "phase6c-delayed-retrigger-same-witness.py"
spec = importlib.util.spec_from_file_location("phase6c_same_witness", SAME_WITNESS_SCRIPT)
if spec is None or spec.loader is None:
    raise RuntimeError("could not load same-witness validator")
same = importlib.util.module_from_spec(spec)
spec.loader.exec_module(same)

CONTRACT = same.CONTRACT
WORKFLOW_NAME = "Phase 6C delayed retrigger observation"
CANDIDATE_ARTIFACT_NAME = "phase6c-delayed-retrigger-candidate"
ORIGINAL_ARTIFACT_NAME = "phase6c-delayed-retrigger-original"
OBSERVATION_PATH = (
    ROOT
    / "phase6c"
    / "evidence"
    / "sequencer-delayed-retrigger-same-witness"
    / "observation.json"
)
HEX40 = re.compile(r"^[0-9a-f]{40}$")
SHA256 = re.compile(r"^[0-9a-f]{64}$")
ARTIFACT_DIGEST = re.compile(r"^sha256:[0-9a-f]{64}$")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path}: expected JSON object")
    return value


def child(root: Path, relative: str) -> Path:
    pure = PurePosixPath(relative)
    if pure.is_absolute() or ".." in pure.parts or "\\" in relative:
        raise ValueError("unsafe artifact-relative path")
    result = (root / relative).resolve()
    result.relative_to(root.resolve())
    return result


def file_binding(root: Path, relative: str) -> dict:
    path = child(root, relative)
    if not path.is_file():
        raise ValueError(f"bound artifact file missing: {relative}")
    return {"path": relative, "sha256": digest(path.read_bytes())}


def validate_artifact_metadata(value: object, expected_name: str, label: str) -> dict:
    if not isinstance(value, dict):
        raise ValueError(f"{label} artifact metadata is not an object")
    if (
        not isinstance(value.get("id"), int)
        or isinstance(value.get("id"), bool)
        or value["id"] <= 0
        or value.get("name") != expected_name
        or not isinstance(value.get("digest"), str)
        or ARTIFACT_DIGEST.fullmatch(value["digest"]) is None
    ):
        raise ValueError(f"{label} artifact metadata is invalid")
    return value


def validate_run_metadata(value: object) -> dict:
    if not isinstance(value, dict):
        raise ValueError("workflow metadata is not an object")
    if (
        value.get("workflow_name") != WORKFLOW_NAME
        or not isinstance(value.get("workflow_run_id"), int)
        or isinstance(value.get("workflow_run_id"), bool)
        or value["workflow_run_id"] <= 0
        or value.get("workflow_event") not in {"push", "workflow_dispatch"}
        or not isinstance(value.get("workflow_head_sha"), str)
        or HEX40.fullmatch(value["workflow_head_sha"]) is None
        or not isinstance(value.get("candidate_job_id"), int)
        or isinstance(value.get("candidate_job_id"), bool)
        or value["candidate_job_id"] <= 0
        or not isinstance(value.get("original_job_id"), int)
        or isinstance(value.get("original_job_id"), bool)
        or value["original_job_id"] <= 0
    ):
        raise ValueError("workflow metadata identity is invalid")
    validate_artifact_metadata(
        value.get("candidate_artifact"), CANDIDATE_ARTIFACT_NAME, "candidate"
    )
    validate_artifact_metadata(
        value.get("original_artifact"), ORIGINAL_ARTIFACT_NAME, "original"
    )
    return value


def validate_stored_derivation(candidate_root: Path, original_root: Path) -> dict:
    """Recompute derived receipts in a temporary original-artifact copy."""
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    candidate = same.validate_candidate(candidate_root)

    stored_analysis_path = original_root / same.ORIGINAL_ANALYSIS
    stored_comparison_path = original_root / same.COMPARISON
    if not stored_analysis_path.is_file() or not stored_comparison_path.is_file():
        raise ValueError("hosted original artifact lacks same-witness derived receipts")
    stored_analysis = read_json(stored_analysis_path)
    stored_comparison = read_json(stored_comparison_path)

    with tempfile.TemporaryDirectory() as temporary:
        replay_root = Path(temporary) / "original"
        shutil.copytree(original_root, replay_root)
        for relative in (same.ORIGINAL_ANALYSIS, same.COMPARISON):
            path = replay_root / relative
            if path.exists():
                path.unlink()
        recomputed = same.validate_original(candidate_root, replay_root)
        replay_analysis = read_json(replay_root / same.ORIGINAL_ANALYSIS)
        replay_comparison = read_json(replay_root / same.COMPARISON)

    if replay_analysis != stored_analysis:
        raise ValueError("hosted original analysis differs from independent recomputation")
    if replay_comparison != stored_comparison or recomputed != stored_comparison:
        raise ValueError("hosted comparison differs from independent recomputation")

    candidate_receipt_path = candidate_root / same.CANDIDATE_RECEIPT
    original_receipt_path = original_root / same.ORIGINAL_RECEIPT
    if not original_receipt_path.is_file():
        raise ValueError("hosted original artifact lacks raw same-witness receipt")
    original_receipt = read_json(original_receipt_path)

    if candidate.get("fixture_sha256") != original_receipt.get("fixture_sha256"):
        raise ValueError("same-witness hosted fixture hashes differ")
    if stored_comparison.get("fixture_sha256") != candidate.get("fixture_sha256"):
        raise ValueError("same-witness comparison fixture hash differs")
    if stored_comparison.get("parity_status") != "UNKNOWN":
        raise ValueError("same-witness hosted comparison must retain UNKNOWN parity")
    if stored_comparison.get("classification_allowed") is not False:
        raise ValueError("same-witness hosted comparison unexpectedly allows classification")
    if stored_comparison.get("exact_onset_timing_interpretation") != "deferred":
        raise ValueError("same-witness hosted comparison interpreted timing too early")

    return {
        "candidate": candidate,
        "original_receipt": original_receipt,
        "original_analysis": stored_analysis,
        "comparison": stored_comparison,
        "candidate_receipt": file_binding(candidate_root, same.CANDIDATE_RECEIPT),
        "original_receipt_binding": file_binding(original_root, same.ORIGINAL_RECEIPT),
        "original_analysis_binding": file_binding(original_root, same.ORIGINAL_ANALYSIS),
        "comparison_binding": file_binding(original_root, same.COMPARISON),
    }


def projection_from_hosted(
    candidate_root: Path,
    original_root: Path,
    metadata: dict,
) -> dict:
    metadata = validate_run_metadata(metadata)
    hosted = validate_stored_derivation(candidate_root, original_root)
    candidate = hosted["candidate"]
    original_analysis = hosted["original_analysis"]
    comparison = hosted["comparison"]

    pair_observed = comparison.get("command_bearing_runtime_pair_observed")
    if not isinstance(pair_observed, bool):
        raise ValueError("same-witness comparison lacks pair-observed boolean")
    original_command = original_analysis.get("runtime_command_execution_observed")
    if not isinstance(original_command, bool):
        raise ValueError("same-witness original analysis lacks command-execution boolean")
    if pair_observed and not original_command:
        raise ValueError("runtime pair cannot be observed without original command evidence")

    next_boundary = (
        "interpret exact onset timing in a separately versioned comparison without "
        "expanding the established FB/FA execution scope"
        if pair_observed
        else "obtain a reproducible command-bearing original runtime output from the "
        "same four-beat witness; do not classify delayed/retrigger timing yet"
    )

    projection = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "parity_status": "UNKNOWN",
        "classification_allowed": False,
        "exact_onset_timing_interpretation": "deferred",
        "fixture": {
            "path": candidate["fixture"],
            "sha256": candidate["fixture_sha256"],
        },
        "canonical_observation": {
            "workflow_name": metadata["workflow_name"],
            "workflow_run_id": metadata["workflow_run_id"],
            "workflow_event": metadata["workflow_event"],
            "workflow_head_sha": metadata["workflow_head_sha"],
            "candidate_job_id": metadata["candidate_job_id"],
            "original_job_id": metadata["original_job_id"],
            "candidate_artifact": {
                **metadata["candidate_artifact"],
                "raw_receipt": hosted["candidate_receipt"],
            },
            "original_artifact": {
                **metadata["original_artifact"],
                "raw_receipt": hosted["original_receipt_binding"],
                "analysis_receipt": hosted["original_analysis_binding"],
                "comparison_receipt": hosted["comparison_binding"],
            },
        },
        "candidate": {
            "runtime_command_execution_observed": candidate[
                "runtime_command_execution_observed"
            ],
            "runtime_command_execution_scope": candidate[
                "runtime_command_execution_scope"
            ],
            "render_sha256": candidate["render_sha256"],
            "analysis": candidate["analysis"],
        },
        "original": {
            "outcome": original_analysis.get("outcome", "rendered-twice"),
            "inconclusive_reason": original_analysis.get("inconclusive_reason"),
            "runtime_command_execution_observed": original_command,
            "runtime_command_execution_scope": original_analysis.get(
                "runtime_command_execution_scope"
            ),
            "render_sha256": original_analysis.get("render_sha256"),
            "analysis": original_analysis.get("analysis"),
            "process_exit_code": original_analysis.get("process_exit_code"),
            "fresh_render_event_binding": original_analysis.get(
                "fresh_render_event_binding"
            ),
            "fresh_render_event_binding_error": original_analysis.get(
                "fresh_render_event_binding_error"
            ),
        },
        "comparison": {
            "same_fixture_bytes": comparison.get("same_fixture_bytes"),
            "same_onset_analyzer": comparison.get("same_onset_analyzer"),
            "command_bearing_runtime_pair_observed": pair_observed,
            "classification_allowed": comparison["classification_allowed"],
            "parity_status": comparison["parity_status"],
            "interpretation_boundary": comparison.get("interpretation_boundary"),
        },
        "derived_observation": {
            "candidate_command_execution_observed": candidate[
                "runtime_command_execution_observed"
            ],
            "original_command_execution_observed": original_command,
            "command_bearing_runtime_pair_observed": pair_observed,
            "timing_classification_permitted": False,
            "next_evidence_boundary": next_boundary,
        },
    }
    validate_projection(projection)
    return projection


def require_sha256(value: object, label: str) -> str:
    if not isinstance(value, str) or SHA256.fullmatch(value) is None:
        raise ValueError(f"{label} is not a SHA-256")
    return value


def validate_projection(value: object) -> dict:
    if not isinstance(value, dict):
        raise ValueError("same-witness observation projection is not an object")
    if (
        value.get("schema_version") != 1
        or value.get("phase") != "6C"
        or value.get("contract") != CONTRACT
        or value.get("parity_status") != "UNKNOWN"
        or value.get("classification_allowed") is not False
        or value.get("exact_onset_timing_interpretation") != "deferred"
    ):
        raise ValueError("same-witness observation projection envelope is invalid")

    fixture = value.get("fixture")
    if (
        not isinstance(fixture, dict)
        or fixture.get("path") != same.FIXTURE
    ):
        raise ValueError("same-witness observation fixture path is invalid")
    require_sha256(fixture.get("sha256"), "fixture.sha256")

    canonical = value.get("canonical_observation")
    validate_run_metadata(
        {
            "workflow_name": canonical.get("workflow_name") if isinstance(canonical, dict) else None,
            "workflow_run_id": canonical.get("workflow_run_id") if isinstance(canonical, dict) else None,
            "workflow_event": canonical.get("workflow_event") if isinstance(canonical, dict) else None,
            "workflow_head_sha": canonical.get("workflow_head_sha") if isinstance(canonical, dict) else None,
            "candidate_job_id": canonical.get("candidate_job_id") if isinstance(canonical, dict) else None,
            "original_job_id": canonical.get("original_job_id") if isinstance(canonical, dict) else None,
            "candidate_artifact": canonical.get("candidate_artifact") if isinstance(canonical, dict) else None,
            "original_artifact": canonical.get("original_artifact") if isinstance(canonical, dict) else None,
        }
    )
    for role in ("candidate_artifact", "original_artifact"):
        artifact = canonical[role]
        raw = artifact.get("raw_receipt")
        if (
            not isinstance(raw, dict)
            or not isinstance(raw.get("path"), str)
        ):
            raise ValueError(f"{role}.raw_receipt is invalid")
        require_sha256(raw.get("sha256"), f"{role}.raw_receipt.sha256")
    original_artifact = canonical["original_artifact"]
    for field in ("analysis_receipt", "comparison_receipt"):
        binding = original_artifact.get(field)
        if not isinstance(binding, dict) or not isinstance(binding.get("path"), str):
            raise ValueError(f"original_artifact.{field} is invalid")
        require_sha256(binding.get("sha256"), f"original_artifact.{field}.sha256")

    candidate = value.get("candidate")
    original = value.get("original")
    comparison = value.get("comparison")
    derived = value.get("derived_observation")
    if not all(isinstance(item, dict) for item in (candidate, original, comparison, derived)):
        raise ValueError("same-witness projection sections are malformed")
    if candidate.get("runtime_command_execution_observed") is not True:
        raise ValueError("candidate command-bearing execution must remain observed")
    require_sha256(candidate.get("render_sha256"), "candidate.render_sha256")
    if comparison.get("same_fixture_bytes") is not True:
        raise ValueError("same-witness projection no longer proves identical fixture bytes")
    if comparison.get("classification_allowed") is not False:
        raise ValueError("same-witness projection cannot allow classification")
    if comparison.get("parity_status") != "UNKNOWN":
        raise ValueError("same-witness projection must retain UNKNOWN parity")
    pair = comparison.get("command_bearing_runtime_pair_observed")
    if not isinstance(pair, bool):
        raise ValueError("same-witness pair-observed field must be boolean")
    if derived.get("command_bearing_runtime_pair_observed") is not pair:
        raise ValueError("derived pair-observed field disagrees with comparison")
    if derived.get("candidate_command_execution_observed") is not True:
        raise ValueError("derived candidate execution observation changed")
    if derived.get("original_command_execution_observed") is not original.get(
        "runtime_command_execution_observed"
    ):
        raise ValueError("derived original execution observation changed")
    if derived.get("timing_classification_permitted") is not False:
        raise ValueError("timing classification must remain deferred")
    if pair:
        if original.get("runtime_command_execution_observed") is not True:
            raise ValueError("observed runtime pair lacks original command evidence")
        require_sha256(original.get("render_sha256"), "original.render_sha256")
        if comparison.get("same_onset_analyzer") is not True:
            raise ValueError("observed runtime pair must use the same onset analyzer")
    return value


def write_projection(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)

    project = sub.add_parser("project")
    project.add_argument("candidate_root", type=Path)
    project.add_argument("original_root", type=Path)
    project.add_argument("metadata", type=Path)
    project.add_argument("output", type=Path)

    check = sub.add_parser("check")
    check.add_argument("observation", type=Path, nargs="?", default=OBSERVATION_PATH)

    args = parser.parse_args()
    if args.mode == "project":
        metadata = read_json(args.metadata)
        value = projection_from_hosted(args.candidate_root, args.original_root, metadata)
        write_projection(args.output, value)
    else:
        value = validate_projection(read_json(args.observation))
    print(json.dumps(value, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
