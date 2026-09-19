#!/usr/bin/env python3
"""Collect and validate Phase 6C BPM/LPB/tick observations.

This lane records candidate and original observations but cannot classify parity
without a separately versioned comparison accepted by the matrix validator.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import struct

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
CONTRACT = "sequencer-bpm-lpb-tick"
FIXTURE = "bpm-lpb-tick/phase6c-bpm-lpb-tick.psy"
TITLE = "PSYCLE-LINUX Phase 6C timing fixture"
BPM = 137.0
LPB = 8
TPB = 24
EXTRA_TICKS = 0
POSITIONS = [0.0, 0.125, 0.25, 0.375]
RATES = [44100, 48000]
WARNING = "This file is from a newer version of Psycle! This process will try to load it anyway."
SOURCE_PATHS = [
    "tests/phase6c_bpm_lpb_tick_fixture.c",
    "tests/phase6c_bpm_lpb_tick.cpp",
    "tests/phase6c_bpm_lpb_tick.pro",
    "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/song.h",
    "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/playertimeinfo.cpp",
    "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/psy3filter.cpp",
]
LOG_LINE = re.compile(r"log:\s+\d+us: ([A-Z]): ([^:]+): (.*)")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_new(path: Path, value) -> None:
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def child(root: Path, name: object) -> Path:
    if not isinstance(name, str):
        raise ValueError("unsafe artifact path")
    pure = PurePosixPath(name)
    if pure.is_absolute() or ".." in pure.parts or "\\" in name:
        raise ValueError("unsafe artifact path")
    path = (root / name).resolve()
    path.relative_to(root.resolve())
    return path


def binding(root: Path, path: Path) -> dict:
    return {"path": path.relative_to(root).as_posix(), "sha256": digest(path.read_bytes())}


def bound_bytes(root: Path, mapping: object) -> bytes:
    if not isinstance(mapping, dict):
        raise ValueError("artifact binding must be an object")
    path = child(root, mapping.get("path"))
    data = path.read_bytes()
    if mapping.get("sha256") != digest(data):
        raise ValueError("artifact hash mismatch")
    return data


def source_hashes() -> dict[str, str]:
    return {path: digest((ROOT / path).read_bytes()) for path in SOURCE_PATHS}


def close(lhs: object, rhs: float, tolerance: float = 1e-7) -> bool:
    return type(lhs) in (int, float) and not isinstance(lhs, bool) and math.isclose(float(lhs), rhs, rel_tol=0.0, abs_tol=tolerance)


def float32(value: float) -> float:
    return struct.unpack("!f", struct.pack("!f", float(value)))[0]


def expected_sample_timing(sample_rate: int, tick_speed: int, is_ticks: bool) -> tuple[float, float, float]:
    # PlayerTimeInfo stores samplesPerBeat_ and samplesPerTick_ as float.
    beat = float32(sample_rate * 60.0 / BPM)
    if is_ticks:
        tick = float32(beat / tick_speed)
    else:
        tick = float32(float32(beat * tick_speed) / 24.0)
    # The probe divides the float samplesPerBeat() result by a double LPB.
    line = beat / LPB
    return beat, tick, line


def diagnostics_clean(log: bytes) -> bool:
    try:
        lines = log.decode("utf-8").splitlines()
    except UnicodeError:
        return False
    saw_loader = saw_warning = saw_master = False
    loader_prefix = "psycle: core: psy3 loader: loading psycle song fileformat version 3: "
    master_message = "psycle: core: machine factory: create machine: loading with host: 0, plugin: <master>"
    for line in lines:
        match = LOG_LINE.fullmatch(line)
        if not match:
            return False
        level, _thread, message = match.groups()
        if level == "T" and message.startswith(loader_prefix) and message.endswith(FIXTURE):
            if saw_loader:
                return False
            saw_loader = True
        elif level == "W" and message == WARNING:
            if saw_warning:
                return False
            saw_warning = True
        elif level == "I" and message == master_message:
            if saw_master:
                return False
            saw_master = True
        else:
            return False
    return saw_loader and saw_warning and saw_master


def parse_probe(raw: bytes, log: bytes, exit_code: object) -> dict:
    inconclusive = {"observation": "inconclusive", "bpm": None, "tick_speed": None,
                    "is_ticks": None, "marker_positions": [], "derived_lpb": None,
                    "sample_rates": [], "reports": []}
    if type(exit_code) is not int or exit_code != 0:
        return inconclusive
    try:
        value = json.loads(raw)
    except (UnicodeError, ValueError):
        return inconclusive
    if not isinstance(value, dict) or type(value.get("schema_version")) is not int or value.get("schema_version") != 1:
        return inconclusive
    if value.get("load_returned") is not True or value.get("song_name") != TITLE:
        return inconclusive
    if value.get("reports") != ["Load Warning: " + WARNING]:
        return inconclusive
    bpm = value.get("bpm")
    tick_speed = value.get("tick_speed")
    is_ticks = value.get("is_ticks")
    positions = value.get("marker_positions")
    derived_lpb = value.get("derived_lpb")
    rates = value.get("sample_rates")
    if not close(bpm, BPM) or type(tick_speed) is not int or isinstance(tick_speed, bool) or tick_speed < 1:
        return inconclusive
    if type(is_ticks) is not bool or not isinstance(positions, list) or len(positions) != len(POSITIONS):
        return inconclusive
    if any(not close(actual, expected) for actual, expected in zip(positions, POSITIONS)) or not close(derived_lpb, LPB):
        return inconclusive
    if not isinstance(rates, list) or len(rates) != len(RATES):
        return inconclusive
    normalized_rates = []
    for mapping, expected_rate in zip(rates, RATES):
        if not isinstance(mapping, dict) or type(mapping.get("sample_rate")) is not int or mapping.get("sample_rate") != expected_rate:
            return inconclusive
        beat, tick, line = expected_sample_timing(expected_rate, tick_speed, is_ticks)
        if not close(mapping.get("samples_per_beat"), beat, 1e-6):
            return inconclusive
        if not close(mapping.get("samples_per_tick"), tick, 1e-6):
            return inconclusive
        if not close(mapping.get("samples_per_fixture_line"), line, 1e-6):
            return inconclusive
        normalized_rates.append({
            "sample_rate": expected_rate,
            "samples_per_beat": float(mapping["samples_per_beat"]),
            "samples_per_tick": float(mapping["samples_per_tick"]),
            "samples_per_fixture_line": float(mapping["samples_per_fixture_line"]),
        })
    if not diagnostics_clean(log):
        return inconclusive
    return {"observation": "timing-model-observed", "bpm": float(bpm),
            "tick_speed": tick_speed, "is_ticks": is_ticks,
            "marker_positions": [float(v) for v in positions],
            "derived_lpb": float(derived_lpb), "sample_rates": normalized_rates,
            "reports": value["reports"]}


def collect(root: Path, probe: Path) -> None:
    root = root.resolve(); probe = probe.resolve()
    fixture = child(root, FIXTURE)
    if not fixture.is_file() or not fixture.read_bytes().startswith(b"PSY3SONG"):
        raise ValueError("missing generated PSY3 timing fixture")
    if not probe.is_file() or not os.access(probe, os.X_OK):
        raise ValueError("missing timing probe executable")
    out = child(root, "bpm-lpb-tick")
    raw_path = out / "candidate-probe.json"
    log_path = out / "candidate-probe.log"
    receipt_path = root / "candidate-bpm-lpb-tick.json"
    if raw_path.exists() or log_path.exists() or receipt_path.exists():
        raise ValueError("refusing stale timing evidence")
    process = subprocess.run([str(probe), str(fixture)], cwd=ROOT,
        env={**os.environ, "PSYCLE_THREADS": "1"}, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, timeout=20, check=False)
    raw_path.write_bytes(process.stdout); log_path.write_bytes(process.stderr)
    parsed = parse_probe(process.stdout, process.stderr, process.returncode)
    receipt = {
        "schema_version": 1, "phase": "6C", "scope": "candidate-observation",
        "contract": CONTRACT, "evidence_role": "candidate", "snapshot": BASELINE,
        "fixture": FIXTURE, "fixture_sha256": digest(fixture.read_bytes()),
        "fixture_expected": {"bpm": 137, "lpb": LPB, "ticks_per_beat": TPB,
                             "extra_ticks_per_beat": EXTRA_TICKS, "marker_positions": POSITIONS},
        "fixture_generator_log": binding(root, out / "fixture-generator.log"),
        "procedure": "generate the project-authored PSY3 timing fixture at BPM 137, LPB 8, TPB 24 and extra ticks 0 with four consecutive line markers; build a separate probe against the frozen Phase 6B core; load without playback using PSYCLE_THREADS=1; record loaded timing metadata, marker spacing and derived sample intervals at 44100 and 48000 Hz",
        "probe_sha256": digest(probe.read_bytes()), "source_sha256": source_hashes(),
        "exit_code": process.returncode, "raw_probe": binding(root, raw_path),
        "log": binding(root, log_path), **parsed, "original_psycle_observed": False,
        "parity_status": "UNKNOWN",
        "parity_note": "Candidate execution and C-Psycle fixture evidence cannot classify compatibility without a versioned original observation and comparison verdict.",
    }
    write_new(receipt_path, receipt)


def validate_candidate(root: Path) -> dict:
    root = root.resolve(); receipt = read(root / "candidate-bpm-lpb-tick.json")
    expected = {"schema_version": 1, "phase": "6C", "scope": "candidate-observation",
                "contract": CONTRACT, "evidence_role": "candidate", "snapshot": BASELINE,
                "fixture": FIXTURE, "original_psycle_observed": False,
                "parity_status": "UNKNOWN"}
    for key, value in expected.items():
        if type(receipt.get(key)) is not type(value) or receipt.get(key) != value:
            raise ValueError("candidate receipt field mismatch: " + key)
    fixture = child(root, receipt["fixture"])
    if receipt.get("fixture_sha256") != digest(fixture.read_bytes()):
        raise ValueError("candidate fixture hash mismatch")
    if receipt.get("fixture_expected") != {"bpm": 137, "lpb": LPB, "ticks_per_beat": TPB,
        "extra_ticks_per_beat": EXTRA_TICKS, "marker_positions": POSITIONS}:
        raise ValueError("candidate fixture expectation changed")
    if b"phase6c-bpm-lpb-tick-fixture: PASS" not in bound_bytes(root, receipt.get("fixture_generator_log")):
        raise ValueError("fixture generator did not record PASS")
    if receipt.get("source_sha256") != source_hashes():
        raise ValueError("candidate source hash mismatch")
    parsed = parse_probe(bound_bytes(root, receipt["raw_probe"]),
                         bound_bytes(root, receipt["log"]), receipt.get("exit_code"))
    for key in ("observation", "bpm", "tick_speed", "is_ticks", "marker_positions",
                "derived_lpb", "sample_rates", "reports"):
        if receipt.get(key) != parsed[key]:
            raise ValueError("candidate raw evidence mismatch: " + key)
    return {**parsed, "parity_status": "UNKNOWN"}


def load_module(name: str):
    spec = importlib.util.spec_from_file_location(name, ROOT / "scripts" / name)
    module = importlib.util.module_from_spec(spec); assert spec.loader is not None
    spec.loader.exec_module(module); return module


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve(); original_root = original_root.resolve()
    validator = load_module("phase6c-validate-original-receipts-v2.py")
    load_result = validator.validate_pair("bpm-lpb-tick", CONTRACT, candidate_root, original_root)
    original = read(original_root / "original-bpm-lpb-tick.json")
    timing = original.get("timing_ui")
    expected = {"tempo": 137, "lines_per_beat": 8, "ticks_per_beat": 24,
                "extra_tick_per_line": 0, "real_tempo": 137, "real_ticks_per_beat": 24}
    if not isinstance(timing, dict) or timing.get("expected_values") != expected:
        raise ValueError("original timing observation is not bound to fixture values")
    observed = timing.get("observed_values")
    polls = timing.get("stable_polls")
    result = timing.get("result")
    matches = timing.get("matches_fixture_expected")
    if result not in {"observed", "inconclusive"} or type(polls) is not int or polls < 0:
        raise ValueError("original timing observation shape is invalid")
    dialog_bootstrap = timing.get("dialog_bootstrap")
    if result == "observed":
        if load_result != "accepted" or polls < 4:
            raise ValueError("conclusive original timing observation is not clean/stable")
        if not isinstance(observed, dict) or set(observed) != set(expected):
            raise ValueError("conclusive original timing observation has incomplete values")
        if any(type(observed[key]) is not int for key in expected):
            raise ValueError("conclusive original timing observation has non-integer values")
        if type(matches) is not bool or matches is not (observed == expected):
            raise ValueError("original timing match flag does not describe observed values")
        if (
            not isinstance(dialog_bootstrap, dict)
            or dialog_bootstrap.get("opened") is not True
            or dialog_bootstrap.get("outcome") not in {"already-open", "opened"}
            or dialog_bootstrap.get("diagnostics") != []
        ):
            raise ValueError("conclusive original timing observation lacks verified dialog bootstrap")
    elif observed not in ({}, None) or matches is not None:
        raise ValueError("inconclusive original timing observation claims values")
    return {"load_result": load_result, "timing_result": result,
            "observed_values": observed, "stable_polls": polls,
            "matches_fixture_expected": matches, "dialog_bootstrap": dialog_bootstrap,
            "parity_status": "UNKNOWN"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("collect", "candidate", "original"))
    parser.add_argument("root", type=Path); parser.add_argument("other", type=Path, nargs="?")
    args = parser.parse_args()
    if args.mode == "collect":
        if args.other is None: parser.error("collect requires the probe path")
        collect(args.root, args.other)
    elif args.mode == "candidate":
        print(json.dumps(validate_candidate(args.root), sort_keys=True))
    else:
        if args.other is None: parser.error("original requires the original artifact root")
        print(json.dumps(validate_original(args.root, args.other), sort_keys=True))


if __name__ == "__main__":
    main()
