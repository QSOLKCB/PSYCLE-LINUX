#!/usr/bin/env python3
"""Validate the one-fixture pinned-original legacy IT observation."""
from __future__ import annotations

import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
BASE = ROOT / "scripts/phase6c-validate-original-receipts-v2.py"
GENERATOR = ROOT / "scripts/phase6c-generate-it-import-fixture.py"


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit(
            "usage: phase6c-validate-legacy-it-original.py "
            "CANDIDATE_ARTIFACT ORIGINAL_ARTIFACT"
        )
    spec = importlib.util.spec_from_file_location("phase6c_original_validator", BASE)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)

    generator_spec = importlib.util.spec_from_file_location(
        "phase6c_legacy_it_generator", GENERATOR
    )
    generator = importlib.util.module_from_spec(generator_spec)
    assert generator_spec.loader is not None
    generator_spec.loader.exec_module(generator)

    candidate_root = pathlib.Path(sys.argv[1]).resolve()
    original_root = pathlib.Path(sys.argv[2]).resolve()
    if not candidate_root.is_dir() or not original_root.is_dir():
        raise SystemExit("legacy IT candidate/original artifact root is missing")

    candidate = module.load_json(candidate_root / "candidate-legacy-it.json")
    fixture_ref = candidate.get("fixture")
    if fixture_ref != "phase6c-legacy-it-import.it":
        raise SystemExit("legacy IT candidate receipt is not bound to the canonical fixture path")
    if candidate.get("fixture_sha256") != generator.EXPECTED_SHA256:
        raise SystemExit("legacy IT candidate receipt is not bound to the canonical fixture SHA-256")

    candidate_fixture = module.resolve_artifact_path(
        candidate_root, fixture_ref, "candidate-legacy-it.fixture"
    )
    candidate_bytes = candidate_fixture.read_bytes()
    if (
        len(candidate_bytes) != generator.EXPECTED_SIZE
        or module.sha256(candidate_fixture) != generator.EXPECTED_SHA256
        or candidate_bytes[:4] != b"IMPM"
    ):
        raise SystemExit("legacy IT candidate fixture bytes are not canonical")

    candidate_it_files = sorted(
        path.relative_to(candidate_root).as_posix()
        for path in candidate_root.rglob("*")
        if path.is_file() and path.suffix.lower() == ".it"
    )
    if candidate_it_files != [fixture_ref]:
        raise SystemExit(
            "legacy IT candidate artifact contains unexpected .it files: "
            + ", ".join(candidate_it_files)
        )

    original_fixture_ref = f"fixtures/legacy-it/{fixture_ref}"
    module.validate_artifact_inventory(
        original_root,
        allowed_it={original_fixture_ref: generator.EXPECTED_SHA256},
    )
    result = module.validate_pair(
        "legacy-it",
        "legacy-it-import-reference",
        candidate_root,
        original_root,
    )
    print(f"phase6c-validate-legacy-it-original: PASS legacy-it={result}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
