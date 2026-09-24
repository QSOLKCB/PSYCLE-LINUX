#!/usr/bin/env python3
"""Build and validate Phase 6C Sampler voice-startup isolation evidence."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath
import sys

CONTRACT = "sequencer-sampler-voice-startup-isolation"
REFERENCE_BUILD = "Psycle 1.12.0 x86"
ROOT = Path(__file__).resolve().parents[1]
SOURCE_COMMIT = "7ac6d2c3553e2ee8dda55814d8e689919c345478"
SAMPLER_BLOB = "6cc0bd7328d01131c3d41b68f4e5d4189959e364"
SONG_BLOB = "9ed00469d29d0a5714cc7b86175d0dc4c6048e3a"
SOURCE_RECEIPT = "original-source-sampler-voice-startup.json"
EXPECTED_ACCESS_VIOLATION_EXIT_CODE = -1073741819  # Windows 0xC0000005
SOURCE_SAMPLER_MARKERS = [
    "lastInstrument[i]=255;",
    "if (data._inst == 255)",
    "data._inst = lastInstrument[channel];",
    "if ( !Global::song().samples.IsEnabled(data._inst) ) return;",
    "useVoice = GetFreeVoice();",
    "_voices[useVoice].Tick(&data, channel, _resampler, baseC, multicmdMem);",
    "inst = Global::song()._pInstrument[_instrument];",
    "controller.wave = &wave;",
]
SOURCE_SONG_MARKERS = [
    "for(int i(0) ; i < MAX_INSTRUMENTS ; ++i) "
    "_pInstrument[i] = new Instrument();"
]
SOURCE_BOUNDARY = [
    "Sampler constructor initializes every lastInstrument slot to 255",
    "instrument FF with no previous instrument returns before sample lookup",
    "disabled sample slot returns before voice selection",
    "enabled sample advances through GetFreeVoice into Voice::Tick",
    "Voice::Tick resolves the constructor-created legacy instrument slot",
    "Voice::Tick binds the enabled sample to the voice controller before audio work",
]
SETTINGS = {
    "sample_rate": 44100,
    "bits_per_sample": 16,
    "channels": "mono-mix",
    "dither": False,
    "range": "entire-song",
}
VARIANTS = {
    "note-no-previous-inst": {
        "title": "PSYCLE-LINUX Phase 6C Sampler startup no previous instrument",
        "note_instrument": 255,
        "serialized_instrument": True,
        "source_gate": "last-instrument-empty-return",
    },
    "note-missing-sample": {
        "title": "PSYCLE-LINUX Phase 6C Sampler startup missing sample",
        "note_instrument": 1,
        "serialized_instrument": True,
        "source_gate": "sample-enabled-return",
    },
    "note-sample-default-inst": {
        "title": "PSYCLE-LINUX Phase 6C Sampler startup default instrument",
        "note_instrument": 0,
        "serialized_instrument": False,
        "source_gate": "voice-tick-default-legacy-instrument",
    },
    "note-sample-serialized-inst": {
        "title": "PSYCLE-LINUX Phase 6C Sampler startup serialized instrument",
        "note_instrument": 0,
        "serialized_instrument": True,
        "source_gate": "voice-tick-serialized-instrument",
    },
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git_blob(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


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
        "sampler-voice-startup/"
        f"phase6c-sampler-voice-startup-{name}.psy"
    )


def candidate_receipt_name(name: str) -> str:
    return f"candidate-sampler-voice-startup-{name}.json"


def original_receipt_name(name: str) -> str:
    return f"original-sampler-voice-startup-{name}.json"


def expected_layout(name: str, spec: dict) -> dict:
    return {
        "bpm": 137,
        "lpb": 8,
        "tpb": 24,
        "pattern_beats": 4.0,
        "sampler_machine": 0,
        "sample_slot_0": {
            "sample_rate": 44100,
            "sample_frames": 512,
            "impulse_frame": 256,
        },
        "sample_slot_1_present": False,
        "serialized_instrument_0": spec["serialized_instrument"],
        "note": {
            "beat": 0.0,
            "note": 60,
            "instrument": spec["note_instrument"],
            "machine": 0,
            "command": "00",
            "parameter": "00",
        },
        "source_gate": spec["source_gate"],
    }


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    receipts = {}
    for name, spec in VARIANTS.items():
        relative = fixture_path(name)
        fixture = child(root, relative)
        if not fixture.is_file():
            raise ValueError("voice-startup fixture is missing: " + relative)
        raw = fixture.read_bytes()
        if not raw.startswith(b"PSY3SONG"):
            raise ValueError("voice-startup fixture is not PSY3: " + relative)
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
            "startup_layout": expected_layout(name, spec),
            "purpose": (
                "advance through the source-pinned original Sampler::Tick gates "
                "after PR #73 localized the render exit to ordinary-note-present "
                "Sampler execution; diagnostic only, no parity classification"
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
            "startup_layout": expected_layout(name, spec),
            "parity_status": "UNKNOWN",
        }
        for key, value in expected.items():
            if receipt.get(key) != value:
                raise ValueError(
                    f"voice-startup candidate {name} identity mismatch: {key}"
                )
        fixture = child(root, receipt["fixture"])
        raw = fixture.read_bytes()
        if not raw.startswith(b"PSY3SONG"):
            raise ValueError(f"voice-startup candidate {name} is not PSY3")
        if digest(raw) != receipt.get("fixture_sha256"):
            raise ValueError(f"voice-startup candidate {name} hash mismatch")
        validated[name] = receipt
    return validated


def expected_source_receipt() -> dict:
    return {
        "schema_version": 1,
        "phase": "6C",
        "scope": "pinned-original-source",
        "contract": CONTRACT,
        "reference_build": REFERENCE_BUILD,
        "source_commit": SOURCE_COMMIT,
        "files": {
            "Sampler.cpp": {
                "git_blob": SAMPLER_BLOB,
                "markers": list(SOURCE_SAMPLER_MARKERS),
            },
            "Song.cpp": {
                "git_blob": SONG_BLOB,
                "markers": list(SOURCE_SONG_MARKERS),
            },
        },
        "source_boundary": list(SOURCE_BOUNDARY),
        "parity_status": "UNKNOWN",
    }


def collect_source(root: Path, sampler_path: Path, song_path: Path) -> dict:
    root = root.resolve()
    sampler_data = sampler_path.read_bytes()
    song_data = song_path.read_bytes()
    if git_blob(sampler_data) != SAMPLER_BLOB:
        raise ValueError("pinned original Sampler.cpp blob mismatch")
    if git_blob(song_data) != SONG_BLOB:
        raise ValueError("pinned original Song.cpp blob mismatch")

    sampler_text = sampler_data.replace(b"\r\n", b"\n").decode("utf-8")
    song_text = song_data.replace(b"\r\n", b"\n").decode("utf-8")
    for marker in SOURCE_SAMPLER_MARKERS:
        if marker not in sampler_text:
            raise ValueError("pinned Sampler.cpp source marker missing: " + marker)
    for marker in SOURCE_SONG_MARKERS:
        if marker not in song_text:
            raise ValueError("pinned Song.cpp source marker missing: " + marker)

    receipt = expected_source_receipt()
    write_new(root / SOURCE_RECEIPT, receipt)
    return receipt


def validate_source(root: Path) -> dict:
    receipt = read_json(root.resolve() / SOURCE_RECEIPT)
    expected = expected_source_receipt()
    if receipt != expected:
        raise ValueError(
            "voice-startup source receipt differs from canonical source semantics"
        )
    return receipt


def materialize_source_receipt(
    candidate_root: Path, original_root: Path
) -> str:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    candidate_receipt = validate_source(candidate_root)
    source_path = child(candidate_root, SOURCE_RECEIPT)
    destination = child(original_root, SOURCE_RECEIPT)
    source_bytes = source_path.read_bytes()

    if destination.exists():
        if destination.read_bytes() != source_bytes:
            raise ValueError(
                "original artifact source receipt conflicts with candidate receipt"
            )
    else:
        destination.write_bytes(source_bytes)

    original_receipt = validate_source(original_root)
    if original_receipt != candidate_receipt:
        raise ValueError("original artifact source receipt changed during materialization")
    return SOURCE_RECEIPT



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


def require_attempt_dispatch_prefix(attempt: object, name: str) -> dict:
    if not isinstance(attempt, dict):
        raise ValueError(f"{name}: render attempt must be an object")
    for key in ("command_verified", "command_dispatched"):
        if attempt.get(key) is not True:
            raise ValueError(
                f"{name}: render attempt did not verify command dispatch: {key}"
            )
    for key in ("dialog_verified", "controls_configured", "save_invoked"):
        if not isinstance(attempt.get(key), bool):
            raise ValueError(
                f"{name}: render attempt has invalid boolean field: {key}"
            )
    return attempt


def validate_fresh_render_event_binding_or_quarantine(
    attempt: dict,
    name: str,
    outcome: object,
    renders: object,
) -> str | None:
    """Reject bad fresh binding unless the attempt is already non-evidentiary."""
    try:
        require_fresh_render_event_binding(attempt, name)
    except ValueError as exc:
        diagnostics = attempt.get("diagnostics")
        if (
            outcome == "inconclusive"
            and renders == []
            and attempt.get("outcome") == "inconclusive"
            and attempt.get("output") is None
            and isinstance(diagnostics, list)
            and diagnostics
        ):
            return str(exc)
        raise
    return None


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
    require_fresh_render_event_binding(attempt, name)
    return attempt


def validate_expected_access_violation(
    receipt: dict, attempt: dict, name: str
) -> int:
    exit_code = attempt.get("process_exit_code")
    if (
        not isinstance(exit_code, int)
        or isinstance(exit_code, bool)
        or exit_code != EXPECTED_ACCESS_VIOLATION_EXIT_CODE
        or receipt.get("exit_code_before_termination") != exit_code
        or attempt.get("diagnostics")
        != ["reference exited during offline render"]
    ):
        raise ValueError(
            f"{name}: process exit does not match expected 0xC0000005 access violation"
        )
    return exit_code


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
            and exit_code != 0
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
        or not (completed_and_closed or completed_with_teardown_failure)
        or not isinstance(attempt.get("stable_output_polls"), int)
        or isinstance(attempt.get("stable_output_polls"), bool)
        or attempt["stable_output_polls"] < 4
    ):
        raise ValueError(
            f"{name}: completed render lacks terminal completion evidence"
        )


def validate_observed_output(
    original_root: Path,
    name: str,
    attempt: dict,
    expected_filename: str,
) -> dict | None:
    observed = attempt.get("observed_output")
    relative = f"sampler-voice-startup-{name}/" + expected_filename
    expected_path = child(original_root, relative)
    if observed is None:
        if expected_path.exists():
            raise ValueError(f"{name}: render created an unbound output file")
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
        raise ValueError(f"{name}: invalid observed output binding")
    data = expected_path.read_bytes()
    if len(data) != observed["size_bytes"] or digest(data) != observed["sha256"]:
        raise ValueError(f"{name}: observed output binding mismatch")
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
        raise ValueError(f"{name}: stable-alive evidence is inconsistent")
    observed = validate_observed_output(
        original_root, name, attempt, expected_filename
    )
    if observed is None or observed["size_bytes"] <= 44:
        raise ValueError(f"{name}: stable-alive result lacks non-empty output")
    data = child(original_root, observed["path"]).read_bytes()
    if (
        len(data) < 12
        or data[:4] != b"RIFF"
        or data[8:12] != b"WAVE"
        or int.from_bytes(data[4:8], "little") + 8 != len(data)
    ):
        raise ValueError(f"{name}: stable-alive output is not finalized RIFF/WAVE")
    parsed = render_module.parse_pcm16_wave(data)
    return {
        "observed_output": observed,
        "frame_count": len(parsed["frames"]),
        "nonzero_frame_count": sum(1 for value in parsed["frames"] if value != 0),
    }


def diagnose(outcomes: dict[str, str]) -> str:
    live = {"rendered-once", "stable-finalized-output-process-alive"}
    crash = "reference-process-exited-during-render"
    allowed = live | {crash}
    if any(value not in allowed for value in outcomes.values()):
        return "inconclusive"

    ordered = [
        "note-no-previous-inst",
        "note-missing-sample",
        "note-sample-default-inst",
        "note-sample-serialized-inst",
    ]
    survived = [outcomes[name] in live for name in ordered]
    seen_crash = False
    for success in survived:
        if seen_crash and success:
            return "nonmonotonic-startup-result"
        if not success:
            seen_crash = True

    if not survived[0]:
        return "sampler-dispatch-before-instrument-resolution-associated-exit"
    if not survived[1]:
        return "explicit-instrument-or-sample-gate-associated-exit"
    if not survived[2]:
        return "enabled-sample-voice-startup-associated-exit"
    if not survived[3]:
        return "serialized-instrument-state-associated-exit"
    return "all-startup-gates-survive-known-crash-not-reproduced"


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    validate_candidate(candidate_root)
    source_receipt = materialize_source_receipt(
        candidate_root, original_root
    )

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
        gate_name = f"sampler-voice-startup-{name}"
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
            raise ValueError(f"{name}: voice-startup runtime identity mismatch")
        pre = runtime.get("pre_render_load")
        clean = (
            isinstance(pre, dict)
            and pre.get("schema_version") == 1
            and pre.get("clean_accepted_load") is True
            and pre.get("load_warning_dismissed") is True
            and pre.get("process_running_before_render") is True
            and pre.get("matched_marker") == Path(fixture_path(name)).name
            and isinstance(pre.get("stable_marker_polls"), int)
            and not isinstance(pre.get("stable_marker_polls"), bool)
            and pre["stable_marker_polls"] >= 4
        )
        attempts = runtime.get("attempts")
        renders = runtime.get("renders")
        if not isinstance(attempts, list) or not isinstance(renders, list):
            raise ValueError(f"{name}: render arrays malformed")
        if not clean:
            if runtime.get("outcome") != "inconclusive" or attempts or renders:
                raise ValueError(f"{name}: dirty pre-render state was promoted")
            results[name] = {
                "outcome": "inconclusive",
                "load_result": load_result,
                "pre_render_load": pre,
            }
            continue
        if len(attempts) != 1:
            raise ValueError(f"{name}: expected one startup render attempt")
        filename = f"original-sampler-voice-startup-{name}-1.wav"
        outcome = runtime.get("outcome")
        raw_attempt = attempts[0]
        if outcome == "inconclusive" and renders == []:
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
        attempt = require_attempt_dispatch_prefix(raw_attempt, name)
        binding_error = validate_fresh_render_event_binding_or_quarantine(
            attempt, name, outcome, renders
        )
        if binding_error is not None:
            observed = validate_observed_output(
                original_root, name, attempt, filename
            )
            results[name] = {
                "outcome": "inconclusive",
                "load_result": load_result,
                "process_exit_code": attempt.get("process_exit_code"),
                "diagnostics": attempt.get("diagnostics"),
                "fresh_render_event_binding": "rejected",
                "fresh_render_event_binding_error": binding_error,
                "observed_output": observed,
            }
            continue
        attempt = require_attempt_prefix(attempt, name)

        if outcome == "rendered-once":
            if load_result != "accepted" or len(renders) != 1:
                raise ValueError(f"{name}: completed render state inconsistent")
            validate_completed_render_attempt(attempt, name)
            expected_relative = f"sampler-voice-startup-{name}/" + filename
            output = attempt.get("output")
            if (
                not isinstance(output, dict)
                or output.get("path") != filename
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
                "nonzero_frame_count": sum(
                    1 for value in parsed["frames"] if value != 0
                ),
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
                raise ValueError(f"{name}: process-exit state inconsistent")
            exit_code = validate_expected_access_violation(
                receipt, attempt, name
            )
            observed = validate_observed_output(
                original_root, name, attempt, filename
            )
            results[name] = {
                "outcome": "reference-process-exited-during-render",
                "load_result": load_result,
                "process_exit_code": exit_code,
                "output_created": observed is not None,
                "observed_output": observed,
            }
        elif outcome == "inconclusive":
            post_completion_exit = (
                len(renders) == 1
                and attempt.get("outcome") == "rendered"
                and attempt.get("process_exited") is True
            )
            if post_completion_exit:
                validate_completed_render_attempt(
                    attempt, name, allow_post_completion_exit=True
                )
                exit_code = attempt.get("process_exit_code")
                if receipt.get("exit_code_before_termination") != exit_code:
                    raise ValueError(
                        f"{name}: post-completion exit code mismatch"
                    )
                expected_relative = f"sampler-voice-startup-{name}/" + filename
                output = attempt.get("output")
                if (
                    not isinstance(output, dict)
                    or output.get("path") != filename
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
                observed = validate_observed_output(
                    original_root, name, attempt, filename
                )
                results[name] = {
                    "outcome": "inconclusive",
                    "inconclusive_reason": "process-exit-after-completed-render",
                    "load_result": load_result,
                    "render_sha256": digest(data),
                    "process_exit_code": exit_code,
                    "observed_output": observed,
                    "diagnostics": attempt.get("diagnostics"),
                }
                continue
            if renders:
                raise ValueError(f"{name}: inconclusive result retained completed render")
            stable = None
            if (
                attempt.get("process_exited") is False
                and attempt.get("process_exit_code") is None
                and isinstance(attempt.get("stable_output_polls"), int)
                and not isinstance(attempt.get("stable_output_polls"), bool)
                and attempt["stable_output_polls"] >= 4
                and attempt.get("observed_output") is not None
            ):
                stable = validate_stable_alive_output(
                    original_root, name, attempt, filename, render
                )
            if stable is not None:
                results[name] = {
                    "outcome": "stable-finalized-output-process-alive",
                    "runtime_outcome": "inconclusive",
                    "load_result": load_result,
                    "process_exit_code": None,
                    "dialog_closed": False,
                    "stable_output_polls": attempt["stable_output_polls"],
                    **stable,
                }
            else:
                results[name] = {
                    "outcome": "inconclusive",
                    "load_result": load_result,
                    "process_exit_code": attempt.get("process_exit_code"),
                    "diagnostics": attempt.get("diagnostics"),
                }
        else:
            raise ValueError(f"{name}: unexpected startup outcome: {outcome!r}")

    outcomes = {name: value["outcome"] for name, value in results.items()}
    summary = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "original-sampler-voice-startup-isolation",
        "contract": CONTRACT,
        "reference_build": REFERENCE_BUILD,
        "source_receipt": source_receipt,
        "results": results,
        "diagnosis": diagnose(outcomes),
        "observed_exit_codes": sorted(
            {
                value["process_exit_code"]
                for value in results.values()
                if isinstance(value.get("process_exit_code"), int)
                and not isinstance(value.get("process_exit_code"), bool)
            }
        ),
        "interpretation_boundary": (
            "the source-bound note/instrument/sample ladder isolates how far "
            "pinned original Sampler::Tick/Voice::Tick proceeds before the "
            "offline-render process exit; it does not establish delayed/retrigger "
            "execution and does not classify parity"
        ),
        "parity_status": "UNKNOWN",
    }
    write_new(
        original_root / "original-sampler-voice-startup-isolation.json",
        summary,
    )
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    for command in ("candidate", "candidate-check", "source-check"):
        item = sub.add_parser(command)
        item.add_argument("root", type=Path)

    source = sub.add_parser("source")
    source.add_argument("root", type=Path)
    source.add_argument("sampler_cpp", type=Path)
    source.add_argument("song_cpp", type=Path)

    original = sub.add_parser("original")
    original.add_argument("candidate_root", type=Path)
    original.add_argument("original_root", type=Path)

    args = parser.parse_args()
    if args.command == "candidate":
        result = collect_candidate(args.root)
    elif args.command == "candidate-check":
        result = validate_candidate(args.root)
    elif args.command == "source":
        result = collect_source(args.root, args.sampler_cpp, args.song_cpp)
    elif args.command == "source-check":
        result = validate_source(args.root)
    else:
        result = validate_original(args.candidate_root, args.original_root)
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
