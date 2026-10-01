#!/usr/bin/env python3
"""Validate the one-fixture pinned-original legacy IT observation."""
from __future__ import annotations

import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
BASE = ROOT / "scripts/phase6c-validate-original-receipts-v2.py"


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

    candidate_root = pathlib.Path(sys.argv[1]).resolve()
    original_root = pathlib.Path(sys.argv[2]).resolve()
    if not candidate_root.is_dir() or not original_root.is_dir():
        raise SystemExit("legacy IT candidate/original artifact root is missing")

    module.validate_artifact_inventory(original_root)
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
