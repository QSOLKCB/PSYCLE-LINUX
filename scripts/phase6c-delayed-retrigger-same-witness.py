#!/usr/bin/env python3
"""Collect and compare the Phase 6C same-witness Sampulse runtime renders."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
BASE_ANALYZER = ROOT / "scripts" / "phase6c-delayed-retrigger-render-evidence.py"
ORIGINAL_GATE = ROOT / "scripts" / "phase6c-delayed-retrigger-original.py"

CONTRACT = "sequencer-delayed-retrigger-same-witness-render"
NAME = "delayed-retrigger-sampulse-runtime"
FIXTURE = "delayed-retrigger/phase6c-delayed-retrigger-sampulse-execution.psy"
TITLE = "PSYCLE-LINUX Phase 6C delayed/retrigger Sampulse execution witness"
CANDIDATE_RECEIPT = "candidate-delayed-retrigger-sampulse-runtime.json"
ORIGINAL_RECEIPT = "original-delayed-retrigger-sampulse-runtime.json"
ORIGINAL_ANALYSIS = "original-delayed-retrigger-sampulse-runtime-analysis.json"
COMPARISON = "delayed-retrigger-sampulse-runtime-comparison.json"
REFERENCE_BUILD = "Psycle 1.12.0 x86"


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ValueError(f"could not load module: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


base = load_module(BASE_ANALYZER, "phase6c_render_base")
original_gate_module = load_module(ORIGINAL_GATE, "phase6c_original_gate")


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


def render_binding(root: Path, relative: str) -> tuple[dict, bytes]:
    path = child(root, relative)
    data = path.read_bytes()
    return {"path": relative, "sha256": digest(data)}, data


def validate_pair_of_waves(
    root: Path, prefix: str, role: str
) -> tuple[list[dict], bytes, dict]:
    bindings: list[dict] = []
    waves: list[bytes] = []
    analyses: list[dict] = []
    for index in (1, 2):
        relative = f"{NAME}/{prefix}-{index}.wav"
        binding, data = render_binding(root, relative)
        analysis = base.analyze_wave(data)
        bindings.append(binding)
        waves.append(data)
        analyses.append(analysis)
    if waves[0] != waves[1] or bindings[0]["sha256"] != bindings[1]["sha256"]:
        raise ValueError(f"{role} repeated renders are not byte-identical")
    if analyses[0] != analyses[1]:
        raise ValueError(f"{role} repeated onset analyses differ")
    return bindings, waves[0], analyses[0]


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    fixture = child(root, FIXTURE)
    raw = fixture.read_bytes()
    if not raw.startswith(b"PSY3SONG"):
        raise ValueError("same-witness Sampulse fixture is not PSY3")

    renders, wave, analysis = validate_pair_of_waves(
        root, "candidate-delayed-retrigger-sampulse-runtime", "candidate"
    )
    receipt = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-runtime-execution-observation",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "fixture": FIXTURE,
        "fixture_sha256": digest(raw),
        "song_title": TITLE,
        "machine_substrate": "XMSampler/Sampulse",
        "command_layout": base.EXPECTED_LAYOUT["commands"],
        "render_procedure": {
            "engine": "frozen SourceForge SVN r12005 C++ candidate",
            "sample_rate": 44100,
            "bits_per_sample": 16,
            "channels": "mono-mix",
            "dither": False,
            "fixed_frame_render": True,
            "target_beats": 4.25,
            "threads": 1,
            "repeat_count": 2,
        },
        "renders": renders,
        "render_sha256": digest(wave),
        "analysis": analysis,
        "runtime_command_execution_observed": True,
        "timing_interpretation": "deferred",
        "parity_status": "UNKNOWN",
    }
    write_new(root / CANDIDATE_RECEIPT, receipt)
    return receipt


def validate_candidate(root: Path) -> dict:
    root = root.resolve()
    receipt = read_json(root / CANDIDATE_RECEIPT)
    fixture = child(root, FIXTURE)
    if (
        receipt.get("schema_version") != 1
        or receipt.get("phase") != "6C"
        or receipt.get("contract") != CONTRACT
        or receipt.get("evidence_role") != "candidate"
        or receipt.get("fixture") != FIXTURE
        or receipt.get("fixture_sha256") != digest(fixture.read_bytes())
        or receipt.get("song_title") != TITLE
        or receipt.get("machine_substrate") != "XMSampler/Sampulse"
        or receipt.get("command_layout") != base.EXPECTED_LAYOUT["commands"]
        or receipt.get("runtime_command_execution_observed") is not True
        or receipt.get("timing_interpretation") != "deferred"
        or receipt.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("same-witness candidate receipt identity mismatch")
    renders, wave, analysis = validate_pair_of_waves(
        root, "candidate-delayed-retrigger-sampulse-runtime", "candidate"
    )
    if (
        receipt.get("renders") != renders
        or receipt.get("render_sha256") != digest(wave)
        or receipt.get("analysis") != analysis
    ):
        raise ValueError("same-witness candidate render binding mismatch")
    return receipt


def validate_original_attempt(
    original_root: Path, attempt: object, index: int
) -> tuple[dict, bytes]:
    if not isinstance(attempt, dict):
        raise ValueError("same-witness original render attempt is not an object")
    for key in (
        "command_verified",
        "command_dispatched",
        "dialog_verified",
        "controls_configured",
        "save_invoked",
    ):
        if attempt.get(key) is not True:
            raise ValueError(f"same-witness original render did not verify {key}")
    if (
        attempt.get("outcome") != "rendered"
        or attempt.get("process_exited") is not False
        or attempt.get("process_exit_code") is not None
    ):
        raise ValueError("same-witness original render did not complete cleanly")
    expected_name = f"original-delayed-retrigger-sampulse-runtime-{index}.wav"
    output = attempt.get("output")
    if (
        not isinstance(output, dict)
        or output.get("path") != expected_name
        or not isinstance(output.get("sha256"), str)
    ):
        raise ValueError("same-witness original output binding is invalid")
    relative = f"{NAME}/{expected_name}"
    data = child(original_root, relative).read_bytes()
    if digest(data) != output["sha256"]:
        raise ValueError("same-witness original output hash mismatch")
    return {"path": relative, "sha256": output["sha256"]}, data


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    candidate = validate_candidate(candidate_root)

    gate = original_gate_module.delayed_validator()
    generic_result = gate.validate_pair(
        NAME, CONTRACT, candidate_root, original_root
    )
    receipt = read_json(original_root / ORIGINAL_RECEIPT)
    if (
        generic_result != "accepted"
        or receipt.get("load_result") != "accepted"
        or receipt.get("schema_version") != 1
        or receipt.get("phase") != "6C"
        or receipt.get("contract") != CONTRACT
        or receipt.get("evidence_role") != "original"
        or receipt.get("reference_build") != REFERENCE_BUILD
        or receipt.get("fixture_sha256") != candidate["fixture_sha256"]
        or receipt.get("original_psycle_observed") is not True
        or receipt.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("same-witness original receipt identity mismatch")

    runtime = receipt.get("runtime_execution")
    attempts = runtime.get("attempts") if isinstance(runtime, dict) else None
    if (
        not isinstance(runtime, dict)
        or runtime.get("schema_version") != 1
        or runtime.get("outcome") != "rendered-twice"
        or runtime.get("deterministic") is not True
        or not isinstance(attempts, list)
        or len(attempts) != 2
    ):
        raise ValueError("same-witness original runtime did not render twice")

    first_binding, first = validate_original_attempt(original_root, attempts[0], 1)
    second_binding, second = validate_original_attempt(original_root, attempts[1], 2)
    if first != second or first_binding["sha256"] != second_binding["sha256"]:
        raise ValueError("same-witness original renders are not byte-identical")
    analysis = base.analyze_wave(first)
    if analysis != base.analyze_wave(second):
        raise ValueError("same-witness original onset analyses differ")

    original_analysis = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "evidence_role": "original-runtime",
        "reference_build": REFERENCE_BUILD,
        "fixture": candidate["fixture"],
        "fixture_sha256": candidate["fixture_sha256"],
        "machine_substrate": "XMSampler/Sampulse",
        "renders": [first_binding, second_binding],
        "render_sha256": digest(first),
        "analysis": analysis,
        "runtime_command_execution_observed": True,
        "timing_interpretation": "deferred",
        "parity_status": "UNKNOWN",
    }
    write_new(original_root / ORIGINAL_ANALYSIS, original_analysis)

    comparison = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "fixture_sha256": candidate["fixture_sha256"],
        "same_fixture_bytes": True,
        "same_onset_analyzer": "phase6c-delayed-retrigger-render-evidence.py::analyze_wave",
        "candidate": {
            "render_sha256": candidate["render_sha256"],
            "analysis": candidate["analysis"],
            "runtime_command_execution_observed": True,
        },
        "original": {
            "reference_build": REFERENCE_BUILD,
            "render_sha256": original_analysis["render_sha256"],
            "analysis": original_analysis["analysis"],
            "runtime_command_execution_observed": True,
        },
        "command_bearing_runtime_pair_observed": True,
        "exact_onset_timing_interpretation": "deferred",
        "classification_allowed": False,
        "parity_status": "UNKNOWN",
        "interpretation_boundary": (
            "Both sides rendered the exact same four-beat Sampulse/XMSampler witness "
            "with the same command geometry and the same onset analyzer. This completes "
            "the command-bearing runtime evidence pair. Exact onset timing is retained "
            "for the next evidence rung and is not classified in this receipt."
        ),
    }
    write_new(original_root / COMPARISON, comparison)
    return comparison


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("candidate", "candidate-check", "original"))
    parser.add_argument("root", type=Path)
    parser.add_argument("other", type=Path, nargs="?")
    args = parser.parse_args()
    if args.mode == "candidate":
        if args.other is not None:
            parser.error("candidate accepts only ROOT")
        value = collect_candidate(args.root)
    elif args.mode == "candidate-check":
        if args.other is not None:
            parser.error("candidate-check accepts only ROOT")
        value = validate_candidate(args.root)
    else:
        if args.other is None:
            parser.error("original requires ORIGINAL_ROOT")
        value = validate_original(args.root, args.other)
    print(json.dumps(value, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
