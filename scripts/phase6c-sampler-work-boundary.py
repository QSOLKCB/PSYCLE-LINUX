#!/usr/bin/env python3
"""Build and validate Phase 6C Sampler Voice::Tick / Voice::Work boundary evidence."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath

CONTRACT = "sequencer-sampler-work-boundary-isolation"
REFERENCE_BUILD = "Psycle 1.12.0 x86"
ROOT = Path(__file__).resolve().parents[1]
SOURCE_COMMIT = "7ac6d2c3553e2ee8dda55814d8e689919c345478"
SAMPLER_CPP_BLOB = "6cc0bd7328d01131c3d41b68f4e5d4189959e364"
SAMPLER_HPP_BLOB = "46da9fa80757b11ed146a21a529c70dbc6364f12"
SONG_STRUCTS_BLOB = "3ad8cd1b1a0a41bc2225cfd3070bf205cbebec31"
SOURCE_RECEIPT = "original-source-sampler-work-boundary.json"
PROJECTION = (
    ROOT
    / "phase6c"
    / "evidence"
    / "sequencer-sampler-work-boundary"
    / "observation.json"
)
FROZEN_MANIFEST = (
    ROOT
    / "phase6c"
    / "evidence"
    / "sequencer-sampler-work-boundary"
    / "historical-manifest.json"
)
FROZEN_PROJECTION_RUN_ID = 35752301481
FROZEN_PROJECTION_HEAD_SHA = "4cf71eba6f7b5f6ba14be6089cf37422b16b5ca6"
FROZEN_CANDIDATE_ARTIFACT = {
    "id": 10706755598,
    "name": "phase6c-delayed-retrigger-candidate",
    "digest": "sha256:ce4016b1a64ce67cca97079711c4852a55b9a7b47a3bb96c3bf8589741f16919",
}
FROZEN_ORIGINAL_ARTIFACT = {
    "id": 10706732946,
    "name": "phase6c-delayed-retrigger-original",
    "digest": "sha256:0865e2a07dcfd9ea72b1069db284e55dbd36c58d74fd5140b21ae0ba8705546d",
}
FROZEN_SOURCE_RECEIPT_SHA256 = (
    "ef64d01bf1a2f98ee2921821d7463a30e73d8e7478e21f6b4a9b9794d13d2313"
)
FROZEN_HISTORICAL_RECEIPT = {
    "path": "original-sampler-work-boundary-isolation.json",
    "sha256": "35874e0b2cfc0ed56c30a2dacf8c7110ecca398e9fd0a74df7187f60c14c7666",
    "diagnosis": "voice-tick-initialization-associated-exit",
    "interpretation_boundary": (
        "the source-bound release/delayed/ordinary ladder isolates whether the "
        "pinned original access violation begins before Voice::Tick, during "
        "Voice::Tick initialization, or only once controller.Work sample "
        "processing becomes reachable; it does not classify delayed/retrigger parity"
    ),
}
FROZEN_FIXTURE_IDENTITIES = {
    "release-no-active-voice": {
        "fixture_sha256": "edb7cd5fdf85b8379267d880bc535aa8f4d4b4a53c3048f21c0b9f5e201ed5cd",
        "candidate_receipt_sha256": "9829ecf6d108202fc3bf40902165873f79169a6c83c906b2b4ae6e4369bbdb8b",
        "original_receipt_sha256": "1cf7254ce4fdcac16cd13f26e4ea122078662ef4ff6cb76ba56c1ecc946bbebb",
        "outcome": "stable-finalized-output-process-alive",
        "process_exit_code": None,
        "observed_output": {
            "path": "sampler-work-boundary-release-no-active-voice/original-sampler-work-boundary-release-no-active-voice-1.wav",
            "size_bytes": 4868,
            "sha256": "f40e6f9f280c993b8c0ecdf6a54ba8314462803b7075e46d7db0c4a2bd9dfbae",
        },
    },
    "delayed-note-short": {
        "fixture_sha256": "f1021ee05ef3e76e71dca9f7d9f982decaef225124c8242788dbd3191813b195",
        "candidate_receipt_sha256": "3947bfa36f834c2abcaef8dffd338c179b820618f22e62e3557c2e0216108fc3",
        "original_receipt_sha256": "bcb6ce918df078e70e5b9ef99cf2a62f9ed81bf01330bb9577627c28245ff24a",
        "outcome": "reference-process-exited-during-render",
        "process_exit_code": -1073741819,
        "observed_output": {
            "path": "sampler-work-boundary-delayed-note-short/original-sampler-work-boundary-delayed-note-short-1.wav",
            "size_bytes": 0,
            "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        },
    },
    "delayed-note-long": {
        "fixture_sha256": "e4efab094ea6ce78c9c83b9eec4ffbc10abadc18a56bf2feec816830aa8543e9",
        "candidate_receipt_sha256": "b385e263a3818e98f2ddd74a5dced790e23c39105b84d6b9a6aac31d57b6dee2",
        "original_receipt_sha256": "041362cfa7e2f4e03397bb2957c947d40311ea3a3b15a715df4a10da87028d7d",
        "outcome": "reference-process-exited-during-render",
        "process_exit_code": -1073741819,
        "observed_output": {
            "path": "sampler-work-boundary-delayed-note-long/original-sampler-work-boundary-delayed-note-long-1.wav",
            "size_bytes": 0,
            "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        },
    },
    "ordinary-note-short": {
        "fixture_sha256": "6c6b606512ca3c87f91b7ea3ee41702b2e128fb741c1ac0a9ec0128ccaa1a3d3",
        "candidate_receipt_sha256": "4f19b1966322b98f65eb2f467d13771a1bb55c4a3e48860e8421c69083ad01d5",
        "original_receipt_sha256": "04bf960dabb83f5b0a3a54c367f686cd351a5606c2e65fc53570625a73ffca1f",
        "outcome": "reference-process-exited-during-render",
        "process_exit_code": -1073741819,
        "observed_output": {
            "path": "sampler-work-boundary-ordinary-note-short/original-sampler-work-boundary-ordinary-note-short-1.wav",
            "size_bytes": 0,
            "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        },
    },
}
QUALIFIED_DIAGNOSIS = "enabled-note-startup-pre-controller-work-associated-exit"
HISTORICAL_DIAGNOSIS = "voice-tick-initialization-associated-exit"
EXPECTED_ACCESS_VIOLATION_EXIT_CODE = -1073741819  # Windows 0xC0000005
SETTINGS = {
    "sample_rate": 44100,
    "bits_per_sample": 16,
    "channels": "mono-mix",
    "dither": False,
    "range": "entire-song",
}
VARIANTS = {
    "release-no-active-voice": {
        "title": "PSYCLE-LINUX Phase 6C Sampler work boundary release no active voice",
        "note": 120,
        "command": 0,
        "parameter": 0,
        "pattern_beats": 0.125,
        "source_boundary": "post-sample-gate-pre-voice-tick",
    },
    "delayed-note-short": {
        "title": "PSYCLE-LINUX Phase 6C Sampler work boundary delayed note short",
        "note": 60,
        "command": 0x0E,
        "parameter": 0xDF,
        "pattern_beats": 0.125,
        "source_boundary": "voice-tick-complete-work-pre-controller",
    },
    "delayed-note-long": {
        "title": "PSYCLE-LINUX Phase 6C Sampler work boundary delayed note long",
        "note": 60,
        "command": 0x0E,
        "parameter": 0xDF,
        "pattern_beats": 4.0,
        "source_boundary": "delayed-controller-work-reachable",
    },
    "ordinary-note-short": {
        "title": "PSYCLE-LINUX Phase 6C Sampler work boundary ordinary note short",
        "note": 60,
        "command": 0,
        "parameter": 0,
        "pattern_beats": 0.125,
        "source_boundary": "immediate-controller-work-reachable",
    },
}

SOURCE_SAMPLER_CPP_MARKERS = [
    "if ( !Global::song().samples.IsEnabled(data._inst) ) return;",
    "int idx = GetCurrentVoice(channel);",
    "if ( useVoice == -1 ) return;",
    "_voices[useVoice].Tick(&data, channel, _resampler, baseC, multicmdMem);",
    "_triggerNoteDelay = (Global::player().SamplesPerRow()/6.f)*(ite->_parameter & 0x0f);",
    "_envelope._stage = ENV_OFF;",
    "if (_triggerNoteDelay>0 )",
    "if (_sampleCounter >= _triggerNoteDelay)",
    "else if (_envelope._stage == ENV_OFF)",
    "controller.Work(pResamplerWork, left_output, right_output);",
]
SOURCE_SAMPLER_HPP_MARKERS = [
    "#define SAMPLER_CMD_EXTENDED",
    "#define SAMPLER_CMD_EXT_NOTEDELAY",
]
SOURCE_SONG_STRUCTS_MARKERS = [
    "release = 120,",
]
SOURCE_BOUNDARY = [
    "release note 120 with enabled sample and no active voice passes sample lookup but returns before Voice::Tick",
    "E-DF uses Sampler extended command 0x0E with note-delay parameter 0xDF",
    "DF sets trigger delay to 15/6 rows, or 2.5 rows",
    "one-row delayed-note fixture is shorter than the 2.5-row trigger delay",
    "Voice::Tick leaves the envelope ENV_OFF while trigger delay is nonzero",
    "Voice::Work returns while the delayed envelope remains ENV_OFF before controller.Work",
    "four-beat delayed-note fixture allows the 2.5-row delay to expire",
    "ordinary one-row note makes controller.Work reachable immediately",
]


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
        "sampler-work-boundary/"
        f"phase6c-sampler-work-boundary-{name}.psy"
    )


def candidate_receipt_name(name: str) -> str:
    return f"candidate-sampler-work-boundary-{name}.json"


def original_receipt_name(name: str) -> str:
    return f"original-sampler-work-boundary-{name}.json"


def expected_layout(name: str, spec: dict) -> dict:
    return {
        "bpm": 137,
        "lpb": 8,
        "tpb": 24,
        "pattern_beats": spec["pattern_beats"],
        "pattern_rows": spec["pattern_beats"] * 8,
        "sampler_machine": 0,
        "sample_slot_0": {
            "sample_rate": 44100,
            "sample_frames": 512,
            "impulse_frame": 256,
        },
        "serialized_instrument_0": True,
        "event": {
            "beat": 0.0,
            "note": spec["note"],
            "instrument": 0,
            "machine": 0,
            "command": spec["command"],
            "parameter": spec["parameter"],
        },
        "source_boundary": spec["source_boundary"],
    }


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    receipts = {}
    for name, spec in VARIANTS.items():
        relative = fixture_path(name)
        fixture = child(root, relative)
        if not fixture.is_file():
            raise ValueError("work-boundary fixture is missing: " + relative)
        raw = fixture.read_bytes()
        if not raw.startswith(b"PSY3SONG"):
            raise ValueError("work-boundary fixture is not PSY3: " + relative)
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
            "work_boundary_layout": expected_layout(name, spec),
            "purpose": (
                "split the PR #74 enabled-sample boundary between pre-Voice::Tick, "
                "Voice::Tick initialization with delayed Voice::Work early return, "
                "and controller.Work-reachable sample processing; diagnostic only"
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
            "work_boundary_layout": expected_layout(name, spec),
            "parity_status": "UNKNOWN",
        }
        for key, value in expected.items():
            if receipt.get(key) != value:
                raise ValueError(
                    f"work-boundary candidate {name} identity mismatch: {key}"
                )
        fixture = child(root, receipt["fixture"])
        raw = fixture.read_bytes()
        if not raw.startswith(b"PSY3SONG"):
            raise ValueError(f"work-boundary candidate {name} is not PSY3")
        if digest(raw) != receipt.get("fixture_sha256"):
            raise ValueError(f"work-boundary candidate {name} hash mismatch")
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
                "git_blob": SAMPLER_CPP_BLOB,
                "markers": list(SOURCE_SAMPLER_CPP_MARKERS),
            },
            "Sampler.hpp": {
                "git_blob": SAMPLER_HPP_BLOB,
                "markers": list(SOURCE_SAMPLER_HPP_MARKERS),
            },
            "SongStructs.hpp": {
                "git_blob": SONG_STRUCTS_BLOB,
                "markers": list(SOURCE_SONG_STRUCTS_MARKERS),
            },
        },
        "source_boundary": list(SOURCE_BOUNDARY),
        "parity_status": "UNKNOWN",
    }


def collect_source(
    root: Path, sampler_cpp: Path, sampler_hpp: Path, song_structs: Path
) -> dict:
    root = root.resolve()
    inputs = {
        "Sampler.cpp": (sampler_cpp.read_bytes(), SAMPLER_CPP_BLOB, SOURCE_SAMPLER_CPP_MARKERS),
        "Sampler.hpp": (sampler_hpp.read_bytes(), SAMPLER_HPP_BLOB, SOURCE_SAMPLER_HPP_MARKERS),
        "SongStructs.hpp": (song_structs.read_bytes(), SONG_STRUCTS_BLOB, SOURCE_SONG_STRUCTS_MARKERS),
    }
    for name, (data, expected_blob, markers) in inputs.items():
        if git_blob(data) != expected_blob:
            raise ValueError(f"pinned original {name} blob mismatch")
        text = data.replace(b"\r\n", b"\n").decode("utf-8")
        for marker in markers:
            if marker not in text:
                raise ValueError(f"pinned {name} source marker missing: {marker}")

    receipt = expected_source_receipt()
    write_new(root / SOURCE_RECEIPT, receipt)
    return receipt


def validate_source(root: Path) -> dict:
    receipt = read_json(root.resolve() / SOURCE_RECEIPT)
    if receipt != expected_source_receipt():
        raise ValueError(
            "Sampler work-boundary source receipt differs from canonical semantics"
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
                "original artifact work-boundary source receipt conflicts with candidate"
            )
    else:
        destination.write_bytes(source_bytes)
    if validate_source(original_root) != candidate_receipt:
        raise ValueError(
            "original artifact work-boundary source receipt changed during materialization"
        )
    return SOURCE_RECEIPT


def require_fresh_render_event_binding(attempt: dict, name: str) -> None:
    """Bind a fresh work-boundary attempt to its observed render dialog."""
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
        raise ValueError(f"{name}: fresh render lacks bound post-dispatch dialog evidence")


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


def require_attempt_prefix(
    attempt: object, name: str, *, historical: bool = False
) -> dict:
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
    if not historical:
        require_fresh_render_event_binding(attempt, name)
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


def validate_completed_render_attempt(
    attempt: dict,
    name: str,
    *,
    historical: bool = False,
    allow_post_completion_exit: bool = False,
) -> None:
    if not historical:
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
        or not (completed_and_closed or completed_with_teardown_failure)
        or not isinstance(attempt.get("stable_output_polls"), int)
        or isinstance(attempt.get("stable_output_polls"), bool)
        or attempt["stable_output_polls"] < 4
    ):
        raise ValueError(
            f"{name}: completed render lacks terminal completion evidence"
        )


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


def validate_observed_output(
    original_root: Path,
    name: str,
    attempt: dict,
    expected_filename: str,
) -> dict | None:
    observed = attempt.get("observed_output")
    relative = f"sampler-work-boundary-{name}/" + expected_filename
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

    release_live = outcomes["release-no-active-voice"] in live
    delayed_short_live = outcomes["delayed-note-short"] in live
    delayed_long_live = outcomes["delayed-note-long"] in live
    ordinary_short_live = outcomes["ordinary-note-short"] in live

    if not release_live:
        return "post-sample-gate-pre-voice-tick-associated-exit"
    if not delayed_short_live:
        return QUALIFIED_DIAGNOSIS
    if not delayed_long_live and not ordinary_short_live:
        return "controller-work-associated-exit"
    if delayed_long_live and not ordinary_short_live:
        return "immediate-undelayed-work-associated-exit"
    if not delayed_long_live and ordinary_short_live:
        return "delayed-work-transition-associated-exit"
    return "known-enabled-sample-crash-not-reproduced"


def interpretation_for_outcomes(
    outcomes: dict[str, str], diagnosis: str
) -> str:
    outcome_summary = ", ".join(
        f"{name}={outcomes[name]}" for name in VARIANTS
    )
    return (
        "the current work-boundary receipt set yields diagnosis "
        f"{diagnosis} from outcomes {outcome_summary}; this interpretation is "
        "limited to the current receipt set, does not rewrite the frozen "
        "historical projection, and does not classify delayed/retrigger parity"
    )


def validate_projection_semantic_envelope(projection: object) -> dict:
    expected_keys = {
        "schema_version",
        "phase",
        "contract",
        "parity_status",
        "evidence_run",
        "historical_receipt",
        "historical_diagnosis",
        "qualified_diagnosis",
        "source_identity",
        "fixtures",
        "qualified_interpretation",
    }
    if (
        not isinstance(projection, dict)
        or set(projection) != expected_keys
        or projection.get("schema_version") != 1
        or projection.get("phase") != "6C"
        or projection.get("contract") != CONTRACT
        or projection.get("parity_status") != "UNKNOWN"
        or projection.get("historical_diagnosis")
        != HISTORICAL_DIAGNOSIS
        or projection.get("qualified_diagnosis")
        != QUALIFIED_DIAGNOSIS
    ):
        raise ValueError("Sampler work-boundary projection identity mismatch")
    return projection


def validate_projection(
    candidate_root: Path, original_root: Path, summary: dict
) -> str:
    projection = validate_projection_semantic_envelope(
        read_json(PROJECTION)
    )

    source = projection.get("source_identity")
    if (
        not isinstance(source, dict)
        or source.get("source_commit") != SOURCE_COMMIT
        or source.get("Sampler.cpp") != SAMPLER_CPP_BLOB
        or source.get("Sampler.hpp") != SAMPLER_HPP_BLOB
        or source.get("SongStructs.hpp") != SONG_STRUCTS_BLOB
    ):
        raise ValueError("Sampler work-boundary projection source mismatch")

    fixtures = projection.get("fixtures")
    if not isinstance(fixtures, dict) or set(fixtures) != set(VARIANTS):
        raise ValueError("Sampler work-boundary projection fixture set mismatch")

    for name in VARIANTS:
        projected = fixtures[name]
        candidate_path = candidate_root / candidate_receipt_name(name)
        original_path = original_root / original_receipt_name(name)
        candidate = read_json(candidate_path)
        try:
            original_bytes = original_path.read_bytes()
        except FileNotFoundError as exc:
            raise ValueError(
                f"Sampler work-boundary projection original receipt missing: {name}"
            ) from exc
        result = summary["results"][name]
        if (
            not isinstance(projected, dict)
            or projected.get("fixture_sha256")
            != candidate.get("fixture_sha256")
            or projected.get("candidate_receipt_sha256")
            != digest(candidate_path.read_bytes())
            or projected.get("original_receipt_sha256")
            != digest(original_bytes)
            or projected.get("outcome") != result.get("outcome")
            or projected.get("process_exit_code")
            != result.get("process_exit_code")
            or projected.get("observed_output")
            != result.get("observed_output")
        ):
            raise ValueError(
                f"Sampler work-boundary projection mismatch: {name}"
            )

    interpretation = projection.get("qualified_interpretation")
    if (
        not isinstance(interpretation, dict)
        or interpretation.get("controller_work_reachable") is not False
        or interpretation.get("voice_work_entry") != "unresolved"
        or interpretation.get("voice_tick_fault_location") != "unresolved"
        or interpretation.get("voice_selection_fault_location") != "unresolved"
    ):
        raise ValueError(
            "Sampler work-boundary projection overstates fault location"
        )
    return str(PROJECTION.relative_to(ROOT))


def expected_frozen_manifest() -> dict:
    return {
        "schema_version": 1,
        "workflow_run_id": FROZEN_PROJECTION_RUN_ID,
        "head_sha": FROZEN_PROJECTION_HEAD_SHA,
        "candidate_artifact": dict(FROZEN_CANDIDATE_ARTIFACT),
        "original_artifact": dict(FROZEN_ORIGINAL_ARTIFACT),
        "source_receipt_sha256": FROZEN_SOURCE_RECEIPT_SHA256,
        "historical_receipt": dict(FROZEN_HISTORICAL_RECEIPT),
        "fixtures": {
            name: dict(value)
            for name, value in FROZEN_FIXTURE_IDENTITIES.items()
        },
    }


def validate_frozen_projection() -> dict:
    manifest = read_json(FROZEN_MANIFEST)
    if manifest != expected_frozen_manifest():
        raise ValueError(
            "Sampler work-boundary durable historical manifest mismatch"
        )

    projection = validate_projection_semantic_envelope(
        read_json(PROJECTION)
    )
    evidence_run = projection.get("evidence_run")
    if (
        not isinstance(evidence_run, dict)
        or evidence_run.get("workflow_run_id") != FROZEN_PROJECTION_RUN_ID
        or evidence_run.get("head_sha") != FROZEN_PROJECTION_HEAD_SHA
        or evidence_run.get("candidate_artifact") != FROZEN_CANDIDATE_ARTIFACT
        or evidence_run.get("original_artifact") != FROZEN_ORIGINAL_ARTIFACT
    ):
        raise ValueError(
            "Sampler work-boundary frozen artifact identity mismatch"
        )

    source = projection.get("source_identity")
    if (
        not isinstance(source, dict)
        or source.get("source_commit") != SOURCE_COMMIT
        or source.get("Sampler.cpp") != SAMPLER_CPP_BLOB
        or source.get("Sampler.hpp") != SAMPLER_HPP_BLOB
        or source.get("SongStructs.hpp") != SONG_STRUCTS_BLOB
        or source.get("source_receipt_sha256")
        != manifest["source_receipt_sha256"]
    ):
        raise ValueError(
            "Sampler work-boundary frozen source-receipt binding mismatch"
        )

    historical = projection.get("historical_receipt")
    if historical != manifest["historical_receipt"]:
        raise ValueError(
            "Sampler work-boundary historical receipt identity mismatch"
        )

    fixtures = projection.get("fixtures")
    if fixtures != manifest["fixtures"]:
        raise ValueError(
            "Sampler work-boundary frozen fixture receipt bindings mismatch"
        )

    interpretation = projection.get("qualified_interpretation")
    if (
        not isinstance(interpretation, dict)
        or interpretation.get("controller_work_reachable") is not False
        or interpretation.get("voice_work_entry") != "unresolved"
        or interpretation.get("voice_tick_fault_location") != "unresolved"
        or interpretation.get("voice_selection_fault_location") != "unresolved"
    ):
        raise ValueError(
            "Sampler work-boundary projection overstates fault location"
        )

    projection_path = str(PROJECTION.relative_to(ROOT))
    return {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "projection": projection_path,
        "workflow_run_id": FROZEN_PROJECTION_RUN_ID,
        "head_sha": FROZEN_PROJECTION_HEAD_SHA,
        "candidate_artifact": dict(FROZEN_CANDIDATE_ARTIFACT),
        "original_artifact": dict(FROZEN_ORIGINAL_ARTIFACT),
        "historical_receipt_sha256": historical["sha256"],
        "source_receipt_sha256": manifest["source_receipt_sha256"],
        "validation": "durable-repository-manifest",
        "parity_status": "UNKNOWN",
    }


def projection_for_run(
    candidate_root: Path,
    original_root: Path,
    summary: dict,
    replay_historical_projection: bool,
) -> tuple[str, str]:
    if replay_historical_projection:
        return (
            validate_projection(candidate_root, original_root, summary),
            "historical-byte-replay",
        )
    return (
        str(PROJECTION.relative_to(ROOT)),
        "fresh-rerun-semantic-only",
    )


def validate_original(
    candidate_root: Path,
    original_root: Path,
    *,
    replay_historical_projection: bool = True,
) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    validate_candidate(candidate_root)
    source_receipt = materialize_source_receipt(candidate_root, original_root)

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
        gate_name = f"sampler-work-boundary-{name}"
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
            raise ValueError(f"{name}: work-boundary runtime identity mismatch")

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
            raise ValueError(f"{name}: expected one work-boundary render attempt")
        filename = f"original-sampler-work-boundary-{name}-1.wav"
        outcome = runtime.get("outcome")
        raw_attempt = attempts[0]

        if (
            not replay_historical_projection
            and outcome == "inconclusive"
            and renders == []
        ):
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

        if not replay_historical_projection:
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
            attempt = require_attempt_prefix(
                attempt, name, historical=True
            )
        else:
            attempt = require_attempt_prefix(
                attempts[0], name, historical=True
            )

        if outcome == "rendered-once":
            if load_result != "accepted" or len(renders) != 1:
                raise ValueError(f"{name}: completed render state inconsistent")
            validate_completed_render_attempt(
                attempt, name, historical=replay_historical_projection
            )
            expected_relative = f"sampler-work-boundary-{name}/" + filename
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
                    attempt,
                    name,
                    historical=replay_historical_projection,
                    allow_post_completion_exit=True,
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
                expected_relative = f"sampler-work-boundary-{name}/" + filename
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
            raise ValueError(f"{name}: unexpected work-boundary outcome: {outcome!r}")

    outcomes = {name: result["outcome"] for name, result in results.items()}
    summary = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "original-sampler-work-boundary-isolation",
        "contract": CONTRACT,
        "reference_build": REFERENCE_BUILD,
        "source_receipt": source_receipt,
        "results": results,
        "diagnosis": diagnose(outcomes),
        "observed_exit_codes": sorted(
            {
                result["process_exit_code"]
                for result in results.values()
                if isinstance(result.get("process_exit_code"), int)
                and not isinstance(result.get("process_exit_code"), bool)
            }
        ),
        "interpretation_boundary": interpretation_for_outcomes(
            outcomes, diagnose(outcomes)
        ),
        "parity_status": "UNKNOWN",
    }
    (
        summary["projection"],
        summary["projection_validation"],
    ) = projection_for_run(
        candidate_root,
        original_root,
        summary,
        replay_historical_projection,
    )
    write_new(
        original_root / "original-sampler-work-boundary-isolation.json",
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
    source.add_argument("sampler_hpp", type=Path)
    source.add_argument("song_structs", type=Path)

    original = sub.add_parser("original")
    original.add_argument("candidate_root", type=Path)
    original.add_argument("original_root", type=Path)

    rerun = sub.add_parser("original-rerun")
    rerun.add_argument("candidate_root", type=Path)
    rerun.add_argument("original_root", type=Path)

    sub.add_parser("projection-check")

    args = parser.parse_args()
    if args.command == "candidate":
        result = collect_candidate(args.root)
    elif args.command == "candidate-check":
        result = validate_candidate(args.root)
    elif args.command == "source":
        result = collect_source(
            args.root, args.sampler_cpp, args.sampler_hpp, args.song_structs
        )
    elif args.command == "source-check":
        result = validate_source(args.root)
    elif args.command == "original-rerun":
        result = validate_original(
            args.candidate_root,
            args.original_root,
            replay_historical_projection=False,
        )
    elif args.command == "projection-check":
        result = validate_frozen_projection()
    else:
        result = validate_original(
            args.candidate_root,
            args.original_root,
            replay_historical_projection=True,
        )

    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
