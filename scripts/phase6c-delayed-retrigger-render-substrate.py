#!/usr/bin/env python3
"""Build and validate Phase 6C original-render substrate isolation evidence."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath

CONTRACT = "sequencer-delayed-retrigger-render-substrate-isolation"
REFERENCE_BUILD = "Psycle 1.12.0 x86"
ROOT = Path(__file__).resolve().parents[1]
SETTINGS = {
    "sample_rate": 44100,
    "bits_per_sample": 16,
    "channels": "mono-mix",
    "dither": False,
    "range": "entire-song",
}
VARIANTS = {
    "master-only": {
        "title": "PSYCLE-LINUX Phase 6C render substrate master-only",
        "sampler": False,
        "sample_state": False,
        "ordinary_note": False,
    },
    "sampler-empty": {
        "title": "PSYCLE-LINUX Phase 6C render substrate sampler-empty",
        "sampler": True,
        "sample_state": False,
        "ordinary_note": False,
    },
    "sample-state": {
        "title": "PSYCLE-LINUX Phase 6C render substrate sample-state",
        "sampler": True,
        "sample_state": True,
        "ordinary_note": False,
    },
    "ordinary-note": {
        "title": "PSYCLE-LINUX Phase 6C render substrate ordinary-note",
        "sampler": True,
        "sample_state": True,
        "ordinary_note": True,
    },
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def child(root: Path, relative: str) -> Path:
    if not isinstance(relative, str):
        raise ValueError("unsafe artifact path")
    pure = PurePosixPath(relative)
    if pure.is_absolute() or ".." in pure.parts or "\\" in relative:
        raise ValueError("unsafe artifact path")
    result = (root / relative).resolve()
    result.relative_to(root.resolve())
    return result


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def write_new(path: Path, value: dict) -> None:
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ValueError("could not load module: " + str(path))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def fixture_path(name: str) -> str:
    return (
        "delayed-retrigger-substrate/"
        f"phase6c-delayed-retrigger-substrate-{name}.psy"
    )


def candidate_receipt_name(name: str) -> str:
    return f"candidate-delayed-retrigger-substrate-{name}.json"


def original_receipt_name(name: str) -> str:
    return f"original-delayed-retrigger-substrate-{name}.json"


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    receipts = {}
    for name, spec in VARIANTS.items():
        relative = fixture_path(name)
        fixture = child(root, relative)
        if not fixture.is_file():
            raise ValueError("render-substrate fixture is missing: " + relative)
        raw = fixture.read_bytes()
        if not raw.startswith(b"PSY3SONG"):
            raise ValueError("render-substrate fixture is not PSY3: " + relative)

        layout = {
            "bpm": 137,
            "lpb": 8,
            "tpb": 24,
            "pattern_beats": 4.0,
            "sampler": spec["sampler"],
            "sample_state": spec["sample_state"],
            "ordinary_note": spec["ordinary_note"],
        }
        if spec["sample_state"]:
            layout.update(
                {
                    "sample_rate": 44100,
                    "sample_frames": 512,
                    "impulse_frame": 256,
                }
            )
        if spec["ordinary_note"]:
            layout["note"] = {
                "beat": 0.0,
                "note": 60,
                "instrument": 0,
                "machine": 0,
                "command": "00",
                "parameter": "00",
            }

        receipt = {
            "schema_version": 1,
            "phase": "6C",
            "scope": "candidate-fixture",
            "contract": CONTRACT,
            "evidence_role": "candidate",
            "variant": name,
            "fixture": relative,
            "fixture_sha256": digest(raw),
            "song_title": spec["title"],
            "substrate_layers": layout,
            "purpose": (
                "localize the PR #72 shared sampled-fixture/original-render "
                "access violation across Master-only, empty Sampler, embedded "
                "sample/instrument state, and ordinary-note playback; this is "
                "diagnostic evidence only and cannot classify delayed/retrigger parity"
            ),
            "parity_status": "UNKNOWN",
        }
        write_new(root / candidate_receipt_name(name), receipt)
        receipts[name] = receipt
    return receipts


def validate_candidate(root: Path) -> dict:
    root = root.resolve()
    validated = {}
    for name, spec in VARIANTS.items():
        receipt = read_json(root / candidate_receipt_name(name))
        expected = {
            "schema_version": 1,
            "phase": "6C",
            "scope": "candidate-fixture",
            "contract": CONTRACT,
            "evidence_role": "candidate",
            "variant": name,
            "fixture": fixture_path(name),
            "song_title": spec["title"],
            "parity_status": "UNKNOWN",
        }
        for key, value in expected.items():
            if receipt.get(key) != value:
                raise ValueError(
                    f"render-substrate candidate {name} identity mismatch: {key}"
                )

        fixture = child(root, receipt["fixture"])
        raw = fixture.read_bytes()
        if not raw.startswith(b"PSY3SONG"):
            raise ValueError(f"render-substrate candidate {name} is not PSY3")
        if digest(raw) != receipt.get("fixture_sha256"):
            raise ValueError(f"render-substrate candidate {name} hash mismatch")

        layout = receipt.get("substrate_layers")
        if not isinstance(layout, dict):
            raise ValueError(f"render-substrate candidate {name} lacks layer metadata")
        expected_layout = {
            "bpm": 137,
            "lpb": 8,
            "tpb": 24,
            "pattern_beats": 4.0,
            "sampler": spec["sampler"],
            "sample_state": spec["sample_state"],
            "ordinary_note": spec["ordinary_note"],
        }
        if spec["sample_state"]:
            expected_layout.update(
                {
                    "sample_rate": 44100,
                    "sample_frames": 512,
                    "impulse_frame": 256,
                }
            )
        if spec["ordinary_note"]:
            expected_layout["note"] = {
                "beat": 0.0,
                "note": 60,
                "instrument": 0,
                "machine": 0,
                "command": "00",
                "parameter": "00",
            }
        if layout != expected_layout:
            raise ValueError(f"render-substrate candidate {name} layer mismatch")
        validated[name] = receipt
    return validated


def require_attempt_prefix(attempt: object, name: str) -> dict:
    if not isinstance(attempt, dict):
        raise ValueError(f"{name}: render attempt must be an object")
    for key in (
        "command_verified",
        "command_dispatched",
        "dialog_verified",
        "controls_configured",
        "save_invoked",
    ):
        if attempt.get(key) is not True:
            raise ValueError(
                f"{name}: render attempt did not reach verified Save Wave: {key}"
            )
    return attempt


def validate_failed_observed_output(
    original_root: Path,
    name: str,
    attempt: dict,
    expected_filename: str,
) -> dict | None:
    observed = attempt.get("observed_output")
    relative = f"delayed-retrigger-substrate-{name}/" + expected_filename
    expected_path = child(original_root, relative)
    if observed is None:
        if expected_path.exists():
            raise ValueError(f"{name}: failed render created an unbound output file")
        return None
    if (
        not isinstance(observed, dict)
        or set(observed) != {"path", "size_bytes", "sha256"}
        or observed.get("path") != expected_filename
        or not isinstance(observed.get("size_bytes"), int)
        or isinstance(observed.get("size_bytes"), bool)
        or observed["size_bytes"] < 0
        or not isinstance(observed.get("sha256"), str)
        or len(observed["sha256"]) != 64
    ):
        raise ValueError(f"{name}: invalid failed-render output binding")
    data = expected_path.read_bytes()
    if len(data) != observed["size_bytes"] or digest(data) != observed["sha256"]:
        raise ValueError(f"{name}: failed-render output binding mismatch")
    return {
        "path": relative,
        "size_bytes": observed["size_bytes"],
        "sha256": observed["sha256"],
    }


def validate_stable_alive_output(
    original_root: Path,
    name: str,
    attempt: dict,
    expected_filename: str,
    render_module,
) -> dict:
    if (
        attempt.get("process_exited") is not False
        or attempt.get("process_exit_code") is not None
        or attempt.get("dialog_closed") is not False
        or not isinstance(attempt.get("stable_output_polls"), int)
        or isinstance(attempt.get("stable_output_polls"), bool)
        or attempt["stable_output_polls"] < 4
    ):
        raise ValueError(f"{name}: stable-alive render evidence is inconsistent")
    observed = validate_failed_observed_output(
        original_root, name, attempt, expected_filename
    )
    if observed is None or observed["size_bytes"] <= 44:
        raise ValueError(f"{name}: stable-alive render lacks non-empty output")
    data = child(original_root, observed["path"]).read_bytes()
    if (
        len(data) < 12
        or data[:4] != b"RIFF"
        or data[8:12] != b"WAVE"
        or int.from_bytes(data[4:8], "little") + 8 != len(data)
    ):
        raise ValueError(f"{name}: stable-alive output is not a finalized RIFF/WAVE")
    parsed = render_module.parse_pcm16_wave(data)
    return {
        "observed_output": observed,
        "frame_count": len(parsed["frames"]),
        "nonzero_frame_count": sum(1 for value in parsed["frames"] if value != 0),
    }


def diagnose(outcomes: dict[str, str]) -> str:
    control_like = {
        "rendered-once",
        "stable-finalized-output-process-alive",
    }
    crash = "reference-process-exited-during-render"
    allowed = control_like | {crash}
    if any(value not in allowed for value in outcomes.values()):
        return "inconclusive"

    ordered = ["master-only", "sampler-empty", "sample-state", "ordinary-note"]
    survived = [outcomes[name] in control_like for name in ordered]

    # The ladder is cumulative. A later non-crashing output after an earlier
    # process exit defeats a simple first-failing-layer interpretation.
    seen_failure = False
    for success in survived:
        if seen_failure and success:
            return "nonmonotonic-substrate-result"
        if not success:
            seen_failure = True

    if not survived[0]:
        return "master-only-associated-reference-exit"
    if not survived[1]:
        return "sampler-presence-associated-reference-exit"
    if not survived[2]:
        return "sample-state-associated-reference-exit"
    if not survived[3]:
        return "ordinary-note-associated-reference-exit-after-stable-output-controls"
    return "all-substrate-controls-survive-full-witness-detail-remains"


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    validate_candidate(candidate_root)

    delayed = load_module(
        ROOT / "scripts" / "phase6c-delayed-retrigger-original.py",
        "phase6c_delayed_retrigger_original",
    )
    generic = delayed.delayed_validator()
    render = load_module(
        ROOT / "scripts" / "phase6c-delayed-retrigger-render-evidence.py",
        "phase6c_delayed_retrigger_render_evidence",
    )

    results = {}
    for name in VARIANTS:
        gate_name = f"delayed-retrigger-substrate-{name}"
        load_result = generic.validate_pair(
            gate_name, CONTRACT, candidate_root, original_root
        )
        receipt = read_json(original_root / original_receipt_name(name))
        if receipt.get("load_result") != load_result:
            raise ValueError(f"{name}: generic original load-result mismatch")

        runtime = receipt.get("runtime_execution")
        if (
            not isinstance(runtime, dict)
            or runtime.get("schema_version") != 1
            or runtime.get("settings") != SETTINGS
            or runtime.get("deterministic") is not False
        ):
            raise ValueError(f"{name}: render-substrate runtime identity mismatch")

        pre_render = runtime.get("pre_render_load")
        if not isinstance(pre_render, dict) or pre_render.get("schema_version") != 1:
            raise ValueError(f"{name}: pre-render load evidence is malformed")

        clean = (
            pre_render.get("clean_accepted_load") is True
            and pre_render.get("load_warning_dismissed") is True
            and pre_render.get("process_running_before_render") is True
            and pre_render.get("matched_marker") == Path(fixture_path(name)).name
            and isinstance(pre_render.get("stable_marker_polls"), int)
            and not isinstance(pre_render.get("stable_marker_polls"), bool)
            and pre_render["stable_marker_polls"] >= 4
        )
        attempts = runtime.get("attempts")
        renders = runtime.get("renders")
        if not isinstance(attempts, list) or not isinstance(renders, list):
            raise ValueError(f"{name}: render attempt/binding arrays are malformed")

        if not clean:
            if (
                runtime.get("outcome") != "inconclusive"
                or attempts != []
                or renders != []
            ):
                raise ValueError(
                    f"{name}: non-clean pre-render state must remain inconclusive"
                )
            results[name] = {
                "outcome": "inconclusive",
                "load_result": load_result,
                "pre_render_load": pre_render,
                "diagnostics": runtime.get("diagnostics"),
            }
            continue

        if len(attempts) != 1:
            raise ValueError(f"{name}: expected exactly one substrate render attempt")
        attempt = require_attempt_prefix(attempts[0], name)
        expected_filename = f"original-delayed-retrigger-substrate-{name}-1.wav"
        outcome = runtime.get("outcome")

        if outcome == "rendered-once":
            if load_result != "accepted":
                raise ValueError(f"{name}: completed render requires accepted final load")
            if (
                attempt.get("outcome") != "rendered"
                or attempt.get("process_exited") is not False
                or attempt.get("process_exit_code") is not None
                or len(renders) != 1
            ):
                raise ValueError(f"{name}: completed render state is inconsistent")
            output = attempt.get("output")
            expected_relative = (
                f"delayed-retrigger-substrate-{name}/" + expected_filename
            )
            if (
                not isinstance(output, dict)
                or output.get("path") != expected_filename
                or not isinstance(output.get("sha256"), str)
                or renders[0].get("path") != expected_relative
                or renders[0].get("sha256") != output.get("sha256")
            ):
                raise ValueError(f"{name}: completed render binding mismatch")
            data = child(original_root, expected_relative).read_bytes()
            if digest(data) != renders[0]["sha256"]:
                raise ValueError(f"{name}: completed render hash mismatch")
            parsed = render.parse_pcm16_wave(data)
            results[name] = {
                "outcome": "rendered-once",
                "load_result": load_result,
                "render_sha256": digest(data),
                "frame_count": len(parsed["frames"]),
                "process_exit_code": None,
            }
        elif outcome == "reference-process-exited-during-render":
            if (
                load_result != "inconclusive"
                or receipt.get("observation")
                != "reference-process-exited-before-harness-termination"
                or renders != []
                or attempt.get("outcome") != "inconclusive"
                or attempt.get("process_exited") is not True
                or attempt.get("output") is not None
            ):
                raise ValueError(f"{name}: process-exit render evidence is inconsistent")
            exit_code = attempt.get("process_exit_code")
            if (
                not isinstance(exit_code, int)
                or isinstance(exit_code, bool)
                or exit_code == 0
                or receipt.get("exit_code_before_termination") != exit_code
                or attempt.get("diagnostics")
                != ["reference exited during offline render"]
            ):
                raise ValueError(f"{name}: process-exit code/diagnostic mismatch")
            observed = validate_failed_observed_output(
                original_root, name, attempt, expected_filename
            )
            results[name] = {
                "outcome": "reference-process-exited-during-render",
                "load_result": load_result,
                "process_exit_code": exit_code,
                "output_created": observed is not None,
                "observed_output": observed,
            }
        elif outcome == "inconclusive":
            if renders != []:
                raise ValueError(f"{name}: inconclusive render retained completed output")
            stable_alive = None
            if (
                attempt.get("process_exited") is False
                and attempt.get("process_exit_code") is None
                and isinstance(attempt.get("stable_output_polls"), int)
                and not isinstance(attempt.get("stable_output_polls"), bool)
                and attempt["stable_output_polls"] >= 4
                and attempt.get("observed_output") is not None
            ):
                stable_alive = validate_stable_alive_output(
                    original_root,
                    name,
                    attempt,
                    expected_filename,
                    render,
                )
            if stable_alive is not None:
                results[name] = {
                    "outcome": "stable-finalized-output-process-alive",
                    "runtime_outcome": "inconclusive",
                    "load_result": load_result,
                    "process_exit_code": None,
                    "dialog_closed": False,
                    "stable_output_polls": attempt["stable_output_polls"],
                    **stable_alive,
                }
            else:
                results[name] = {
                    "outcome": "inconclusive",
                    "load_result": load_result,
                    "process_exit_code": attempt.get("process_exit_code"),
                    "diagnostics": attempt.get("diagnostics"),
                }
        else:
            raise ValueError(
                f"{name}: unexpected render-substrate outcome: {outcome!r}"
            )

    outcomes = {name: value["outcome"] for name, value in results.items()}
    diagnosis = diagnose(outcomes)
    exit_codes = sorted(
        {
            value["process_exit_code"]
            for value in results.values()
            if isinstance(value.get("process_exit_code"), int)
            and not isinstance(value.get("process_exit_code"), bool)
        }
    )
    summary = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "original-render-substrate-isolation",
        "contract": CONTRACT,
        "reference_build": REFERENCE_BUILD,
        "results": results,
        "diagnosis": diagnosis,
        "observed_exit_codes": exit_codes,
        "interpretation_boundary": (
            "the cumulative Master/Sampler/sample-state/note ladder distinguishes "
            "stable finalized PCM output with the reference process still alive from "
            "a verified process exit during offline render; a stable output does not "
            "by itself prove the original Render dialog reached its normal terminal "
            "state, and this diagnostic evidence does not establish delayed/retrigger "
            "execution or classify parity"
        ),
        "parity_status": "UNKNOWN",
    }
    write_new(
        original_root / "original-delayed-retrigger-render-substrate.json",
        summary,
    )
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
