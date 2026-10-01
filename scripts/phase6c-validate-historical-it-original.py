#!/usr/bin/env python3
"""Validate the private-input original Psycle SickMaate observation."""
from __future__ import annotations

import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
BASE = ROOT / "scripts/phase6c-validate-original-receipts-v2.py"
HELPER = ROOT / "scripts/phase6c-historical-it.py"
EXPECTED_SHA256 = "cab23d8f66a6815b3248457e4f38de74f0a6d61c2691c47a78e582edb63cd432"
EXPECTED_SIZE = 306462
EXPECTED_FIXTURE = "private/d-503_-_sickmaate.it"
CONTRACT = "legacy-impulse-tracker-import-reference"


def load_module(name: str, path: pathlib.Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit(
            "usage: phase6c-validate-historical-it-original.py "
            "PRIVATE_CANDIDATE_ROOT ORIGINAL_ARTIFACT_ROOT"
        )

    base = load_module("phase6c_original_validator", BASE)
    helper = load_module("phase6c_historical_it_helper", HELPER)

    candidate_root = pathlib.Path(sys.argv[1]).resolve()
    original_root = pathlib.Path(sys.argv[2]).resolve()
    if not candidate_root.is_dir() or not original_root.is_dir():
        raise SystemExit("historical IT private candidate/original root is missing")

    candidate = base.load_json(
        candidate_root / "candidate-historical-legacy-it.json"
    )
    if candidate.get("contract") != CONTRACT:
        raise SystemExit("historical candidate receipt contract changed")
    if candidate.get("fixture") != EXPECTED_FIXTURE:
        raise SystemExit("historical candidate receipt fixture path changed")
    if candidate.get("fixture_sha256") != EXPECTED_SHA256:
        raise SystemExit("historical candidate receipt SHA-256 changed")
    if candidate.get("fixture_size_bytes") != EXPECTED_SIZE:
        raise SystemExit("historical candidate receipt size changed")

    identity = helper.validate_historical_file(
        candidate_root / EXPECTED_FIXTURE
    )
    if identity["sha256"] != EXPECTED_SHA256:
        raise SystemExit("historical private fixture validation changed")

    base.validate_artifact_inventory(original_root)
    result = base.validate_pair(
        "historical-legacy-it",
        CONTRACT,
        candidate_root,
        original_root,
        external_fixture=True,
    )

    original = base.load_json(
        original_root / "original-historical-legacy-it.json"
    )
    if original.get("fixture_sha256") != EXPECTED_SHA256:
        raise SystemExit("historical original receipt SHA-256 changed")
    if original.get("fixture_distribution") != "external-hash-bound":
        raise SystemExit("historical original receipt distribution changed")
    if original.get("fixture_redistributed") is not False:
        raise SystemExit("historical original receipt redistribution flag changed")

    historical_it_files = [
        path.relative_to(original_root).as_posix()
        for path in original_root.rglob("*")
        if path.is_file() and path.suffix.lower() == ".it"
    ]
    if historical_it_files:
        raise SystemExit(
            "historical original artifact leaked IT bytes: "
            + ", ".join(sorted(historical_it_files))
        )

    print(
        "phase6c-validate-historical-it-original: PASS "
        f"historical-legacy-it={result}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
