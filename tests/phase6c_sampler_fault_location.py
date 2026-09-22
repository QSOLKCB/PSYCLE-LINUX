#!/usr/bin/env python3
"""Negative controls for the Phase 6C Sampler fault-location witness."""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "phase6c-sampler-fault-location.py"
spec = importlib.util.spec_from_file_location("phase6c_sampler_fault_location", SCRIPT)
assert spec is not None and spec.loader is not None
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def expect_value_error(callable_obj, phrase: str) -> None:
    try:
        callable_obj()
    except ValueError as exc:
        assert phrase in str(exc), exc
    else:
        raise AssertionError("expected ValueError")


candidate = {"fixture_sha256": "a" * 64}
control = {
    "contract": module.work_boundary.CONTRACT,
    "fixture_sha256": candidate["fixture_sha256"],
    "reference_build": module.work_boundary.REFERENCE_BUILD,
    "parity_status": "UNKNOWN",
    "runtime_execution": {
        "outcome": "reference-process-exited-during-render",
        "attempts": [
            {
                "process_exited": True,
                "process_exit_code": module.EXPECTED_EXIT,
            }
        ],
    },
}
module.validate_uninstrumented_control(candidate, control)

bad_control = json.loads(json.dumps(control))
bad_control["runtime_execution"]["attempts"][0]["process_exit_code"] = 1
expect_value_error(
    lambda: module.validate_uninstrumented_control(candidate, bad_control),
    "exact access violation",
)

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    evidence = root / "sampler-fault-location"
    evidence.mkdir()

    module_file = evidence / "modules.json"
    module_snapshot = {
        "schema_version": 1,
        "process_id": 1234,
        "modules": [
            {
                "name": "psycle.exe",
                "path": r"C:\Psycle\psycle.exe",
                "base_address_hex": "0x00400000",
                "size_bytes": 0x20000,
                "end_address_hex": "0x00420000",
                "sha256": "b" * 64,
            }
        ],
    }
    module_file.write_text(
        json.dumps(module_snapshot, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    log_file = evidence / "cdb.log"
    log_file.write_text(
        "Access violation - code c0000005\n"
        "ExceptionAddress: 00401234\n",
        encoding="utf-8",
    )

    fault = {
        "schema_version": 1,
        "scope": "pinned-original-sampler-fault-location",
        "outcome": "captured-module-offset",
        "debugger": {
            "name": "cdb.exe",
            "architecture": "x86",
            "sha256": "c" * 64,
        },
        "module_snapshot": {
            "path": "sampler-fault-location/modules.json",
            "sha256": hashlib.sha256(module_file.read_bytes()).hexdigest(),
        },
        "debugger_log": {
            "path": "sampler-fault-location/cdb.log",
            "sha256": hashlib.sha256(log_file.read_bytes()).hexdigest(),
        },
        "render_attempt": {
            "process_exited": True,
            "process_exit_code": module.EXPECTED_EXIT,
        },
        "exception": {
            "expected_code_hex": "0xc0000005",
            "observed_code_hex": "0xc0000005",
            "process_exit_code": module.EXPECTED_EXIT,
            "address_hex": "0x00401234",
            "symbol_resolution": "not-required-for-module-offset",
            "module": {
                "name": "psycle.exe",
                "path": r"C:\Psycle\psycle.exe",
                "sha256": "b" * 64,
                "base_address_hex": "0x00400000",
                "size_bytes": 0x20000,
                "offset_hex": "0x1234",
            },
        },
        "function_location": "unresolved",
        "diagnostics": [],
    }
    receipt = {
        "contract": module.CONTRACT,
        "fixture_sha256": candidate["fixture_sha256"],
        "reference_build": module.work_boundary.REFERENCE_BUILD,
        "parity_status": "UNKNOWN",
        "original_psycle_observed": True,
        "runtime_execution": {
            "schema_version": 1,
            "diagnostic_only": True,
            "parity_status": "UNKNOWN",
            "fault_location": fault,
        },
    }
    validated = module.validate_fault_location(candidate, root, receipt)
    assert validated["outcome"] == "captured-module-offset"

    bad_offset = json.loads(json.dumps(receipt))
    bad_offset["runtime_execution"]["fault_location"]["exception"]["module"][
        "offset_hex"
    ] = "0x1235"
    expect_value_error(
        lambda: module.validate_fault_location(candidate, root, bad_offset),
        "offset is inconsistent",
    )

    bad_function = json.loads(json.dumps(receipt))
    bad_function["runtime_execution"]["fault_location"][
        "function_location"
    ] = "Voice::Tick"
    expect_value_error(
        lambda: module.validate_fault_location(candidate, root, bad_function),
        "observation envelope mismatch",
    )

    log_file.unlink()
    expect_value_error(
        lambda: module.validate_fault_location(candidate, root, receipt),
        "bound artifact missing",
    )

tool_unavailable = {
    "contract": module.CONTRACT,
    "fixture_sha256": candidate["fixture_sha256"],
    "reference_build": module.work_boundary.REFERENCE_BUILD,
    "parity_status": "UNKNOWN",
    "original_psycle_observed": True,
    "runtime_execution": {
        "schema_version": 1,
        "diagnostic_only": True,
        "parity_status": "UNKNOWN",
        "fault_location": {
            "schema_version": 1,
            "scope": "pinned-original-sampler-fault-location",
            "outcome": "inconclusive-tool-unavailable",
            "function_location": "unresolved",
            "diagnostics": ["x86 cdb unavailable"],
        },
    },
}
assert (
    module.validate_fault_location(candidate, Path("."), tool_unavailable)["outcome"]
    == "inconclusive-tool-unavailable"
)

print("phase6c-sampler-fault-location: PASS")
