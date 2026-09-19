#!/usr/bin/env python3
"""Collect and validate Phase 6C sequence/pattern-order observations.

This evidence lane does not self-promote parity. Candidate and original
observations remain UNKNOWN until a separate versioned comparison is committed.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
CONTRACT = "sequencer-pattern-order"
FIXTURE = "sequence-order/phase6c-sequence-order.psy"
TITLE = "PSYCLE-LINUX Phase 6C order fixture"
EXPECTED_ORDER = [0, 2, 1, 2]
EXPECTED_UI_LABELS = ["00: 00", "01: 02", "02: 01", "03: 02"]
WARNING = "This file is from a newer version of Psycle! This process will try to load it anyway."
SOURCE_PATHS = [
    "tests/phase6c_sequence_order_fixture.c",
    "tests/phase6c_sequence_order.cpp",
    "tests/phase6c_sequence_order.pro",
    "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sequence.h",
    "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/psy3filter.cpp",
]
LOG_LINE = re.compile(
    r"log:\s+\d+us: ([A-Z]): (sequence-order-probe|thread-id-\d+): (.*)"
)
EXPECTED_LOG_MESSAGES = (
    ("T", r"thread-id-\d+", re.escape(
        "# universalis # ../src/universalis/os/thread_name.cpp:56 # "
        "void universalis::os::thread_name::set_tls()"
    )),
    ("T", r"thread-id-\d+", r"setting name for thread: id: \d+, name: sequence-order-probe"),
    ("T", "sequence-order-probe", re.escape(
        "# psycle-core # ../src/psycle/core/player.cpp:66 # "
        "void psycle::core::Player::start_threads()"
    )),
    ("T", "sequence-order-probe", re.escape("psycle: core: player: starting scheduler threads")),
    ("I", "sequence-order-probe", re.escape("psycle: core: player: using 1 threads")),
    ("T", "sequence-order-probe",
     re.escape("psycle: core: psy3 loader: loading psycle song fileformat version 3: ")
     + r"/(?:[^/\r\n]+/)*" + re.escape(FIXTURE)),
    ("W", "sequence-order-probe", re.escape(WARNING)),
    ("I", "sequence-order-probe",
     re.escape("psycle: core: machine factory: create machine: loading with host: 0, plugin: <master>")),
    ("T", r"thread-id-\d+", re.escape(
        "# psycle-core # ../src/psycle/core/player.cpp:452 # "
        "void psycle::core::Player::stop_threads()"
    )),
    ("T", r"thread-id-\d+", re.escape("terminating and joining scheduler threads ...")),
    ("T", r"thread-id-\d+", re.escape(
        "# psycle-core # ../src/psycle/core/player.cpp:454 # "
        "void psycle::core::Player::stop_threads()"
    )),
    ("T", r"thread-id-\d+", re.escape("scheduler threads were not running")),
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_new(path: Path, value) -> None:
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2) + "\n")


def child(root: Path, name: str) -> Path:
    if not isinstance(name, str):
        raise ValueError("unsafe artifact path")
    pure = PurePosixPath(name)
    if pure.is_absolute() or ".." in pure.parts or "\\" in name:
        raise ValueError("unsafe artifact path")
    result = (root / name).resolve()
    result.relative_to(root.resolve())
    return result


def binding(root: Path, path: Path) -> dict:
    return {
        "path": path.relative_to(root).as_posix(),
        "sha256": digest(path.read_bytes()),
    }


def bound_bytes(root: Path, mapping: dict) -> bytes:
    data = child(root, mapping["path"]).read_bytes()
    if digest(data) != mapping["sha256"]:
        raise ValueError("artifact hash mismatch: " + mapping["path"])
    return data


def load_module(filename: str):
    spec = importlib.util.spec_from_file_location(filename, ROOT / "scripts" / filename)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def source_hashes() -> dict[str, str]:
    return {
        path: digest(subprocess.check_output(["git", "show", "HEAD:" + path], cwd=ROOT))
        for path in SOURCE_PATHS
    }


def diagnostics_clean(log: bytes) -> bool:
    try:
        lines = log.decode("utf-8").splitlines()
    except UnicodeError:
        return False
    for line in lines:
        if not line:
            continue
        match = LOG_LINE.fullmatch(line)
        if not match:
            return False
        level, thread, message = match.groups()
        if not any(
            level == expected_level
            and re.fullmatch(expected_thread, thread)
            and re.fullmatch(expected_message, message)
            for expected_level, expected_thread, expected_message
            in EXPECTED_LOG_MESSAGES
        ):
            return False
    return True


def parse_probe(raw: bytes, log: bytes, exit_code: object) -> dict:
    result = {
        "observation": "inconclusive",
        "sequence_lines": [],
        "reports": [],
    }
    if type(exit_code) is not int or exit_code != 0:
        return result
    try:
        value = json.loads(raw)
    except (UnicodeError, ValueError):
        return result
    if not isinstance(value, dict) or value.get("schema_version") != 1:
        return result
    if value.get("load_returned") is not True or value.get("song_name") != TITLE:
        return result
    if value.get("reports") != ["Load Warning: " + WARNING]:
        return result
    lines = value.get("sequence_lines")
    if not isinstance(lines, list) or not lines:
        return result

    normalized = []
    for expected_line_index, line in enumerate(lines):
        if not isinstance(line, dict) or line.get("line_index") != expected_line_index:
            return result
        entries = line.get("entries")
        if not isinstance(entries, list):
            return result
        normalized_entries = []
        previous_position = None
        for entry in entries:
            if not isinstance(entry, dict):
                return result
            position = entry.get("position")
            pattern_id = entry.get("pattern_id")
            pattern_name = entry.get("pattern_name")
            if (
                type(position) not in (int, float)
                or isinstance(position, bool)
                or type(pattern_id) is not int
                or isinstance(pattern_id, bool)
                or not isinstance(pattern_name, str)
            ):
                return result
            if previous_position is not None and float(position) < previous_position:
                return result
            previous_position = float(position)
            normalized_entries.append({
                "position": float(position),
                "pattern_id": pattern_id,
                "pattern_name": pattern_name,
            })
        normalized.append({
            "line_index": expected_line_index,
            "entries": normalized_entries,
        })

    result["sequence_lines"] = normalized
    result["reports"] = value["reports"]
    if diagnostics_clean(log):
        result["observation"] = "sequence-model-observed"
    return result


def collect(root: Path, probe: Path) -> None:
    root = root.resolve()
    probe = probe.resolve()
    fixture = child(root, FIXTURE)
    if not fixture.is_file() or fixture.stat().st_size <= 8:
        raise ValueError("missing generated sequence-order fixture")
    if not fixture.read_bytes().startswith(b"PSY3SONG"):
        raise ValueError("sequence-order fixture is not PSY3")
    if not probe.is_file() or not os.access(probe, os.X_OK):
        raise ValueError("missing sequence-order observation executable")

    source = source_hashes()
    if source != {path: digest((ROOT / path).read_bytes()) for path in SOURCE_PATHS}:
        raise ValueError("probe or fixture source bytes differ from committed Git bytes")

    out_dir = child(root, "sequence-order")
    raw_path = out_dir / "candidate-probe.json"
    log_path = out_dir / "candidate-probe.log"
    receipt_path = root / "candidate-sequence-order.json"
    if raw_path.exists() or log_path.exists() or receipt_path.exists():
        raise ValueError("refusing stale sequence-order evidence")

    env = os.environ.copy()
    env["PSYCLE_THREADS"] = "1"
    process = subprocess.run(
        [str(probe), str(fixture)],
        cwd=ROOT,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=20,
        check=False,
    )
    raw_path.write_bytes(process.stdout)
    log_path.write_bytes(process.stderr)
    parsed = parse_probe(process.stdout, process.stderr, process.returncode)

    receipt = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-observation",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "snapshot": BASELINE,
        "fixture": FIXTURE,
        "fixture_sha256": digest(fixture.read_bytes()),
        "fixture_expected_order": EXPECTED_ORDER,
        "fixture_expected_ui_labels": EXPECTED_UI_LABELS,
        "procedure": (
            "generate the project-authored single-sequence PSY3 fixture with one "
            "legacy play-order list 0,2,1,2; build a separate probe against the "
            "verified historical static core; load the exact fixture without "
            "starting playback using PSYCLE_THREADS=1; enumerate every loaded "
            "SequenceLine entry in model order and record its position and Pattern ID"
        ),
        "probe_sha256": digest(probe.read_bytes()),
        "source_sha256": source,
        "exit_code": process.returncode,
        "raw_probe": binding(root, raw_path),
        "log": binding(root, log_path),
        "observation": parsed["observation"],
        "observed_sequence_lines": parsed["sequence_lines"],
        "reports": parsed["reports"],
        "original_psycle_observed": False,
        "parity_status": "UNKNOWN",
        "parity_note": (
            "Candidate execution evidence alone cannot classify compatibility "
            "with the pinned original Psycle reference."
        ),
    }
    write_new(receipt_path, receipt)


def validate_candidate(root: Path) -> dict:
    root = root.resolve()
    receipt = read(root / "candidate-sequence-order.json")
    expected = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-observation",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "snapshot": BASELINE,
        "fixture": FIXTURE,
        "fixture_expected_order": EXPECTED_ORDER,
        "fixture_expected_ui_labels": EXPECTED_UI_LABELS,
        "original_psycle_observed": False,
        "parity_status": "UNKNOWN",
    }
    for key, value in expected.items():
        if type(receipt.get(key)) is not type(value) or receipt.get(key) != value:
            raise ValueError("candidate receipt field mismatch: " + key)

    fixture = child(root, receipt["fixture"])
    if digest(fixture.read_bytes()) != receipt.get("fixture_sha256"):
        raise ValueError("candidate fixture hash mismatch")
    if not fixture.read_bytes().startswith(b"PSY3SONG"):
        raise ValueError("candidate fixture is not PSY3")

    if receipt.get("source_sha256") != source_hashes():
        raise ValueError("candidate receipt source hashes do not match committed source")
    raw = bound_bytes(root, receipt["raw_probe"])
    log = bound_bytes(root, receipt["log"])
    parsed = parse_probe(raw, log, receipt.get("exit_code"))
    if receipt.get("observation") != parsed["observation"]:
        raise ValueError("candidate observation does not match raw probe evidence")
    if receipt.get("observed_sequence_lines") != parsed["sequence_lines"]:
        raise ValueError("candidate sequence model does not match raw probe evidence")
    if receipt.get("reports") != parsed["reports"]:
        raise ValueError("candidate reports do not match raw probe evidence")
    if not isinstance(receipt.get("probe_sha256"), str) or not re.fullmatch(
        r"[0-9a-f]{64}", receipt["probe_sha256"]
    ):
        raise ValueError("candidate probe SHA-256 is invalid")
    return {
        "observation": parsed["observation"],
        "sequence_lines": parsed["sequence_lines"],
        "parity_status": "UNKNOWN",
    }


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    original_validator = load_module("phase6c-validate-original-receipts-v2.py")
    load_result = original_validator.validate_pair(
        "sequence-order", CONTRACT, candidate_root, original_root
    )
    candidate = read(candidate_root / "candidate-sequence-order.json")
    original = read(original_root / "original-sequence-order.json")

    sequence_ui = original.get("sequence_order_ui")
    if not isinstance(sequence_ui, dict):
        raise ValueError("original sequence-order receipt lacks sequence_order_ui")
    if sequence_ui.get("expected_labels") != candidate.get("fixture_expected_ui_labels"):
        raise ValueError("original sequence order is bound to the wrong expected labels")
    labels = sequence_ui.get("observed_labels")
    if not isinstance(labels, list) or any(
        not isinstance(label, str) or not re.fullmatch(r"\d{2}: \d{2}", label)
        for label in labels
    ):
        raise ValueError("original observed sequence labels are invalid")
    polls = sequence_ui.get("stable_polls")
    if type(polls) is not int or polls < 0:
        raise ValueError("original sequence order stable_polls is invalid")
    result = sequence_ui.get("result")
    if result not in {"observed", "inconclusive"}:
        raise ValueError("original sequence order result is invalid")
    matches = sequence_ui.get("matches_fixture_expected")
    if matches is not None and type(matches) is not bool:
        raise ValueError("original sequence order match flag is invalid")
    if result == "observed":
        if polls < 4 or len(labels) != len(EXPECTED_UI_LABELS):
            raise ValueError("conclusive original sequence order lacks stable complete labels")
        expected_match = labels == EXPECTED_UI_LABELS
        if matches is not expected_match:
            raise ValueError("original sequence order match flag disagrees with observed labels")
    elif matches is not None:
        raise ValueError("inconclusive original sequence order cannot claim a match result")

    return {
        "load_result": load_result,
        "sequence_order_result": result,
        "observed_labels": labels,
        "stable_polls": polls,
        "matches_fixture_expected": matches,
        "parity_status": "UNKNOWN",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("collect", "candidate", "original"))
    parser.add_argument("root", type=Path)
    parser.add_argument("other", type=Path, nargs="?")
    args = parser.parse_args()

    if args.mode == "collect":
        if args.other is None:
            parser.error("collect requires the built probe path")
        collect(args.root, args.other)
    elif args.mode == "candidate":
        print(json.dumps(validate_candidate(args.root), sort_keys=True))
    else:
        if args.other is None:
            parser.error("original requires candidate and original artifact roots")
        print(json.dumps(validate_original(args.root, args.other), sort_keys=True))


if __name__ == "__main__":
    main()
