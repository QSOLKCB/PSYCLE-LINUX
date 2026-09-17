#!/usr/bin/env python3
"""Validate the Phase 6C three-way compatibility matrix.

The important invariant is epistemic, not cosmetic: candidate/C-Psycle evidence alone
must never promote a row out of UNKNOWN. PASS, DIFFERENT and MISSING are claims about
the pinned original-Psycle compatibility target and therefore require reproducible
original-reference evidence as well as candidate evidence.
"""

from __future__ import annotations

import json
import pathlib
import sys

MATRIX = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path(
    "phase6c/compatibility-matrix.json"
)

EXPECTED_ORIGINAL_SHA256 = (
    "f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769"
)
EXPECTED_ORIGINAL_SIZE = 9322919
EXPECTED_CANDIDATE_BASELINE = (
    "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
)
ALLOWED_STATUS = {"PASS", "DIFFERENT", "MISSING", "UNKNOWN"}
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
    if original.get("file") != "PsycleInstallerx86-1.12.0.exe":
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

        for required in ("subsystem", "contract", "status", "original", "candidate", "cpsycle"):
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
