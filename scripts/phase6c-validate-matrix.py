#!/usr/bin/env python3
"""Validate the Phase 6C three-way compatibility matrix.

The important invariant is epistemic, not cosmetic: candidate/C-Psycle evidence alone
must never promote a row out of UNKNOWN. PASS, DIFFERENT and MISSING are claims about
the pinned original-Psycle compatibility target and therefore require reproducible
original-reference evidence, candidate evidence, and a versioned comparison verdict.
"""

from __future__ import annotations

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
ALLOWED_STATUS = {"PASS", "DIFFERENT", "MISSING", "UNKNOWN"}
CLASSIFIED_STATUS = ALLOWED_STATUS - {"UNKNOWN"}
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
) -> tuple[str, str]:
    """Validate one versioned observation receipt and return its ref and fixture hash."""
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
    elif role == "candidate":
        if receipt.get("snapshot") != EXPECTED_CANDIDATE_BASELINE:
            die(f"{context}.observation receipt is bound to the wrong candidate snapshot")
        require_sha256(
            receipt.get("executable_sha256"),
            f"{context}.observation receipt executable_sha256",
        )
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
    return observation_ref, fixture_sha256


def validate_comparison_receipt(
    row: dict[str, object],
    row_id: str,
    status: str,
    original_ref: str,
    candidate_ref: str,
    original_fixture_sha256: str,
    candidate_fixture_sha256: str,
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

            original_ref, original_fixture_sha256 = validate_classification_receipt(
                row["original"], row_id, "original", f"{row_id}.original"
            )
            candidate_ref, candidate_fixture_sha256 = validate_classification_receipt(
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
