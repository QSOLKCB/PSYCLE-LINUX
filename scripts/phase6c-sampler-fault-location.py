#!/usr/bin/env python3
"""Build and validate the opt-in Phase 6C Sampler fault-location witness."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
WORK_BOUNDARY_SCRIPT = ROOT / "scripts" / "phase6c-sampler-work-boundary.py"
CONTRACT = "sequencer-sampler-fault-location"
TARGET_VARIANT = "delayed-note-short"
CANDIDATE_RECEIPT = "candidate-sampler-fault-location.json"
ORIGINAL_RECEIPT = "original-sampler-fault-location.json"
SUMMARY_RECEIPT = "original-sampler-fault-location-summary.json"
EXPECTED_EXIT = -1073741819


def load_work_boundary():
    spec = importlib.util.spec_from_file_location(
        "phase6c_sampler_work_boundary", WORK_BOUNDARY_SCRIPT
    )
    if spec is None or spec.loader is None:
        raise ValueError("could not load Sampler work-boundary validator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


work_boundary = load_work_boundary()


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def write_new(path: Path, value: dict) -> None:
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def child(root: Path, relative: str) -> Path:
    if not isinstance(relative, str):
        raise ValueError("unsafe artifact path")
    pure = PurePosixPath(relative)
    if pure.is_absolute() or ".." in pure.parts or "\\" in relative:
        raise ValueError("unsafe artifact path")
    result = (root / relative).resolve()
    result.relative_to(root.resolve())
    return result


def target_candidate_receipt(root: Path) -> tuple[Path, dict]:
    path = root / work_boundary.candidate_receipt_name(TARGET_VARIANT)
    receipt = read_json(path)
    if (
        receipt.get("contract") != work_boundary.CONTRACT
        or receipt.get("variant") != TARGET_VARIANT
        or receipt.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("fault-location target work-boundary identity mismatch")
    return path, receipt


def expected_candidate(root: Path) -> dict:
    source_path, source = target_candidate_receipt(root)
    return {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "observation": "diagnostic-input-binding-only",
        "target_variant": TARGET_VARIANT,
        "fixture": source["fixture"],
        "fixture_sha256": source["fixture_sha256"],
        "work_boundary_candidate_receipt": {
            "path": source_path.name,
            "sha256": digest(source_path.read_bytes()),
        },
        "purpose": (
            "bind the exact short Sampler-local E-DF witness used for an opt-in "
            "pinned-original exception/module-offset observation; this receipt "
            "contains no candidate runtime fault-location evidence"
        ),
        "parity_status": "UNKNOWN",
    }


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    work_boundary.validate_candidate(root)
    receipt = expected_candidate(root)
    write_new(root / CANDIDATE_RECEIPT, receipt)
    return receipt


def validate_candidate(root: Path) -> dict:
    root = root.resolve()
    work_boundary.validate_candidate(root)
    actual = read_json(root / CANDIDATE_RECEIPT)
    expected = expected_candidate(root)
    if actual != expected:
        raise ValueError("Sampler fault-location candidate receipt mismatch")
    fixture = child(root, actual["fixture"])
    if digest(fixture.read_bytes()) != actual["fixture_sha256"]:
        raise ValueError("Sampler fault-location fixture hash mismatch")
    return actual


def validate_binding(root: Path, binding: dict, label: str) -> Path:
    if (
        not isinstance(binding, dict)
        or not isinstance(binding.get("path"), str)
        or not isinstance(binding.get("sha256"), str)
        or len(binding["sha256"]) != 64
    ):
        raise ValueError(f"{label}: malformed artifact binding")
    path = child(root, binding["path"])
    if not path.is_file():
        raise ValueError(f"{label}: bound artifact missing")
    if digest(path.read_bytes()) != binding["sha256"]:
        raise ValueError(f"{label}: bound artifact hash mismatch")
    return path


def parse_hex(value: str, label: str) -> int:
    if not isinstance(value, str) or not value.lower().startswith("0x"):
        raise ValueError(f"{label}: malformed hexadecimal value")
    try:
        return int(value[2:], 16)
    except ValueError as exc:
        raise ValueError(f"{label}: malformed hexadecimal value") from exc


def validate_uninstrumented_control(candidate: dict, receipt: dict) -> dict:
    if (
        receipt.get("contract") != work_boundary.CONTRACT
        or receipt.get("fixture_sha256") != candidate["fixture_sha256"]
        or receipt.get("reference_build") != work_boundary.REFERENCE_BUILD
        or receipt.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("uninstrumented fault-location control identity mismatch")
    runtime = receipt.get("runtime_execution")
    attempts = runtime.get("attempts") if isinstance(runtime, dict) else None
    if (
        not isinstance(runtime, dict)
        or runtime.get("outcome") != "reference-process-exited-during-render"
        or not isinstance(attempts, list)
        or len(attempts) != 1
        or attempts[0].get("process_exited") is not True
        or attempts[0].get("process_exit_code") != EXPECTED_EXIT
    ):
        raise ValueError(
            "uninstrumented short E-DF control did not reproduce exact access violation"
        )
    return runtime


ALLOWED_FAULT_OUTCOMES = {
    "captured-module-offset",
    "inconclusive-clean-load-required",
    "inconclusive-tool-unavailable",
    "inconclusive-module-snapshot-failed",
    "inconclusive-debugger-start-failed",
    "inconclusive-debugger-attach-failed",
    "inconclusive-debugger-log-missing",
    "inconclusive-exception-code-not-captured",
    "inconclusive-exit-code-mismatch",
    "inconclusive-exception-address-not-captured",
    "inconclusive-exception-module-unresolved",
}


def validate_fault_location(
    candidate: dict, original_root: Path, receipt: dict
) -> dict:
    if (
        receipt.get("contract") != CONTRACT
        or receipt.get("fixture_sha256") != candidate["fixture_sha256"]
        or receipt.get("reference_build") != work_boundary.REFERENCE_BUILD
        or receipt.get("parity_status") != "UNKNOWN"
        or receipt.get("original_psycle_observed") is not True
    ):
        raise ValueError("instrumented fault-location receipt identity mismatch")

    runtime = receipt.get("runtime_execution")
    if (
        not isinstance(runtime, dict)
        or runtime.get("schema_version") != 1
        or runtime.get("diagnostic_only") is not True
        or runtime.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("fault-location runtime envelope mismatch")

    fault = runtime.get("fault_location")
    if (
        not isinstance(fault, dict)
        or fault.get("schema_version") != 1
        or fault.get("function_location") != "unresolved"
        or fault.get("outcome") not in ALLOWED_FAULT_OUTCOMES
    ):
        raise ValueError("fault-location observation envelope mismatch")

    outcome = fault["outcome"]
    module_snapshot_path = None
    debugger_log_path = None
    if fault.get("module_snapshot") is not None:
        module_snapshot_path = validate_binding(
            original_root, fault["module_snapshot"], "module snapshot"
        )
    if fault.get("debugger_log") is not None:
        debugger_log_path = validate_binding(
            original_root, fault["debugger_log"], "debugger log"
        )

    if outcome == "captured-module-offset":
        debugger = fault.get("debugger")
        exception = fault.get("exception")
        module = exception.get("module") if isinstance(exception, dict) else None
        attempt = fault.get("render_attempt")
        if (
            module_snapshot_path is None
            or debugger_log_path is None
            or not isinstance(debugger, dict)
            or debugger.get("name") != "cdb.exe"
            or debugger.get("architecture") != "x86"
            or not isinstance(debugger.get("sha256"), str)
            or len(debugger["sha256"]) != 64
            or not isinstance(attempt, dict)
            or attempt.get("process_exited") is not True
            or attempt.get("process_exit_code") != EXPECTED_EXIT
            or not isinstance(exception, dict)
            or exception.get("expected_code_hex") != "0xc0000005"
            or exception.get("observed_code_hex") != "0xc0000005"
            or exception.get("process_exit_code") != EXPECTED_EXIT
            or not isinstance(exception.get("address_hex"), str)
            or not isinstance(module, dict)
            or not isinstance(module.get("name"), str)
            or not isinstance(module.get("base_address_hex"), str)
            or not isinstance(module.get("offset_hex"), str)
            or exception.get("symbol_resolution")
            != "not-required-for-module-offset"
        ):
            raise ValueError("captured module-offset witness is incomplete")

        snapshot = read_json(module_snapshot_path)
        modules = snapshot.get("modules")
        if not isinstance(modules, list):
            raise ValueError("captured module snapshot has no module list")
        matching = [
            item
            for item in modules
            if isinstance(item, dict)
            and item.get("name") == module.get("name")
            and item.get("base_address_hex") == module.get("base_address_hex")
            and item.get("size_bytes") == module.get("size_bytes")
            and item.get("sha256") == module.get("sha256")
        ]
        if len(matching) != 1:
            raise ValueError(
                "captured exception module is not uniquely bound to module snapshot"
            )
        address = parse_hex(exception["address_hex"], "exception address")
        base = parse_hex(module["base_address_hex"], "module base")
        offset = parse_hex(module["offset_hex"], "module offset")
        if address - base != offset or offset >= int(module["size_bytes"]):
            raise ValueError("captured exception module offset is inconsistent")
    return fault


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    candidate = validate_candidate(candidate_root)

    control_path = (
        original_root
        / work_boundary.original_receipt_name(TARGET_VARIANT)
    )
    debug_path = original_root / ORIGINAL_RECEIPT
    control = read_json(control_path)
    debug = read_json(debug_path)

    validate_uninstrumented_control(candidate, control)
    if (
        debug.get("reference_executable_sha256")
        != control.get("reference_executable_sha256")
    ):
        raise ValueError(
            "instrumented witness reference executable differs from control"
        )
    fault = validate_fault_location(candidate, original_root, debug)

    outcome = fault["outcome"]
    summary = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "reference_build": work_boundary.REFERENCE_BUILD,
        "target_variant": TARGET_VARIANT,
        "fixture": candidate["fixture"],
        "fixture_sha256": candidate["fixture_sha256"],
        "uninstrumented_control": {
            "path": control_path.name,
            "sha256": digest(control_path.read_bytes()),
            "exit_code": EXPECTED_EXIT,
        },
        "instrumented_observation": {
            "path": debug_path.name,
            "sha256": digest(debug_path.read_bytes()),
            "outcome": outcome,
        },
        "module_offset": (
            fault["exception"]["module"]
            if outcome == "captured-module-offset"
            else None
        ),
        "exception_address_hex": (
            fault["exception"]["address_hex"]
            if outcome == "captured-module-offset"
            else None
        ),
        "function_location": "unresolved",
        "interpretation_boundary": (
            "an observed module offset can localize the access violation to a "
            "loaded binary region but cannot by itself name a source function; "
            "missing debugger/tool/address/module evidence is inconclusive"
        ),
        "parity_status": "UNKNOWN",
    }
    write_new(original_root / SUMMARY_RECEIPT, summary)
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    for command in ("candidate", "candidate-check"):
        item = sub.add_parser(command)
        item.add_argument("root", type=Path)

    original = sub.add_parser("original")
    original.add_argument("candidate_root", type=Path)
    original.add_argument("original_root", type=Path)

    args = parser.parse_args()
    if args.command == "candidate":
        result = collect_candidate(args.root)
    elif args.command == "candidate-check":
        result = validate_candidate(args.root)
    else:
        result = validate_original(args.candidate_root, args.original_root)

    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
