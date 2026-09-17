#!/usr/bin/env python3
"""Validate transient native-Windows original-Psycle Phase 6C receipts.

This validator proves evidence integrity only. It deliberately does not classify
compatibility rows; PASS/DIFFERENT/MISSING still require committed original and
candidate receipts plus a committed comparison verdict.
"""

from __future__ import annotations

import hashlib
import json
import pathlib
import re
import sys

EXPECTED_REFERENCE_FILE = "PsycleInstallerx86-1.12.0.exe"
EXPECTED_REFERENCE_BUILD = "Psycle 1.12.0 x86"
EXPECTED_REFERENCE_SHA256 = (
    "f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769"
)
EXPECTED_REFERENCE_SIZE = 9322919
EXPECTED = {
    "psy2": "project-io-psy2-parse",
    "psy3": "project-io-psy3-parse",
}
ALLOWED_LOAD_RESULT = {"accepted", "rejected", "inconclusive"}
ALLOWED_ARTIFACT_SUFFIXES = {".json", ".txt", ".log", ".png", ".md"}
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def die(message: str) -> None:
    raise SystemExit(f"phase6c-validate-original-receipts: {message}")


def load_json(path: pathlib.Path) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8-sig"))
    except FileNotFoundError:
        die(f"missing JSON receipt: {path}")
    except json.JSONDecodeError as exc:
        die(f"invalid JSON in {path}: {exc}")
    if not isinstance(value, dict):
        die(f"receipt must be an object: {path}")
    return value


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require_hash(value: object, context: str) -> str:
    if not isinstance(value, str) or SHA256_RE.fullmatch(value) is None:
        die(f"{context} must be a lowercase SHA-256 digest")
    return value


def require_artifact_file(
    root: pathlib.Path, mapping: object, context: str, *, optional: bool = False
) -> None:
    if mapping is None and optional:
        return
    if not isinstance(mapping, dict):
        die(f"{context} must be an object")
    path_value = mapping.get("path")
    if not isinstance(path_value, str) or not path_value.strip():
        die(f"{context}.path must be a non-empty string")
    relative = pathlib.PurePosixPath(path_value.replace("\\", "/"))
    if relative.is_absolute() or ".." in relative.parts:
        die(f"{context}.path must remain artifact-relative")
    path = (root / pathlib.Path(*relative.parts)).resolve()
    try:
        path.relative_to(root)
    except ValueError:
        die(f"{context}.path escapes artifact root")
    if not path.is_file():
        die(f"{context}.path is missing: {path_value}")
    expected_hash = require_hash(mapping.get("sha256"), f"{context}.sha256")
    actual_hash = sha256(path)
    if actual_hash != expected_hash:
        die(f"{context} SHA-256 mismatch: expected={expected_hash} actual={actual_hash}")


def validate_pair(
    name: str,
    contract: str,
    candidate_root: pathlib.Path,
    original_root: pathlib.Path,
) -> str:
    candidate = load_json(candidate_root / f"candidate-{name}.json")
    original = load_json(original_root / f"original-{name}.json")

    if candidate.get("schema_version") != 1 or candidate.get("phase") != "6C":
        die(f"candidate-{name} has unexpected schema_version/phase")
    if candidate.get("contract") != contract or candidate.get("evidence_role") != "candidate":
        die(f"candidate-{name} is bound to the wrong contract/role")

    if original.get("schema_version") != 1 or original.get("phase") != "6C":
        die(f"original-{name} has unexpected schema_version/phase")
    if original.get("scope") != "original-observation":
        die(f"original-{name} has the wrong scope")
    if original.get("contract") != contract or original.get("evidence_role") != "original":
        die(f"original-{name} is bound to the wrong contract/role")
    if original.get("reference_build") != EXPECTED_REFERENCE_BUILD:
        die(f"original-{name} is bound to the wrong reference build")
    if original.get("reference_file") != EXPECTED_REFERENCE_FILE:
        die(f"original-{name} is bound to the wrong reference file")
    if original.get("reference_installer_sha256") != EXPECTED_REFERENCE_SHA256:
        die(f"original-{name} has the wrong installer SHA-256")
    if original.get("reference_installer_size_bytes") != EXPECTED_REFERENCE_SIZE:
        die(f"original-{name} has the wrong installer size")
    require_hash(
        original.get("reference_executable_sha256"),
        f"original-{name}.reference_executable_sha256",
    )

    candidate_fixture = candidate.get("fixture")
    original_fixture = original.get("fixture")
    if not isinstance(candidate_fixture, str) or not candidate_fixture.strip():
        die(f"candidate-{name} has no fixture")
    if original_fixture != candidate_fixture:
        die(f"original-{name} does not identify the same fixture path as candidate")
    candidate_fixture_hash = require_hash(
        candidate.get("fixture_sha256"), f"candidate-{name}.fixture_sha256"
    )
    original_fixture_hash = require_hash(
        original.get("fixture_sha256"), f"original-{name}.fixture_sha256"
    )
    if original_fixture_hash != candidate_fixture_hash:
        die(f"original-{name} does not identify the same fixture bytes as candidate")

    procedure = original.get("procedure")
    if not isinstance(procedure, str) or not procedure.strip():
        die(f"original-{name}.procedure must be non-empty")
    observation = original.get("observation")
    if not isinstance(observation, str) or not observation.strip():
        die(f"original-{name}.observation must be non-empty")

    result = original.get("load_result")
    if result not in ALLOWED_LOAD_RESULT:
        die(f"original-{name}.load_result is invalid: {result!r}")
    if result == "accepted":
        marker = original.get("load_evidence_marker")
        if not isinstance(marker, str) or not marker.strip():
            die(f"original-{name} accepted result lacks a concrete load evidence marker")

    environment = original.get("environment")
    if not isinstance(environment, dict):
        die(f"original-{name}.environment must be an object")
    mode = environment.get("observation_mode")
    if mode != "native-windows-github-runner-transient-extracted-payload":
        die(f"original-{name} is not native-Windows evidence")
    if environment.get("runner_os") != "Windows":
        die(f"original-{name} runner_os is not Windows")

    if original.get("original_psycle_observed") is not True:
        die(f"original-{name} must explicitly record original_psycle_observed=true")
    if original.get("parity_status") != "UNKNOWN":
        die(f"original-{name} observation artifact must not self-promote parity")

    require_artifact_file(original_root, original.get("stdout"), f"original-{name}.stdout")
    require_artifact_file(original_root, original.get("stderr"), f"original-{name}.stderr")
    require_artifact_file(
        original_root, original.get("ui_evidence"), f"original-{name}.ui_evidence"
    )
    require_artifact_file(
        original_root,
        original.get("screenshot"),
        f"original-{name}.screenshot",
        optional=True,
    )
    return result


def main() -> int:
    if len(sys.argv) != 3:
        die("usage: phase6c-validate-original-receipts.py CANDIDATE_ARTIFACT ORIGINAL_ARTIFACT")

    candidate_root = pathlib.Path(sys.argv[1]).resolve()
    original_root = pathlib.Path(sys.argv[2]).resolve()
    if not candidate_root.is_dir():
        die(f"candidate artifact root is missing: {candidate_root}")
    if not original_root.is_dir():
        die(f"original artifact root is missing: {original_root}")

    for path in original_root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in ALLOWED_ARTIFACT_SUFFIXES:
            die(
                "original evidence artifact contains a prohibited/unexpected file "
                f"type: {path.relative_to(original_root)}"
            )

    results = {
        name: validate_pair(name, contract, candidate_root, original_root)
        for name, contract in EXPECTED.items()
    }

    print(
        "phase6c-validate-original-receipts: PASS "
        + " ".join(f"{name}={result}" for name, result in results.items())
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
