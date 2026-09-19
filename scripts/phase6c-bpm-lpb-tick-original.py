#!/usr/bin/env python3
"""Validate the timing original receipt with the maintained generic validator.

The generic validator predates this fixture and names the PSY3 fixtures that
require the exact Load Warning bootstrap inline. Extend only that frozen set in
memory, with exact anchor counts, then delegate all receipt/inventory checks.
"""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import sys
import types

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE_PATH = ROOT / "scripts/phase6c-bpm-lpb-tick-evidence.py"
VALIDATOR_PATH = ROOT / "scripts/phase6c-validate-original-receipts-v2.py"


def load(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def timing_validator():
    source = VALIDATOR_PATH.read_text(encoding="utf-8")
    old = '{"psy3", "sequence-order"}'
    if source.count(old) != 2:
        raise ValueError(
            "generic original validator PSY3 warning-scope anchor changed"
        )
    source = source.replace(
        old, '{"psy3", "sequence-order", "bpm-lpb-tick"}'
    )
    module = types.ModuleType("phase6c_original_receipts_timing")
    module.__file__ = str(VALIDATOR_PATH)
    exec(compile(source, str(VALIDATOR_PATH), "exec"), module.__dict__)
    return module


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit(
            "usage: phase6c-bpm-lpb-tick-original.py "
            "CANDIDATE_ARTIFACT_ROOT ORIGINAL_ARTIFACT_ROOT"
        )
    evidence = load(EVIDENCE_PATH, "phase6c_bpm_lpb_tick_evidence")
    validator = timing_validator()
    evidence.load_module = lambda name: (
        validator
        if name == "phase6c-validate-original-receipts-v2.py"
        else load(ROOT / "scripts" / name, name)
    )
    result = evidence.validate_original(Path(sys.argv[1]), Path(sys.argv[2]))
    if result.get("timing_result") != "observed":
        raise ValueError(
            "timing observation workflow requires an observed original timing result"
        )
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
