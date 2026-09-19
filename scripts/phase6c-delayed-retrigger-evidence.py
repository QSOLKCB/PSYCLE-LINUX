#!/usr/bin/env python3
"""Collect and validate Phase 6C delayed/retrigger command observations."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
CONTRACT = "sequencer-delayed-retrigger"
FIXTURE = "delayed-retrigger/phase6c-delayed-retrigger.psy"
PROBE_ARTIFACT = "delayed-retrigger/candidate-probe.bin"
TITLE = "PSYCLE-LINUX Phase 6C delayed/retrigger fixture"
WARNING = "This file is from a newer version of Psycle! This process will try to load it anyway."
SOURCE_COMMIT = "7ac6d2c3553e2ee8dda55814d8e689919c345478"
PLAYER_BLOB = "b5c962cbcc15f267b9fa1ffe1c9ab21d881b8dbc"
SONGSTRUCTS_BLOB = "3ad8cd1b1a0a41bc2225cfd3070bf205cbebec31"
SOURCE_PATHS = [
    "tests/phase6c_delayed_retrigger_fixture.c",
    "tests/phase6c_delayed_retrigger.cpp",
    "tests/phase6c_delayed_retrigger.pro",
    "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sequencer.cpp",
    "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/psy3filter.cpp",
    "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/playertimeinfo.cpp",
]
RAW_PARAMETERS = {
    "note_delay": 0x7F,
    "retrigger": 0x3F,
    "retr_cont": 0x42,
    "extended_lpb": 0x04,
}
LOADED_PARAMETERS = {
    "note_delay": 0x0F,
    "retrigger": 0x3F,
    "retr_cont": 0x42,
    "extended_lpb": 0x04,
}
LOG_LINE = re.compile(
    r"log:\s+\d+us: ([A-Z]): (delayed-retrigger-probe|thread-id-\d+): (.*)"
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git_blob(data: bytes) -> str:
    return hashlib.sha1(
        b"blob " + str(len(data)).encode("ascii") + b"\0" + data
    ).hexdigest()


def read(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_new(path: Path, value) -> None:
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def child(root: Path, value: str) -> Path:
    if not isinstance(value, str):
        raise ValueError("unsafe artifact path")
    pure = PurePosixPath(value)
    if pure.is_absolute() or ".." in pure.parts or "\\" in value:
        raise ValueError("unsafe artifact path")
    path = (root / value).resolve()
    path.relative_to(root.resolve())
    return path


def binding(root: Path, path: Path) -> dict:
    return {
        "path": path.relative_to(root).as_posix(),
        "sha256": digest(path.read_bytes()),
    }


def bound_bytes(root: Path, mapping: object) -> bytes:
    if (
        not isinstance(mapping, dict)
        or set(mapping) != {"path", "sha256"}
        or not isinstance(mapping.get("sha256"), str)
    ):
        raise ValueError("invalid artifact binding")
    data = child(root, mapping["path"]).read_bytes()
    if digest(data) != mapping["sha256"]:
        raise ValueError("artifact hash mismatch: " + mapping["path"])
    return data


def canonical_text_bytes(data: bytes) -> bytes:
    normalized = data.replace(b"\r\n", b"\n")
    if b"\r" in normalized:
        raise ValueError("source contains unsupported carriage returns")
    return normalized


def source_hashes() -> dict[str, str]:
    return {
        path: digest(canonical_text_bytes((ROOT / path).read_bytes()))
        for path in SOURCE_PATHS
    }


def float32(value: float) -> float:
    return struct.unpack("!f", struct.pack("!f", float(value)))[0]


def expected_time() -> tuple[float, float]:
    beat = float32(44100 * 60.0 / 137.0)
    tick = float32(beat / 8)
    return beat, tick


def close(lhs: object, rhs: float, tolerance: float = 1e-9) -> bool:
    return (
        type(lhs) in (int, float)
        and not isinstance(lhs, bool)
        and math.isclose(float(lhs), rhs, rel_tol=0.0, abs_tol=tolerance)
    )


def expected_retrigger_offsets() -> list[float]:
    beat, tick = expected_time()
    delay = ((RAW_PARAMETERS["retrigger"] + 1) * int(tick)) >> 8
    samples = 0
    offsets = []
    while samples < tick:
        offsets.append(samples / beat)
        samples += delay
    return offsets


def expected_retr_cont_offsets() -> list[float]:
    beat, tick = expected_time()
    parameter = RAW_PARAMETERS["retr_cont"]
    variation = (
        4 * (parameter & 0x0F)
        if (parameter & 0x0F) < 9
        else -2 * (16 - (parameter & 0x0F))
    )
    rate = parameter & 0xF0
    delay = (rate * int(tick)) >> 8
    samples = 0
    offsets = []
    while samples < tick:
        offsets.append(samples / beat)
        samples += delay
        rate += variation
        if rate < 16:
            rate = 16
        delay = (rate * int(tick)) >> 8
        if delay <= 0:
            raise ValueError("invalid frozen retrigger-continue delay")
    return offsets


def diagnostics_clean(log: bytes) -> bool:
    try:
        lines = log.decode("utf-8").splitlines()
    except UnicodeError:
        return False
    loader_prefix = "psycle: core: psy3 loader: loading psycle song fileformat version 3: "
    known_exact = {
        ("T", "# universalis # ../src/universalis/os/thread_name.cpp:56 # void universalis::os::thread_name::set_tls()"),
        ("T", "# psycle-core # ../src/psycle/core/player.cpp:66 # void psycle::core::Player::start_threads()"),
        ("T", "psycle: core: player: starting scheduler threads"),
        ("I", "psycle: core: player: using 1 threads"),
        ("W", WARNING),
        ("I", "psycle: core: machine factory: create machine: loading with host: 0, plugin: <sampler>"),
        ("I", "psycle: core: machine factory: create machine: loading with host: 0, plugin: <master>"),
        ("T", "# psycle-core # ../src/psycle/core/player.cpp:452 # void psycle::core::Player::stop_threads()"),
        ("T", "terminating and joining scheduler threads ..."),
        ("T", "# psycle-core # ../src/psycle/core/player.cpp:454 # void psycle::core::Player::stop_threads()"),
        ("T", "scheduler threads were not running"),
    }
    saw_loader = saw_warning = saw_sampler = saw_master = False
    for line in lines:
        match = LOG_LINE.fullmatch(line)
        if not match:
            return False
        level, _thread, message = match.groups()
        if re.fullmatch(
            r"setting name for thread: id: \d+, name: delayed-retrigger-probe",
            message,
        ):
            if level != "T":
                return False
            continue
        if message.startswith(loader_prefix) and message.endswith(FIXTURE):
            if level != "T" or saw_loader:
                return False
            saw_loader = True
            continue
        if (level, message) not in known_exact:
            return False
        if level == "W" and message == WARNING:
            if saw_warning:
                return False
            saw_warning = True
        elif message.endswith("plugin: <sampler>"):
            if saw_sampler:
                return False
            saw_sampler = True
        elif message.endswith("plugin: <master>"):
            if saw_master:
                return False
            saw_master = True
    return saw_loader and saw_warning and saw_sampler and saw_master


def normalize_captures(
    value: object, note: int, expected_track: int, offsets: list[float]
) -> list[dict]:
    if not isinstance(value, list) or len(value) != len(offsets):
        raise ValueError("capture count mismatch")
    result = []
    for index, (capture, expected_offset) in enumerate(zip(value, offsets)):
        if not isinstance(capture, dict):
            raise ValueError("capture must be an object")
        if not close(capture.get("offset"), expected_offset, 2e-9):
            raise ValueError("capture offset mismatch")
        track = capture.get("track")
        if type(track) is not int or isinstance(track, bool) or track != expected_track:
            raise ValueError("capture track is invalid")
        if (
            capture.get("note") != note
            or capture.get("command") != 0
            or capture.get("parameter") != 0
        ):
            raise ValueError("scheduled event was not normalized")
        result.append({
            "offset": float(capture["offset"]),
            "track": track,
            "note": note,
            "command": 0,
            "parameter": 0,
        })
    return result


def parse_probe(raw: bytes, log: bytes, exit_code: object) -> dict:
    inconclusive = {
        "observation": "inconclusive",
        "loaded_parameters": {},
        "marker_position_after_extended": None,
        "note_delay_events": [],
        "retrigger_events": [],
        "retr_cont_events": [],
        "reports": [],
    }
    if type(exit_code) is not int or exit_code != 0:
        return inconclusive
    try:
        value = json.loads(raw)
    except (UnicodeError, ValueError):
        return inconclusive
    try:
        if (
            not isinstance(value, dict)
            or type(value.get("schema_version")) is not int
            or value.get("schema_version") != 1
            or value.get("load_returned") is not True
            or value.get("song_name") != TITLE
            or value.get("bpm") != 137
            or value.get("tick_speed") != 8
            or value.get("is_ticks") is not True
            or value.get("sample_rate") != 44100
            or value.get("loaded_parameters") != LOADED_PARAMETERS
            or value.get("reports") != ["Load Warning: " + WARNING]
            or not diagnostics_clean(log)
        ):
            return inconclusive
        beat, tick = expected_time()
        if (
            not close(value.get("samples_per_beat"), beat, 1e-6)
            or not close(value.get("samples_per_tick"), tick, 1e-6)
            or not close(value.get("marker_position_after_extended"), 0.25)
        ):
            return inconclusive
        note_delay = normalize_captures(
            value.get("note_delay_events"),
            48,
            0,
            [LOADED_PARAMETERS["note_delay"] / 256.0],
        )
        retrigger = normalize_captures(
            value.get("retrigger_events"), 50, 1, expected_retrigger_offsets()
        )
        retr_cont = normalize_captures(
            value.get("retr_cont_events"), 52, 2, expected_retr_cont_offsets()
        )
    except (KeyError, TypeError, ValueError):
        return inconclusive

    return {
        "observation": "command-scheduling-observed",
        "loaded_parameters": dict(LOADED_PARAMETERS),
        "marker_position_after_extended": 0.25,
        "note_delay_events": note_delay,
        "retrigger_events": retrigger,
        "retr_cont_events": retr_cont,
        "reports": value["reports"],
        "bpm": 137.0,
        "tick_speed": 8,
        "is_ticks": True,
        "sample_rate": 44100,
        "samples_per_beat": beat,
        "samples_per_tick": tick,
    }


def validate_probe_identity(root: Path, receipt: dict) -> bytes:
    if receipt.get("probe", {}).get("path") != PROBE_ARTIFACT:
        raise ValueError("candidate probe binding path mismatch")
    data = bound_bytes(root, receipt.get("probe"))
    if receipt.get("probe_sha256") != digest(data):
        raise ValueError("candidate probe identity mismatch")
    return data


def collect(root: Path, probe: Path) -> dict:
    root = root.resolve()
    out = child(root, "delayed-retrigger")
    fixture = child(root, FIXTURE)
    if not fixture.is_file():
        raise ValueError("command fixture is missing")
    raw_path = out / "candidate-probe.json"
    log_path = out / "candidate-probe.log"
    probe_path = child(root, PROBE_ARTIFACT)
    receipt_path = root / "candidate-delayed-retrigger.json"
    for path in (raw_path, log_path, probe_path, receipt_path):
        if path.exists():
            raise ValueError("refusing stale command evidence: " + str(path))
    shutil.copy2(probe, probe_path)
    if digest(probe_path.read_bytes()) != digest(probe.read_bytes()):
        raise ValueError("preserved command probe differs from build output")
    process = subprocess.run(
        [str(probe_path), str(fixture)],
        cwd=ROOT,
        env={**os.environ, "PSYCLE_THREADS": "1"},
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
        "fixture_commands": dict(RAW_PARAMETERS),
        "fixture_generator_log": binding(root, out / "fixture-generator.log"),
        "procedure": (
            "generate the project-authored PSY3 command fixture with FD 7F, "
            "FB 3F, FA 42 and FE 04 on separate tracks; build a separate "
            "probe against the frozen Phase 6B core; preserve and execute the "
            "exact probe bytes with PSYCLE_THREADS=1; load without playback; "
            "replace the loaded Sampler only inside the test harness with a "
            "recording Sampler subclass and invoke the frozen Sequencer "
            "command-scheduling path to record normalized beat offsets"
        ),
        "probe": binding(root, probe_path),
        "probe_sha256": digest(probe_path.read_bytes()),
        "source_sha256": source_hashes(),
        "exit_code": process.returncode,
        "raw_probe": binding(root, raw_path),
        "log": binding(root, log_path),
        **parsed,
        "original_psycle_observed": False,
        "parity_status": "UNKNOWN",
        "parity_note": (
            "Candidate scheduling evidence alone cannot classify compatibility "
            "with pinned original Psycle."
        ),
    }
    write_new(receipt_path, receipt)
    return receipt


def validate_candidate(root: Path) -> dict:
    root = root.resolve()
    receipt = read(root / "candidate-delayed-retrigger.json")
    expected_identity = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-observation",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "snapshot": BASELINE,
        "fixture": FIXTURE,
        "fixture_commands": RAW_PARAMETERS,
        "source_sha256": source_hashes(),
        "original_psycle_observed": False,
        "parity_status": "UNKNOWN",
    }
    for key, expected in expected_identity.items():
        if receipt.get(key) != expected:
            raise ValueError("candidate identity mismatch: " + key)
    fixture = child(root, FIXTURE)
    if digest(fixture.read_bytes()) != receipt.get("fixture_sha256"):
        raise ValueError("candidate fixture identity mismatch")
    bound_bytes(root, receipt.get("fixture_generator_log"))
    validate_probe_identity(root, receipt)
    raw = bound_bytes(root, receipt.get("raw_probe"))
    log = bound_bytes(root, receipt.get("log"))
    parsed = parse_probe(raw, log, receipt.get("exit_code"))
    for key in (
        "observation",
        "loaded_parameters",
        "marker_position_after_extended",
        "note_delay_events",
        "retrigger_events",
        "retr_cont_events",
        "reports",
        "bpm",
        "tick_speed",
        "is_ticks",
        "sample_rate",
        "samples_per_beat",
        "samples_per_tick",
    ):
        if receipt.get(key) != parsed.get(key):
            raise ValueError("candidate raw evidence mismatch: " + key)
    if parsed["observation"] != "command-scheduling-observed":
        raise ValueError(
            "candidate delayed/retrigger observation is inconclusive; "
            "raw evidence remains available for diagnosis"
        )
    return {**parsed, "parity_status": "UNKNOWN"}


def validate_original_source_bytes(player: bytes, structs: bytes) -> dict:
    player = canonical_text_bytes(player)
    structs = canonical_text_bytes(structs)
    if git_blob(player) != PLAYER_BLOB:
        raise ValueError("pinned original Player.cpp blob mismatch")
    if git_blob(structs) != SONGSTRUCTS_BLOB:
        raise ValueError("pinned original SongStructs.hpp blob mismatch")
    player_text = player.decode("utf-8")
    structs_text = structs.decode("utf-8")
    required_structs = [
        "NOTE_DELAY\t= 0xFD",
        "RETRIGGER   = 0xFB",
        "RETR_CONT\t= 0xFA",
        "EXTENDED\t= 0xFE",
        "SET_LINESPERBEAT0 = 0x00",
        "SET_LINESPERBEAT1 = 0x10",
    ]
    required_player = [
        "((entry._parameter+1)*SamplesPerRow())/256",
        "pMachine->RetriggerRate[track] = (entry._parameter+1);",
        "if(entry._parameter&0xf0) pMachine->RetriggerRate[track] = (entry._parameter&0xf0);",
    ]
    for marker in required_structs:
        if structs_text.count(marker) != 1:
            raise ValueError("original command-definition marker changed: " + marker)
    for marker in required_player:
        if player_text.count(marker) != 1:
            raise ValueError("original scheduling marker changed: " + marker)
    extended_lpb = re.compile(
        r"if\s*\(\s*\(pEntry->_parameter&0xE0\)\s*==\s*0\s*\)"
        r"[^{}]*\{\s*SetBPM\(-1,pEntry->_parameter\);",
        re.MULTILINE,
    )
    if len(extended_lpb.findall(player_text)) != 1:
        raise ValueError("original extended-LPB scheduling block changed")
    return {
        "command_ids": {
            "note_delay": 0xFD,
            "retrigger": 0xFB,
            "retr_cont": 0xFA,
            "extended": 0xFE,
            "set_lpb_range": [0x00, 0x1F],
        },
        "semantics": {
            "note_delay_counter": "((parameter+1)*SamplesPerRow())/256",
            "retrigger_rate": "parameter+1",
            "retr_cont_rate_override": "parameter high nibble when nonzero",
            "extended_lpb": "FE00..FE1F calls SetBPM(-1, parameter)",
        },
    }


def collect_source(root: Path, player_path: Path, structs_path: Path) -> dict:
    root = root.resolve()
    receipt_path = root / "original-source-delayed-retrigger.json"
    if receipt_path.exists():
        raise ValueError("refusing stale original-source command receipt")
    player = player_path.read_bytes()
    structs = structs_path.read_bytes()
    derived = validate_original_source_bytes(player, structs)
    receipt = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "original-source-observation",
        "contract": CONTRACT,
        "evidence_role": "original-source",
        "source_repository": "jpaquim/psycle",
        "source_commit": SOURCE_COMMIT,
        "files": {
            "psycle/src/psycle/host/Player.cpp": {
                "git_blob": PLAYER_BLOB,
            },
            "psycle/src/psycle/host/SongStructs.hpp": {
                "git_blob": SONGSTRUCTS_BLOB,
            },
        },
        **derived,
        "original_psycle_executed": False,
        "parity_status": "UNKNOWN",
        "parity_note": (
            "Pinned original-source semantics are not a runtime execution trace; "
            "combine them with the version-pinned native Windows load receipt "
            "before any later comparison."
        ),
    }
    write_new(receipt_path, receipt)
    return receipt


def validate_source(root: Path) -> dict:
    root = root.resolve()
    receipt = read(root / "original-source-delayed-retrigger.json")
    if (
        receipt.get("schema_version") != 1
        or receipt.get("phase") != "6C"
        or receipt.get("scope") != "original-source-observation"
        or receipt.get("contract") != CONTRACT
        or receipt.get("evidence_role") != "original-source"
        or receipt.get("source_repository") != "jpaquim/psycle"
        or receipt.get("source_commit") != SOURCE_COMMIT
        or receipt.get("original_psycle_executed") is not False
        or receipt.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("original-source receipt identity mismatch")
    if receipt.get("command_ids") != {
        "note_delay": 0xFD,
        "retrigger": 0xFB,
        "retr_cont": 0xFA,
        "extended": 0xFE,
        "set_lpb_range": [0x00, 0x1F],
    }:
        raise ValueError("original-source command IDs changed")
    if receipt.get("semantics") != {
        "note_delay_counter": "((parameter+1)*SamplesPerRow())/256",
        "retrigger_rate": "parameter+1",
        "retr_cont_rate_override": "parameter high nibble when nonzero",
        "extended_lpb": "FE00..FE1F calls SetBPM(-1, parameter)",
    }:
        raise ValueError("original-source command semantics changed")
    files = receipt.get("files")
    expected_blobs = {
        "psycle/src/psycle/host/Player.cpp": PLAYER_BLOB,
        "psycle/src/psycle/host/SongStructs.hpp": SONGSTRUCTS_BLOB,
    }
    if not isinstance(files, dict) or set(files) != set(expected_blobs):
        raise ValueError("original-source file inventory changed")
    for path, expected_blob in expected_blobs.items():
        mapping = files.get(path)
        if (
            not isinstance(mapping, dict)
            or set(mapping) != {"git_blob"}
            or mapping.get("git_blob") != expected_blob
        ):
            raise ValueError("original-source file identity changed: " + path)
    return receipt


def load_module(filename: str):
    spec = importlib.util.spec_from_file_location(filename, ROOT / "scripts" / filename)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    validate_candidate(candidate_root)
    source = validate_source(candidate_root)
    validator = load_module("phase6c-validate-original-receipts-v2.py")
    load_result = validator.validate_pair(
        "delayed-retrigger", CONTRACT, candidate_root, original_root
    )
    original = read(original_root / "original-delayed-retrigger.json")
    if load_result != "accepted":
        raise ValueError("original delayed/retrigger fixture load is not accepted")
    return {
        "load_result": load_result,
        "stable_marker_polls": original.get("stable_marker_polls"),
        "source_commit": source["source_commit"],
        "source_command_ids": source["command_ids"],
        "runtime_execution_trace": "not-observed",
        "parity_status": "UNKNOWN",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "mode", choices=("collect", "candidate", "source", "source-check", "original")
    )
    parser.add_argument("root", type=Path)
    parser.add_argument("other", type=Path, nargs="*")
    args = parser.parse_args()
    if args.mode == "collect":
        if len(args.other) != 1:
            parser.error("collect requires PROBE")
        print(json.dumps(collect(args.root, args.other[0]), sort_keys=True))
    elif args.mode == "candidate":
        if args.other:
            parser.error("candidate accepts only ROOT")
        print(json.dumps(validate_candidate(args.root), sort_keys=True))
    elif args.mode == "source":
        if len(args.other) != 2:
            parser.error("source requires PLAYER_CPP SONGSTRUCTS_HPP")
        print(json.dumps(collect_source(args.root, args.other[0], args.other[1]), sort_keys=True))
    elif args.mode == "source-check":
        if args.other:
            parser.error("source-check accepts only ROOT")
        print(json.dumps(validate_source(args.root), sort_keys=True))
    else:
        if len(args.other) != 1:
            parser.error("original requires ORIGINAL_ROOT")
        print(json.dumps(validate_original(args.root, args.other[0]), sort_keys=True))


if __name__ == "__main__":
    main()
