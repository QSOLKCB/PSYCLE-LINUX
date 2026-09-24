#!/usr/bin/env python3
"""Build and validate Phase 6C delayed/retrigger original-render isolation evidence."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath

CONTRACT = "sequencer-delayed-retrigger-render-isolation"
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
    "control": {
        "title": "PSYCLE-LINUX Phase 6C delayed/retrigger render isolation control",
        "events": [
            {"beat": 0.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 1.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 2.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 3.125, "command": "00", "parameter": "00", "role": "ordinary-note"},
        ],
    },
    "fd": {
        "title": "PSYCLE-LINUX Phase 6C delayed/retrigger render isolation FD",
        "events": [
            {"beat": 0.0, "command": "FD", "parameter": "7F", "role": "note-delay"},
            {"beat": 1.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 2.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 3.125, "command": "00", "parameter": "00", "role": "ordinary-note"},
        ],
    },
    "fb": {
        "title": "PSYCLE-LINUX Phase 6C delayed/retrigger render isolation FB",
        "events": [
            {"beat": 0.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 1.0, "command": "FB", "parameter": "3F", "role": "retrigger"},
            {"beat": 2.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 3.125, "command": "00", "parameter": "00", "role": "ordinary-note"},
        ],
    },
    "fa": {
        "title": "PSYCLE-LINUX Phase 6C delayed/retrigger render isolation FA",
        "events": [
            {"beat": 0.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 1.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 2.0, "command": "FA", "parameter": "42", "role": "retrigger-continue"},
            {"beat": 3.125, "command": "00", "parameter": "00", "role": "ordinary-note"},
        ],
    },
    "fe": {
        "title": "PSYCLE-LINUX Phase 6C delayed/retrigger render isolation FE",
        "events": [
            {"beat": 0.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 1.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 2.0, "command": "00", "parameter": "00", "role": "ordinary-note"},
            {"beat": 3.0, "command": "FE", "parameter": "04", "role": "set-lpb"},
            {"beat": 3.125, "command": "00", "parameter": "00", "role": "post-FE-marker"},
        ],
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
        "delayed-retrigger-isolation/"
        f"phase6c-delayed-retrigger-isolation-{name}.psy"
    )


def candidate_receipt_name(name: str) -> str:
    return f"candidate-delayed-retrigger-isolation-{name}.json"


def original_receipt_name(name: str) -> str:
    return f"original-delayed-retrigger-isolation-{name}.json"


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    receipts = {}
    for name, spec in VARIANTS.items():
        relative = fixture_path(name)
        fixture = child(root, relative)
        if not fixture.is_file():
            raise ValueError("render-isolation fixture is missing: " + relative)
        raw = fixture.read_bytes()
        if not raw.startswith(b"PSY3SONG"):
            raise ValueError("render-isolation fixture is not PSY3: " + relative)
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
            "witness_layout": {
                "bpm": 137,
                "lpb": 8,
                "tpb": 24,
                "pattern_beats": 4.0,
                "sample_rate": 44100,
                "sample_frames": 512,
                "impulse_frame": 256,
                "events": spec["events"],
            },
            "purpose": (
                "diagnose the pinned original offline-render failure with one "
                "fresh-process control/command variant at a time; this is not "
                "candidate parity evidence and does not replace frozen PR #69/#70 "
                "or the PR #71 full execution witness"
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
                    f"render-isolation candidate {name} identity mismatch: {key}"
                )
        fixture = child(root, receipt["fixture"])
        raw = fixture.read_bytes()
        if not raw.startswith(b"PSY3SONG"):
            raise ValueError(f"render-isolation candidate {name} is not PSY3")
        if digest(raw) != receipt.get("fixture_sha256"):
            raise ValueError(f"render-isolation candidate {name} hash mismatch")
        layout = receipt.get("witness_layout")
        if (
            not isinstance(layout, dict)
            or layout.get("bpm") != 137
            or layout.get("lpb") != 8
            or layout.get("tpb") != 24
            or layout.get("pattern_beats") != 4.0
            or layout.get("sample_rate") != 44100
            or layout.get("sample_frames") != 512
            or layout.get("impulse_frame") != 256
            or layout.get("events") != spec["events"]
        ):
            raise ValueError(f"render-isolation candidate {name} layout mismatch")
        validated[name] = receipt
    return validated


def require_attempt_prefix(attempt: object, name: str) -> dict:
    if not isinstance(attempt, dict):
        raise ValueError(f"{name}: render attempt must be an object")
    required = {
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": True,
        "save_invoked": True,
    }
    for key, value in required.items():
        if attempt.get(key) is not value:
            raise ValueError(f"{name}: render attempt did not reach verified Save Wave: {key}")
    return attempt


def require_fresh_render_event_binding(attempt: dict, name: str) -> None:
    tick = attempt.get("render_dialog_dispatch_boundary_tick")
    preexisting = attempt.get("preexisting_render_dialog_count")
    handle = attempt.get("selected_render_dialog_native_handle")
    runtime_id = attempt.get("selected_render_dialog_runtime_id")
    if (
        attempt.get("render_dialog_native_event_hook_armed") is not True
        or attempt.get("render_dialog_event_message_pump_started") is not True
        or attempt.get("render_dialog_dispatch_boundary_set") is not True
        or not isinstance(tick, int)
        or isinstance(tick, bool)
        or not 0 <= tick <= 0xFFFFFFFF
        or attempt.get("dialog_discovery")
        != "pumped-win-event-object-show-strictly-after-dispatch-tick"
        or not isinstance(preexisting, int)
        or isinstance(preexisting, bool)
        or preexisting < 0
        or attempt.get("render_dialog_post_dispatch_observed_window_event_count") != 1
        or type(attempt.get("render_dialog_post_dispatch_observed_window_event_count")) is not int
        or attempt.get("render_dialog_unresolved_post_dispatch_event_count") != 0
        or type(attempt.get("render_dialog_unresolved_post_dispatch_event_count")) is not int
        or attempt.get("render_dialog_post_dispatch_event_count") != 1
        or type(attempt.get("render_dialog_post_dispatch_event_count")) is not int
        or not isinstance(handle, int)
        or isinstance(handle, bool)
        or handle <= 0
        or not isinstance(runtime_id, list)
        or not runtime_id
        or any(type(value) is not int for value in runtime_id)
    ):
        raise ValueError(
            f"{name}: fresh render lacks bound post-dispatch dialog evidence"
        )


def validate_completed_render_attempt(
    attempt: dict,
    name: str,
    *,
    allow_post_completion_exit: bool = False,
) -> None:
    require_fresh_render_event_binding(attempt, name)
    diagnostics = attempt.get("diagnostics")
    teardown_diagnostic = (
        "render output finalized and Close control was verified, "
        "but dialog teardown did not complete"
    )
    completed_and_closed = (
        attempt.get("dialog_closed") is True
        and attempt.get("close_control_seen") is True
        and attempt.get("close_uia_invoked") is True
        and diagnostics == []
    )
    completed_with_teardown_failure = (
        attempt.get("dialog_closed") is False
        and attempt.get("close_control_seen") is True
        and attempt.get("close_uia_invoked") is True
        and diagnostics == [teardown_diagnostic]
    )
    exit_code = attempt.get("process_exit_code")
    process_state_valid = (
        (
            attempt.get("process_exited") is True
            and isinstance(exit_code, int)
            and not isinstance(exit_code, bool)
        )
        if allow_post_completion_exit
        else (
            attempt.get("process_exited") is False
            and exit_code is None
        )
    )
    if (
        attempt.get("outcome") != "rendered"
        or not process_state_valid
        or not isinstance(attempt.get("stable_output_polls"), int)
        or isinstance(attempt.get("stable_output_polls"), bool)
        or attempt["stable_output_polls"] < 4
        or not (completed_and_closed or completed_with_teardown_failure)
    ):
        raise ValueError(
            f"{name}: completed render lacks terminal completion evidence"
        )


def validate_observed_output(
    original_root: Path, name: str, value: object, expected_filename: str
) -> dict:
    if (
        not isinstance(value, dict)
        or set(value) != {"path", "size_bytes", "sha256"}
        or value.get("path") != expected_filename
        or not isinstance(value.get("size_bytes"), int)
        or isinstance(value.get("size_bytes"), bool)
        or value["size_bytes"] < 0
        or not isinstance(value.get("sha256"), str)
        or len(value["sha256"]) != 64
    ):
        raise ValueError(f"{name}: invalid observed render output")
    relative = f"delayed-retrigger-isolation-{name}/" + expected_filename
    data = child(original_root, relative).read_bytes()
    if len(data) != value["size_bytes"] or digest(data) != value["sha256"]:
        raise ValueError(f"{name}: observed render output binding mismatch")
    return {
        "path": relative,
        "size_bytes": value["size_bytes"],
        "sha256": value["sha256"],
    }


def validate_failed_observed_output(
    original_root: Path,
    name: str,
    attempt: dict,
    expected_filename: str,
) -> dict | None:
    observed_value = attempt.get("observed_output")
    expected_relative = (
        f"delayed-retrigger-isolation-{name}/" + expected_filename
    )
    expected_path = child(original_root, expected_relative)
    if observed_value is None:
        if expected_path.exists():
            raise ValueError(
                f"{name}: failed render created an unbound output file"
            )
        return None
    return validate_observed_output(
        original_root, name, observed_value, expected_filename
    )


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
        gate_name = f"delayed-retrigger-isolation-{name}"
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
            raise ValueError(f"{name}: render-isolation runtime identity mismatch")

        pre_render = runtime.get("pre_render_load")
        if (
            not isinstance(pre_render, dict)
            or pre_render.get("schema_version") != 1
            or pre_render.get("clean_accepted_load") is not True
            or pre_render.get("load_warning_dismissed") is not True
            or pre_render.get("process_running_before_render") is not True
            or pre_render.get("matched_marker") != Path(fixture_path(name)).name
            or not isinstance(pre_render.get("stable_marker_polls"), int)
            or isinstance(pre_render.get("stable_marker_polls"), bool)
            or pre_render["stable_marker_polls"] < 4
        ):
            raise ValueError(f"{name}: pre-render clean-load evidence is incomplete")

        attempts = runtime.get("attempts")
        if not isinstance(attempts, list) or len(attempts) != 1:
            raise ValueError(f"{name}: expected exactly one diagnostic render attempt")
        outcome = runtime.get("outcome")
        raw_attempt = attempts[0]
        if outcome == "inconclusive" and runtime.get("renders") == []:
            try:
                predispatch = render.validate_predispatch_observer_failure(
                    raw_attempt
                )
            except ValueError:
                pass
            else:
                results[name] = {
                    "outcome": "inconclusive",
                    "inconclusive_reason": predispatch["inconclusive_reason"],
                    "load_result": load_result,
                    "process_exit_code": None,
                    "diagnostics": predispatch["diagnostics"],
                }
                continue
            try:
                presave = render.validate_presave_render_quarantine(raw_attempt)
            except ValueError:
                pass
            else:
                results[name] = {
                    "outcome": "inconclusive",
                    "inconclusive_reason": presave["inconclusive_reason"],
                    "load_result": load_result,
                    "process_exit_code": presave["process_exit_code"],
                    "diagnostics": presave["diagnostics"],
                }
                continue
        attempt = require_attempt_prefix(raw_attempt, name)
        expected_filename = f"original-delayed-retrigger-isolation-{name}-1.wav"

        if outcome == "rendered-once":
            if load_result != "accepted":
                raise ValueError(f"{name}: completed render requires accepted final load")
            validate_completed_render_attempt(attempt, name)
            renders = runtime.get("renders")
            if not isinstance(renders, list) or len(renders) != 1:
                raise ValueError(f"{name}: completed render binding is missing")
            output = attempt.get("output")
            if (
                not isinstance(output, dict)
                or output.get("path") != expected_filename
                or not isinstance(output.get("sha256"), str)
                or renders[0].get("path")
                != f"delayed-retrigger-isolation-{name}/" + expected_filename
                or renders[0].get("sha256") != output.get("sha256")
            ):
                raise ValueError(f"{name}: completed render binding mismatch")
            data = child(original_root, renders[0]["path"]).read_bytes()
            if digest(data) != renders[0]["sha256"]:
                raise ValueError(f"{name}: completed render hash mismatch")
            parsed = render.parse_pcm16_wave(data)
            result = {
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
                or runtime.get("renders") != []
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
            result = {
                "outcome": "reference-process-exited-during-render",
                "load_result": load_result,
                "process_exit_code": exit_code,
                "output_created": observed is not None,
                "observed_output": observed,
            }
        elif outcome == "inconclusive":
            renders = runtime.get("renders")
            post_completion_exit = (
                isinstance(renders, list)
                and len(renders) == 1
                and attempt.get("outcome") == "rendered"
                and attempt.get("process_exited") is True
            )
            if post_completion_exit:
                validate_completed_render_attempt(
                    attempt, name, allow_post_completion_exit=True
                )
                exit_code = attempt.get("process_exit_code")
                if (
                    load_result != "inconclusive"
                    or receipt.get("observation")
                    != "reference-process-exited-before-harness-termination"
                    or receipt.get("exit_code_before_termination") != exit_code
                ):
                    raise ValueError(
                        f"{name}: post-completion exit receipt mismatch"
                    )
                output = attempt.get("output")
                expected_relative = (
                    f"delayed-retrigger-isolation-{name}/" + expected_filename
                )
                if (
                    not isinstance(output, dict)
                    or output.get("path") != expected_filename
                    or renders[0].get("path") != expected_relative
                    or renders[0].get("sha256") != output.get("sha256")
                ):
                    raise ValueError(
                        f"{name}: post-completion render binding mismatch"
                    )
                data = child(original_root, expected_relative).read_bytes()
                if digest(data) != renders[0]["sha256"]:
                    raise ValueError(
                        f"{name}: post-completion render hash mismatch"
                    )
                observed = validate_failed_observed_output(
                    original_root, name, attempt, expected_filename
                )
                result = {
                    "outcome": "inconclusive",
                    "inconclusive_reason": "process-exit-after-completed-render",
                    "load_result": load_result,
                    "render_sha256": digest(data),
                    "process_exit_code": exit_code,
                    "observed_output": observed,
                    "diagnostics": attempt.get("diagnostics"),
                }
            else:
                if renders != []:
                    raise ValueError(
                        f"{name}: inconclusive render cannot retain completed renders"
                    )
                result = {
                    "outcome": "inconclusive",
                    "load_result": load_result,
                    "process_exit_code": attempt.get("process_exit_code"),
                    "diagnostics": attempt.get("diagnostics"),
                }
        else:
            raise ValueError(f"{name}: unexpected render-isolation outcome: {outcome!r}")
        results[name] = result

    outcomes = {name: value["outcome"] for name, value in results.items()}
    if any(value == "inconclusive" for value in outcomes.values()):
        diagnosis = "inconclusive"
    elif outcomes["control"] == "reference-process-exited-during-render":
        diagnosis = "shared-sampled-fixture-or-render-path-failure"
    elif outcomes["control"] == "rendered-once":
        crashing = [
            name for name in ("fd", "fb", "fa", "fe")
            if outcomes[name] == "reference-process-exited-during-render"
        ]
        if crashing:
            diagnosis = "command-associated-render-failure-isolated"
        else:
            diagnosis = "individual-variants-rendered-full-witness-interaction-remains"
    else:
        diagnosis = "inconclusive"

    summary = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "original-render-isolation",
        "contract": CONTRACT,
        "reference_build": REFERENCE_BUILD,
        "results": results,
        "diagnosis": diagnosis,
        "interpretation_boundary": (
            "fresh-process control and one-command variants isolate the original "
            "offline-render access violation only; these receipts do not establish "
            "delayed/retrigger parity or replace the PR #71 full sampled witness"
        ),
        "parity_status": "UNKNOWN",
    }
    write_new(
        original_root / "original-delayed-retrigger-render-isolation.json",
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
