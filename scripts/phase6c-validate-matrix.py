#!/usr/bin/env python3
"""Validate the Phase 6C three-way compatibility matrix.

The important invariant is epistemic, not cosmetic: candidate/C-Psycle evidence alone
must never promote a row out of UNKNOWN. PASS, DIFFERENT and MISSING are claims about
the pinned original-Psycle compatibility target and therefore require reproducible
original-reference evidence, candidate evidence, and a versioned comparison verdict.
"""

from __future__ import annotations

import hashlib
import json
import pathlib
import re
import sys

MATRIX = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path(
    "phase6c/compatibility-matrix.json"
)
REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
EVIDENCE_ROOT = (REPO_ROOT / "phase6c" / "evidence").resolve()

EXPECTED_ORIGINAL_FILE = "PsycleInstallerx86-1.12.0.exe"
EXPECTED_ORIGINAL_SHA256 = (
    "f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769"
)
EXPECTED_ORIGINAL_SIZE = 9322919
EXPECTED_CANDIDATE_BASELINE = (
    "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
)
EXPECTED_ORIGINAL_SOURCE_IDENTITIES = {
    "project-io-psy2-parse": {
        "receipt_sha256": "1b9d05dd5caafc8120ade51e28086d02f05800bcba0479aa4325680333fb6b74",
        "procedure_sha256": "b1d0a80417d8589d458e35386b9051b039210144df707e1aea57ba65ca6b5a6d",
        "executable_sha256": "fdb130d2465d5b4a4acfbfe0bfb2368926380fe07a383c4774f0951591b6d6b6",
        "artifact_digest": "sha256:20957d9e23733bc3ce724611734f07f9d90a49450b66152c68ae186c1dd62595",
    }
}
EXPECTED_WORKFLOW_ATTRIBUTION = {
    "project-io-psy2-parse": {
        "workflow": "Phase 6C compatibility matrix",
        "run_id": 35342946830,
        "event": "pull_request",
        "head_sha": "1f9afb8c8ac675f5bc06b172d3415fbaa136e201",
        "merged_main_sha": "af23c420dcd6c7c62f4177f82eadca645db004a2",
        "original_job_id": 105593457296,
        "original_artifact_id": 10546630670,
        "candidate_job_id": 105592884470,
        "candidate_artifact_id": 10545756966,
    }
}
EXPECTED_ORIGINAL_CLASSIFICATION_FIELDS = {
    "project-io-psy2-parse": {
        "reference_executable_sha256": "fdb130d2465d5b4a4acfbfe0bfb2368926380fe07a383c4774f0951591b6d6b6",
        "observation": "stable-native-window-evidence-identifies-loaded-fixture-without-error",
        "load_result": "accepted",
        "load_evidence_marker": "phase4-historical-psy2.psy",
        "stable_marker_polls": 38,
        "error_marker": None,
        "application_error_marker": None,
        "error_marker_scope": "none",
        "ui_automation_diagnostics": [],
        "startup_bootstrap": {
            "settings_dialog_seen": True,
            "signature_verified": True,
            "attempted": True,
            "action": "invoke-ok",
            "dismissed": True,
            "outcome": "dismissed",
            "diagnostics": [],
        },
        "environment_bootstrap": {
            "directsound": {
                "dialog_seen": True,
                "signature_verified": True,
                "attempted": True,
                "action": "invoke-ok-win32-bm-click",
                "dismissed": True,
                "outcome": "dismissed",
                "diagnostics": [],
            }
        },
        "runtime_identity_diagnostics": [],
        "main_window_seen": True,
        "exit_code_before_termination": None,
        "process_running_before_termination": True,
        "termination": "killed-without-closeable-main-window",
        "original_psycle_observed": True,
        "parity_status": "UNKNOWN",
    }
}
EXPECTED_CANDIDATE_SOURCE_IDENTITIES = {
    "project-io-psy2-parse": {
        "receipt_sha256": "feee397beec6babfc715ad05be08037789e881cecb77abb174e934dbbc42a230",
        "executable_sha256": "79b8bc04918039cca8be4db0909f826b55ad6538b6edf1f1689ec0eb8dcac330",
        "artifact_digest": "sha256:b8d206aa16a2b820149e1e83f76c671fb0b584cea59ed8f69941846e183b6fcf",
    }
}
ALLOWED_STATUS = {"PASS", "DIFFERENT", "MISSING", "UNKNOWN"}
CLASSIFIED_STATUS = ALLOWED_STATUS - {"UNKNOWN"}
PARSE_CONTRACT_IDS = {
    "project-io-psy2-parse",
    "project-io-psy3-parse",
}
ACCEPTED_ORIGINAL_TERMINATION = {
    "killed-without-closeable-main-window",
    "killed-after-observation",
    "closed-after-observation",
    "killed-after-close-error",
}
REQUIRED_IDS = {
    "project-io-psy2-parse",
    "project-io-psy3-parse",
    "project-io-serialization-roundtrip",
    "project-io-malformed-files",
    "sequencer-pattern-order",
    "sequencer-bpm-lpb-tick",
    "sequencer-delayed-retrigger",
    "sampler-ps1",
    "sampler-xmsampler",
    "routing-mixer-master",
    "native-abi-identity",
    "native-parameter-state",
    "plugin-opaque-state",
    "plugin-rescan-cache",
    "midi-routing",
    "automation-tweak",
    "sample-wav-loading",
    "render-bounce",
    "missing-machine-recovery",
    "historical-song-playback",
}
REQUIRED_ORIGINAL_FIELDS = ("reference_build", "fixture", "procedure", "observation")
REQUIRED_CANDIDATE_FIELDS = ("snapshot", "fixture", "procedure", "observation")
PLACEHOLDER_RE = re.compile(
    r"(?:^|[\s:/_\-])(pending|todo|tbd|unknown|placeholder|unverified|"
    r"not[\s_-]+yet|not[\s_-]+observed)(?:$|[\s:/_\-])",
    re.IGNORECASE,
)
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def die(message: str) -> None:
    raise SystemExit(f"phase6c-validate-matrix: {message}")


def require_nonempty_mapping_fields(
    mapping: object, fields: tuple[str, ...], context: str
) -> None:
    if not isinstance(mapping, dict):
        die(f"{context} must be an object")
    for field in fields:
        value = mapping.get(field)
        if not isinstance(value, str) or not value.strip():
            die(f"{context}.{field} must be a non-empty string")


def require_concrete_evidence_value(value: str, context: str) -> None:
    if PLACEHOLDER_RE.search(value.strip()):
        die(f"{context} contains placeholder evidence: {value!r}")


def require_sha256(value: object, context: str) -> str:
    if not isinstance(value, str) or SHA256_RE.fullmatch(value) is None:
        die(f"{context} must be a lowercase SHA-256 hex digest")
    return value


def resolve_versioned_receipt(reference: str, context: str) -> pathlib.Path:
    """Resolve a classification receipt committed below phase6c/evidence/."""
    receipt_ref = pathlib.PurePosixPath(reference.strip())
    if receipt_ref.is_absolute() or ".." in receipt_ref.parts:
        die(f"{context} must be a repository-relative receipt path")
    if receipt_ref.suffix != ".json":
        die(f"{context} must reference a JSON receipt")
    if receipt_ref.parts[:2] != ("phase6c", "evidence"):
        die(f"{context} must reference a receipt below phase6c/evidence/")

    receipt_path = (REPO_ROOT / pathlib.Path(*receipt_ref.parts)).resolve()
    try:
        receipt_path.relative_to(EVIDENCE_ROOT)
    except ValueError:
        die(f"{context} escapes the Phase 6C evidence directory")
    if not receipt_path.is_file():
        die(f"{context} references missing receipt: {reference}")
    return receipt_path


def load_versioned_receipt(reference: str, context: str) -> dict[str, object]:
    receipt_path = resolve_versioned_receipt(reference, context)
    try:
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        die(f"{context} receipt is invalid JSON: {exc}")
    if not isinstance(receipt, dict):
        die(f"{context} receipt must be a JSON object")
    return receipt


def validate_classification_receipt(
    mapping: dict[str, object],
    row_id: str,
    role: str,
    context: str,
) -> tuple[str, str, dict[str, object]]:
    """Validate one versioned observation receipt and return ref, fixture hash and receipt."""
    for field in ("fixture", "procedure"):
        value = mapping[field]
        assert isinstance(value, str)
        require_concrete_evidence_value(value, f"{context}.{field}")

    observation_ref = mapping["observation"]
    assert isinstance(observation_ref, str)
    receipt = load_versioned_receipt(observation_ref, f"{context}.observation")

    if receipt.get("schema_version") != 1 or receipt.get("phase") != "6C":
        die(f"{context}.observation receipt has unexpected schema_version/phase")
    if receipt.get("contract") != row_id:
        die(f"{context}.observation receipt is bound to the wrong contract")
    if receipt.get("evidence_role") != role:
        die(f"{context}.observation receipt is bound to the wrong evidence role")

    if role == "original":
        if receipt.get("reference_build") != "Psycle 1.12.0 x86":
            die(f"{context}.observation receipt is bound to the wrong reference build")
        if receipt.get("reference_file") != EXPECTED_ORIGINAL_FILE:
            die(f"{context}.observation receipt is bound to the wrong reference file")
        if receipt.get("reference_installer_sha256") != EXPECTED_ORIGINAL_SHA256:
            die(f"{context}.observation receipt has the wrong reference installer SHA-256")
        if receipt.get("reference_installer_size_bytes") != EXPECTED_ORIGINAL_SIZE:
            die(f"{context}.observation receipt has the wrong reference installer size")

        expected_source = EXPECTED_ORIGINAL_SOURCE_IDENTITIES.get(row_id)
        if not isinstance(expected_source, dict):
            die(f"{context}.observation receipt lacks a pinned source identity")
        source_evidence = receipt.get("source_evidence")
        if not isinstance(source_evidence, dict):
            die(f"{context}.observation receipt lacks source_evidence")
        source_receipt_sha256 = require_sha256(
            source_evidence.get("artifact_receipt_sha256"),
            f"{context}.observation receipt source artifact_receipt_sha256",
        )
        if source_receipt_sha256 != expected_source["receipt_sha256"]:
            die(f"{context}.observation receipt is bound to the wrong source receipt")
        if source_evidence.get("artifact_digest") != expected_source["artifact_digest"]:
            die(f"{context}.observation receipt is bound to the wrong source artifact")

        expected_attribution = EXPECTED_WORKFLOW_ATTRIBUTION.get(row_id)
        if not isinstance(expected_attribution, dict):
            die(f"{context}.observation receipt lacks pinned workflow attribution")
        original_attribution_bindings = {
            "workflow": expected_attribution["workflow"],
            "run_id": expected_attribution["run_id"],
            "event": expected_attribution["event"],
            "head_sha": expected_attribution["head_sha"],
            "merged_main_sha": expected_attribution["merged_main_sha"],
            "job_id": expected_attribution["original_job_id"],
            "artifact_id": expected_attribution["original_artifact_id"],
        }
        for field, expected_value in original_attribution_bindings.items():
            if source_evidence.get(field) != expected_value:
                die(
                    f"{context}.observation receipt source_evidence.{field} "
                    "does not match the pinned workflow attribution"
                )

        if source_evidence.get("projection_type") != (
            "field-preserving-classification-projection"
        ):
            die(f"{context}.observation receipt has unsupported projection type")
        projection_note = source_evidence.get("projection_note")
        if not isinstance(projection_note, str) or not projection_note.strip():
            die(f"{context}.observation receipt lacks projection provenance")
        require_concrete_evidence_value(
            projection_note, f"{context}.observation receipt projection_note"
        )
        procedure = receipt.get("procedure")
        if not isinstance(procedure, str):
            die(f"{context}.observation receipt procedure must be a string")
        procedure_sha256 = hashlib.sha256(procedure.encode("utf-8")).hexdigest()
        if procedure_sha256 != expected_source["procedure_sha256"]:
            die(f"{context}.observation receipt procedure differs from source artifact")

        expected_classification = EXPECTED_ORIGINAL_CLASSIFICATION_FIELDS.get(row_id)
        if not isinstance(expected_classification, dict):
            die(f"{context}.observation receipt lacks pinned classification fields")
        for field, expected_value in expected_classification.items():
            if field not in receipt or receipt[field] != expected_value:
                die(
                    f"{context}.observation receipt {field} differs from "
                    "the pinned source classification"
                )
    elif role == "candidate":
        if receipt.get("snapshot") != EXPECTED_CANDIDATE_BASELINE:
            die(f"{context}.observation receipt is bound to the wrong candidate snapshot")
        expected_source = EXPECTED_CANDIDATE_SOURCE_IDENTITIES.get(row_id)
        if not isinstance(expected_source, dict):
            die(f"{context}.observation receipt lacks a pinned candidate source identity")
        executable_sha256 = require_sha256(
            receipt.get("executable_sha256"),
            f"{context}.observation receipt executable_sha256",
        )
        if executable_sha256 != expected_source["executable_sha256"]:
            die(f"{context}.observation receipt executable SHA-256 is not pinned")
        receipt_path = resolve_versioned_receipt(
            observation_ref, f"{context}.observation"
        )
        committed_receipt_sha256 = hashlib.sha256(receipt_path.read_bytes()).hexdigest()
        if committed_receipt_sha256 != expected_source["receipt_sha256"]:
            die(f"{context}.observation receipt differs from the source artifact receipt")
        if receipt.get("original_psycle_observed") is not False:
            die(f"{context}.observation receipt must remain candidate-only")
        if receipt.get("parity_status") != "UNKNOWN":
            die(f"{context}.observation receipt candidate parity_status must remain UNKNOWN")
        exit_code = receipt.get("exit_code")
        if not isinstance(exit_code, int) or isinstance(exit_code, bool):
            die(f"{context}.observation receipt exit_code must be an integer")
    else:
        die(f"{context} has unsupported evidence role: {role}")

    if receipt.get("fixture") != mapping["fixture"]:
        die(f"{context}.observation receipt fixture does not match the matrix")
    if receipt.get("procedure") != mapping["procedure"]:
        die(f"{context}.observation receipt procedure does not match the matrix")

    fixture_sha256 = require_sha256(
        receipt.get("fixture_sha256"),
        f"{context}.observation receipt fixture_sha256",
    )
    receipt_observation = receipt.get("observation")
    if not isinstance(receipt_observation, str) or not receipt_observation.strip():
        die(f"{context}.observation receipt must contain a concrete observation")
    require_concrete_evidence_value(
        receipt_observation, f"{context}.observation receipt observation"
    )
    return observation_ref, fixture_sha256, receipt


def validate_parse_pass_semantics(
    row_id: str,
    original_receipt: dict[str, object],
    candidate_receipt: dict[str, object],
    comparison: dict[str, object],
) -> None:
    """Require concrete successful parse observations before a parse row may PASS."""
    if original_receipt.get("original_psycle_observed") is not True:
        die(f"{row_id} PASS original receipt was not observed on original Psycle")
    if original_receipt.get("load_result") != "accepted":
        die(f"{row_id} PASS original receipt is not an accepted load")
    if original_receipt.get("error_marker_scope") != "none":
        die(f"{row_id} PASS original receipt contains scoped application errors")
    if original_receipt.get("error_marker") is not None:
        die(f"{row_id} PASS original receipt contains an error marker")
    if original_receipt.get("application_error_marker") is not None:
        die(f"{row_id} PASS original receipt contains an application error marker")
    if original_receipt.get("ui_automation_diagnostics") != []:
        die(f"{row_id} PASS original receipt contains UI Automation diagnostics")
    if original_receipt.get("runtime_identity_diagnostics") != []:
        die(f"{row_id} PASS original receipt contains runtime identity diagnostics")

    startup_bootstrap = original_receipt.get("startup_bootstrap")
    if not isinstance(startup_bootstrap, dict):
        die(f"{row_id} PASS original receipt lacks startup bootstrap state")

    startup_seen = startup_bootstrap.get("settings_dialog_seen")
    startup_signature = startup_bootstrap.get("signature_verified")
    startup_attempted = startup_bootstrap.get("attempted")
    startup_dismissed = startup_bootstrap.get("dismissed")
    startup_action = startup_bootstrap.get("action")
    startup_outcome = startup_bootstrap.get("outcome")
    startup_diagnostics = startup_bootstrap.get("diagnostics")
    if any(
        not isinstance(value, bool)
        for value in (
            startup_seen,
            startup_signature,
            startup_attempted,
            startup_dismissed,
        )
    ):
        die(f"{row_id} PASS original receipt has invalid settings bootstrap booleans")
    if startup_diagnostics != []:
        die(f"{row_id} PASS original receipt has settings bootstrap diagnostics")
    if startup_seen:
        if not (
            startup_signature
            and startup_attempted
            and startup_dismissed
            and startup_action == "invoke-ok"
            and startup_outcome == "dismissed"
        ):
            die(
                f"{row_id} PASS original receipt has inconsistent successful "
                "settings bootstrap state"
            )
    elif not (
        startup_signature is False
        and startup_attempted is False
        and startup_dismissed is False
        and startup_action is None
        and startup_outcome == "not-seen"
    ):
        die(
            f"{row_id} PASS original receipt has inconsistent not-seen "
            "settings bootstrap state"
        )

    environment_bootstrap = original_receipt.get("environment_bootstrap")
    if not isinstance(environment_bootstrap, dict):
        die(f"{row_id} PASS original receipt lacks environment bootstrap state")
    directsound = environment_bootstrap.get("directsound")
    if not isinstance(directsound, dict):
        die(f"{row_id} PASS original receipt lacks DirectSound bootstrap state")

    directsound_seen = directsound.get("dialog_seen")
    directsound_signature = directsound.get("signature_verified")
    directsound_attempted = directsound.get("attempted")
    directsound_dismissed = directsound.get("dismissed")
    directsound_action = directsound.get("action")
    directsound_outcome = directsound.get("outcome")
    directsound_diagnostics = directsound.get("diagnostics")
    if any(
        not isinstance(value, bool)
        for value in (
            directsound_seen,
            directsound_signature,
            directsound_attempted,
            directsound_dismissed,
        )
    ):
        die(f"{row_id} PASS original receipt has invalid DirectSound bootstrap booleans")
    if directsound_diagnostics != []:
        die(f"{row_id} PASS original receipt has DirectSound bootstrap diagnostics")
    if directsound_seen:
        if not (
            directsound_signature
            and directsound_attempted
            and directsound_dismissed
            and directsound_action
            in {
                "invoke-ok",
                "invoke-ok-win32-wm-command",
                "invoke-ok-win32-bm-click",
            }
            and directsound_outcome == "dismissed"
        ):
            die(
                f"{row_id} PASS original receipt has inconsistent successful "
                "DirectSound bootstrap state"
            )
    elif not (
        directsound_signature is False
        and directsound_attempted is False
        and directsound_dismissed is False
        and directsound_action is None
        and directsound_outcome == "not-seen"
    ):
        die(
            f"{row_id} PASS original receipt has inconsistent not-seen "
            "DirectSound bootstrap state"
        )

    load_marker = original_receipt.get("load_evidence_marker")
    fixture_ref = original_receipt.get("fixture")
    if not isinstance(load_marker, str) or not load_marker.strip():
        die(f"{row_id} PASS original receipt lacks a concrete load marker")
    if not isinstance(fixture_ref, str) or not fixture_ref.strip():
        die(f"{row_id} PASS original receipt lacks its fixture identity")
    expected_marker = pathlib.PurePosixPath(fixture_ref.replace("\\", "/")).name
    if load_marker != expected_marker:
        die(
            f"{row_id} PASS original load marker {load_marker!r} does not "
            f"match fixture basename {expected_marker!r}"
        )

    stable_polls = original_receipt.get("stable_marker_polls")
    if (
        not isinstance(stable_polls, int)
        or isinstance(stable_polls, bool)
        or stable_polls < 4
    ):
        die(f"{row_id} PASS original receipt lacks the stable marker window")
    if original_receipt.get("main_window_seen") is not True:
        die(f"{row_id} PASS original receipt lacks an observed main window")
    if original_receipt.get("exit_code_before_termination") is not None:
        die(f"{row_id} PASS original receipt exited before harness termination")
    if original_receipt.get("process_running_before_termination") is not True:
        die(f"{row_id} PASS original receipt was not live at harness termination")
    termination = original_receipt.get("termination")
    if termination not in ACCEPTED_ORIGINAL_TERMINATION:
        die(
            f"{row_id} PASS original receipt lacks successful "
            "harness-controlled termination"
        )

    if candidate_receipt.get("observation") != "load-and-clean-exit":
        die(f"{row_id} PASS candidate receipt is not a clean load observation")
    candidate_exit_code = candidate_receipt.get("exit_code")
    if (
        not isinstance(candidate_exit_code, int)
        or isinstance(candidate_exit_code, bool)
        or candidate_exit_code != 0
    ):
        die(f"{row_id} PASS candidate receipt does not have integer exit_code=0")

    comparison_exit_code = comparison.get("candidate_exit_code")
    if (
        not isinstance(comparison_exit_code, int)
        or isinstance(comparison_exit_code, bool)
        or comparison_exit_code != 0
    ):
        die(
            f"{row_id}.comparison receipt does not bind an integer "
            "candidate_exit_code=0"
        )

def canonical_candidate_parse_result(
    candidate_receipt: dict[str, object],
) -> str:
    observation = candidate_receipt.get("observation")
    if observation == "load-and-clean-exit":
        return "accepted"
    if observation == "load-returned-nonzero":
        return "rejected"
    if observation == "capability-missing":
        return "missing"
    return "inconclusive"


def validate_parse_classification_semantics(
    row_id: str,
    status: str,
    original_receipt: dict[str, object],
    candidate_receipt: dict[str, object],
    comparison: dict[str, object],
) -> None:
    """Require verdict-specific parse evidence rather than trusting the label."""
    expected_bindings = {
        "original_observation": original_receipt.get("observation"),
        "original_load_result": original_receipt.get("load_result"),
        "candidate_observation": candidate_receipt.get("observation"),
        "candidate_exit_code": candidate_receipt.get("exit_code"),
    }
    for field, expected in expected_bindings.items():
        if comparison.get(field) != expected:
            die(
                f"{row_id}.comparison receipt {field} does not bind the "
                "underlying parse observations"
            )

    if status == "PASS":
        validate_parse_pass_semantics(
            row_id,
            original_receipt,
            candidate_receipt,
            comparison,
        )
        return

    original_result = original_receipt.get("load_result")
    candidate_result = canonical_candidate_parse_result(candidate_receipt)

    if status == "DIFFERENT":
        if original_result not in {"accepted", "rejected"}:
            die(f"{row_id} DIFFERENT original parse result is not conclusive")
        if candidate_result not in {"accepted", "rejected"}:
            die(f"{row_id} DIFFERENT candidate parse result is not conclusive")
        if original_result == candidate_result:
            die(
                f"{row_id} DIFFERENT verdict lacks differing conclusive "
                "parse outcomes"
            )
        return

    if status == "MISSING":
        if original_result != "accepted":
            die(f"{row_id} MISSING requires an accepted original capability")
        if candidate_result != "missing":
            die(
                f"{row_id} MISSING requires candidate observation "
                "'capability-missing'"
            )
        if comparison.get("candidate_missing_capability") is not True:
            die(
                f"{row_id}.comparison receipt must explicitly bind "
                "candidate_missing_capability=true"
            )
        return

    die(f"{row_id} unsupported classified parse status: {status!r}")


def validate_comparison_receipt(
    row: dict[str, object],
    row_id: str,
    status: str,
    original_ref: str,
    candidate_ref: str,
    original_fixture_sha256: str,
    candidate_fixture_sha256: str,
    original_receipt: dict[str, object],
    candidate_receipt: dict[str, object],
) -> None:
    """Bind the matrix status to a versioned comparison verdict over both receipts."""
    comparison_ref = row.get("comparison")
    if not isinstance(comparison_ref, str) or not comparison_ref.strip():
        die(f"{row_id}.comparison must reference a versioned comparison receipt")
    comparison = load_versioned_receipt(comparison_ref, f"{row_id}.comparison")

    if comparison.get("schema_version") != 1 or comparison.get("phase") != "6C":
        die(f"{row_id}.comparison receipt has unexpected schema_version/phase")
    if comparison.get("scope") != "compatibility-comparison":
        die(f"{row_id}.comparison receipt has the wrong scope")
    if comparison.get("contract") != row_id:
        die(f"{row_id}.comparison receipt is bound to the wrong contract")
    if comparison.get("original_receipt") != original_ref:
        die(f"{row_id}.comparison receipt references the wrong original receipt")
    if comparison.get("candidate_receipt") != candidate_ref:
        die(f"{row_id}.comparison receipt references the wrong candidate receipt")
    if comparison.get("original_reference_build") != "Psycle 1.12.0 x86":
        die(f"{row_id}.comparison receipt is bound to the wrong original build")
    if comparison.get("original_installer_sha256") != EXPECTED_ORIGINAL_SHA256:
        die(f"{row_id}.comparison receipt has the wrong original installer SHA-256")
    if comparison.get("original_installer_size_bytes") != EXPECTED_ORIGINAL_SIZE:
        die(f"{row_id}.comparison receipt has the wrong original installer size")
    if comparison.get("candidate_snapshot") != EXPECTED_CANDIDATE_BASELINE:
        die(f"{row_id}.comparison receipt is bound to the wrong candidate snapshot")

    original_source_evidence = original_receipt.get("source_evidence")
    if not isinstance(original_source_evidence, dict):
        die(f"{row_id} original receipt lacks source_evidence")
    original_source_receipt_sha256 = require_sha256(
        original_source_evidence.get("artifact_receipt_sha256"),
        f"{row_id} original source receipt SHA-256",
    )
    if comparison.get("original_source_receipt_sha256") != original_source_receipt_sha256:
        die(
            f"{row_id}.comparison receipt does not bind the original source "
            "receipt SHA-256"
        )

    expected_original_source = EXPECTED_ORIGINAL_SOURCE_IDENTITIES.get(row_id)
    if not isinstance(expected_original_source, dict):
        die(f"{row_id} comparison lacks a pinned original source identity")
    original_executable_sha256 = require_sha256(
        original_receipt.get("reference_executable_sha256"),
        f"{row_id} original executable SHA-256",
    )
    if (
        original_executable_sha256 != expected_original_source["executable_sha256"]
        or comparison.get("original_executable_sha256")
        != expected_original_source["executable_sha256"]
    ):
        die(f"{row_id}.comparison receipt does not bind the original executable")

    source_evidence = comparison.get("source_evidence")
    if not isinstance(source_evidence, dict):
        die(f"{row_id}.comparison receipt lacks source_evidence")

    expected_attribution = EXPECTED_WORKFLOW_ATTRIBUTION.get(row_id)
    if not isinstance(expected_attribution, dict):
        die(f"{row_id}.comparison receipt lacks pinned workflow attribution")
    comparison_attribution_bindings = {
        "workflow": expected_attribution["workflow"],
        "workflow_run_id": expected_attribution["run_id"],
        "workflow_event": expected_attribution["event"],
        "workflow_head_sha": expected_attribution["head_sha"],
        "merged_main_sha": expected_attribution["merged_main_sha"],
        "original_job_id": expected_attribution["original_job_id"],
        "original_artifact_id": expected_attribution["original_artifact_id"],
        "candidate_job_id": expected_attribution["candidate_job_id"],
        "candidate_artifact_id": expected_attribution["candidate_artifact_id"],
    }
    for field, expected_value in comparison_attribution_bindings.items():
        if source_evidence.get(field) != expected_value:
            die(
                f"{row_id}.comparison receipt source_evidence.{field} "
                "does not match the pinned workflow attribution"
            )

    original_projection_bindings = {
        "workflow": "workflow",
        "run_id": "workflow_run_id",
        "event": "workflow_event",
        "head_sha": "workflow_head_sha",
        "merged_main_sha": "merged_main_sha",
        "job_id": "original_job_id",
        "artifact_id": "original_artifact_id",
    }
    for original_field, comparison_field in original_projection_bindings.items():
        if (
            original_source_evidence.get(original_field)
            != source_evidence.get(comparison_field)
        ):
            die(
                f"{row_id}.comparison receipt attribution disagrees with "
                f"the original projection for {original_field}"
            )

    if (
        source_evidence.get("original_artifact_digest")
        != expected_original_source["artifact_digest"]
    ):
        die(f"{row_id}.comparison receipt has the wrong original artifact digest")
    expected_original_artifact_receipt = pathlib.PurePosixPath(original_ref).name
    if (
        source_evidence.get("original_artifact_receipt")
        != expected_original_artifact_receipt
    ):
        die(f"{row_id}.comparison receipt has the wrong original artifact receipt")

    expected_candidate_source = EXPECTED_CANDIDATE_SOURCE_IDENTITIES.get(row_id)
    if not isinstance(expected_candidate_source, dict):
        die(f"{row_id} comparison lacks a pinned candidate source identity")
    if (
        comparison.get("candidate_source_receipt_sha256")
        != expected_candidate_source["receipt_sha256"]
    ):
        die(
            f"{row_id}.comparison receipt does not bind the candidate source "
            "receipt SHA-256"
        )
    candidate_executable_sha256 = require_sha256(
        candidate_receipt.get("executable_sha256"),
        f"{row_id} candidate executable SHA-256",
    )
    if (
        comparison.get("candidate_executable_sha256")
        != expected_candidate_source["executable_sha256"]
        or candidate_executable_sha256
        != expected_candidate_source["executable_sha256"]
    ):
        die(f"{row_id}.comparison receipt does not bind the candidate executable")
    if (
        source_evidence.get("candidate_artifact_digest")
        != expected_candidate_source["artifact_digest"]
    ):
        die(f"{row_id}.comparison receipt has the wrong candidate artifact digest")
    expected_candidate_artifact_receipt = pathlib.PurePosixPath(candidate_ref).name
    if (
        source_evidence.get("candidate_artifact_receipt")
        != expected_candidate_artifact_receipt
    ):
        die(f"{row_id}.comparison receipt has the wrong candidate artifact receipt")

    if original_fixture_sha256 != candidate_fixture_sha256:
        die(f"{row_id} original/candidate receipts do not identify the same fixture")
    if comparison.get("fixture_sha256") != original_fixture_sha256:
        die(f"{row_id}.comparison receipt is bound to the wrong fixture SHA-256")

    verdict = comparison.get("verdict")
    if verdict not in CLASSIFIED_STATUS:
        die(f"{row_id}.comparison receipt has invalid verdict: {verdict!r}")
    if verdict != status:
        die(
            f"{row_id} matrix status {status!r} does not match comparison "
            f"verdict {verdict!r}"
        )

    if row_id in PARSE_CONTRACT_IDS:
        validate_parse_classification_semantics(
            row_id,
            status,
            original_receipt,
            candidate_receipt,
            comparison,
        )

    for field in ("comparison_method", "rationale"):
        value = comparison.get(field)
        if not isinstance(value, str) or not value.strip():
            die(f"{row_id}.comparison receipt {field} must be a non-empty string")
        require_concrete_evidence_value(value, f"{row_id}.comparison receipt {field}")


def main() -> int:
    try:
        matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
    except FileNotFoundError:
        die(f"missing matrix: {MATRIX}")
    except json.JSONDecodeError as exc:
        die(f"invalid JSON: {exc}")

    if matrix.get("schema_version") != 1 or matrix.get("phase") != "6C":
        die("unexpected schema_version/phase")

    identities = matrix.get("identities")
    if not isinstance(identities, dict):
        die("identities must be an object")

    original = identities.get("original_psycle", {})
    if original.get("version") != "1.12.0 x86":
        die("original reference version changed")
    if original.get("file") != EXPECTED_ORIGINAL_FILE:
        die("original reference filename changed")
    if original.get("size_bytes") != EXPECTED_ORIGINAL_SIZE:
        die("original reference size changed")
    if original.get("sha256") != EXPECTED_ORIGINAL_SHA256:
        die("original reference SHA-256 changed")
    if original.get("redistributed") is not False:
        die("original executable must remain non-redistributed")

    candidate = identities.get("candidate_cpp", {})
    if candidate.get("phase6b_baseline_sha256") != EXPECTED_CANDIDATE_BASELINE:
        die("candidate Phase 6B baseline identity changed")
    if candidate.get("sanitized_root") != "psycle-cpp-r12005-sanitized":
        die("candidate sanitized root changed")

    if set(matrix.get("status_values", [])) != ALLOWED_STATUS:
        die("status_values must be exactly PASS/DIFFERENT/MISSING/UNKNOWN")

    contracts = matrix.get("contracts")
    if not isinstance(contracts, list):
        die("contracts must be an array")

    seen: set[str] = set()
    non_unknown = 0
    for index, row in enumerate(contracts):
        if not isinstance(row, dict):
            die(f"contracts[{index}] must be an object")
        row_id = row.get("id")
        if not isinstance(row_id, str) or not row_id:
            die(f"contracts[{index}] has no id")
        if row_id in seen:
            die(f"duplicate contract id: {row_id}")
        seen.add(row_id)

        for required in (
            "subsystem",
            "contract",
            "status",
            "original",
            "candidate",
            "cpsycle",
        ):
            if required not in row:
                die(f"{row_id} missing required field: {required}")

        status = row["status"]
        if status not in ALLOWED_STATUS:
            die(f"{row_id} has invalid status: {status!r}")

        if status != "UNKNOWN":
            non_unknown += 1
            require_nonempty_mapping_fields(
                row["original"], REQUIRED_ORIGINAL_FIELDS, f"{row_id}.original"
            )
            require_nonempty_mapping_fields(
                row["candidate"], REQUIRED_CANDIDATE_FIELDS, f"{row_id}.candidate"
            )
            if row["original"]["reference_build"] != "Psycle 1.12.0 x86":
                die(f"{row_id} non-UNKNOWN claim is not bound to the primary reference")
            if row["candidate"]["snapshot"] != EXPECTED_CANDIDATE_BASELINE:
                die(
                    f"{row_id} non-UNKNOWN claim is not bound to the pinned "
                    "candidate baseline"
                )

            (
                original_ref,
                original_fixture_sha256,
                original_receipt,
            ) = validate_classification_receipt(
                row["original"], row_id, "original", f"{row_id}.original"
            )
            (
                candidate_ref,
                candidate_fixture_sha256,
                candidate_receipt,
            ) = validate_classification_receipt(
                row["candidate"], row_id, "candidate", f"{row_id}.candidate"
            )
            validate_comparison_receipt(
                row,
                row_id,
                status,
                original_ref,
                candidate_ref,
                original_fixture_sha256,
                candidate_fixture_sha256,
                original_receipt,
                candidate_receipt,
            )

    missing = REQUIRED_IDS - seen
    extra = seen - REQUIRED_IDS
    if missing or extra:
        die(
            "contract inventory drift: "
            f"missing={sorted(missing)} extra={sorted(extra)}"
        )

    print(
        "phase6c-validate-matrix: PASS "
        f"contracts={len(contracts)} unknown={len(contracts) - non_unknown} "
        f"classified={non_unknown} candidate-baseline={EXPECTED_CANDIDATE_BASELINE}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
