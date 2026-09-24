#!/usr/bin/env python3
"""Negative controls for the Phase 6C offline-render execution analyzer."""
from __future__ import annotations

import hashlib
import json
import importlib.util
from pathlib import Path
import struct
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "phase6c-delayed-retrigger-render-evidence.py"

spec = importlib.util.spec_from_file_location("phase6c_render_evidence", SCRIPT)
assert spec is not None and spec.loader is not None
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

ISOLATION_SCRIPT = ROOT / "scripts" / "phase6c-delayed-retrigger-render-isolation.py"
isolation_spec = importlib.util.spec_from_file_location(
    "phase6c_render_isolation", ISOLATION_SCRIPT
)
assert isolation_spec is not None and isolation_spec.loader is not None
isolation = importlib.util.module_from_spec(isolation_spec)
isolation_spec.loader.exec_module(isolation)

SUBSTRATE_SCRIPT = ROOT / "scripts" / "phase6c-delayed-retrigger-render-substrate.py"
substrate_spec = importlib.util.spec_from_file_location(
    "phase6c_render_substrate", SUBSTRATE_SCRIPT
)
assert substrate_spec is not None and substrate_spec.loader is not None
substrate = importlib.util.module_from_spec(substrate_spec)
substrate_spec.loader.exec_module(substrate)

STARTUP_SCRIPT = ROOT / "scripts" / "phase6c-sampler-voice-startup.py"
startup_spec = importlib.util.spec_from_file_location(
    "phase6c_sampler_voice_startup", STARTUP_SCRIPT
)
assert startup_spec is not None and startup_spec.loader is not None
startup = importlib.util.module_from_spec(startup_spec)
startup_spec.loader.exec_module(startup)

WORK_BOUNDARY_SCRIPT = ROOT / "scripts" / "phase6c-sampler-work-boundary.py"
ORIGINAL_AUDIO_RENDER_SCRIPT = ROOT / "scripts" / "phase6c-original-audio-render.ps1"
work_boundary_spec = importlib.util.spec_from_file_location(
    "phase6c_sampler_work_boundary", WORK_BOUNDARY_SCRIPT
)
assert work_boundary_spec is not None and work_boundary_spec.loader is not None
work_boundary = importlib.util.module_from_spec(work_boundary_spec)
work_boundary_spec.loader.exec_module(work_boundary)


def wave_pcm16(frames: list[int], channels: int = 1, rate: int = 44100) -> bytes:
    if channels == 1:
        payload = struct.pack("<" + "h" * len(frames), *frames)
    else:
        interleaved = []
        for frame in frames:
            interleaved.extend([frame] * channels)
        payload = struct.pack("<" + "h" * len(interleaved), *interleaved)
    block_align = channels * 2
    byte_rate = rate * block_align
    fmt = struct.pack("<HHIIHH", 1, channels, rate, byte_rate, block_align, 16)
    return (
        b"RIFF"
        + struct.pack("<I", 4 + (8 + len(fmt)) + (8 + len(payload)))
        + b"WAVE"
        + b"fmt "
        + struct.pack("<I", len(fmt))
        + fmt
        + b"data"
        + struct.pack("<I", len(payload))
        + payload
    )


def witness_frames(onset_beats: list[float]) -> list[int]:
    beat_frames = 44100 * 60.0 / 137.0
    length = int(4.5 * beat_frames)
    frames = [0] * length
    for beat in onset_beats:
        start = int(round(beat * beat_frames))
        frames[start : start + 4] = [16000, -16000, 8000, -8000]
    return frames


def expect_failure(data: bytes, phrase: str) -> None:
    try:
        module.analyze_wave(data)
    except ValueError as exc:
        if phrase not in str(exc):
            raise AssertionError(f"unexpected failure: {exc}") from exc
    else:
        raise AssertionError("expected analyzer failure")


valid = wave_pcm16(
    witness_frames([0.0625, 1.0, 1.0625, 1.125, 2.0, 2.0625, 3.125])
)
analysis = module.analyze_wave(valid)
assert analysis["window_onset_counts"]["retrigger_beat_1"] == 3
assert analysis["window_onset_counts"]["retr_cont_beat_2"] == 2
assert len(analysis["onset_frames"]) == 7


def valid_render_event_binding() -> dict:
    return {
        "render_dialog_native_event_hook_armed": True,
        "render_dialog_event_message_pump_started": True,
        "render_dialog_dispatch_boundary_set": True,
        "render_dialog_dispatch_boundary_tick": 123456,
        "dialog_discovery": (
            "pumped-win-event-object-show-strictly-after-dispatch-tick"
        ),
        "preexisting_render_dialog_count": 0,
        "render_dialog_post_dispatch_observed_window_event_count": 1,
        "render_dialog_unresolved_post_dispatch_event_count": 0,
        "render_dialog_post_dispatch_event_count": 1,
        "selected_render_dialog_native_handle": 12345,
        "selected_render_dialog_runtime_id": [1, 2, 3],
    }


expect_failure(
    wave_pcm16(witness_frames([0.0625, 1.0, 2.0, 3.125])),
    "fewer than five",
)
expect_failure(
    wave_pcm16(witness_frames([0.0625, 1.0, 1.0625, 2.0, 3.125])),
    "retrigger and retrigger-continue",
)
expect_failure(
    wave_pcm16(
        witness_frames([0.0625, 1.0, 1.0625, 2.0, 2.0625, 3.125]),
        channels=2,
    ),
    "must be mono",
)

silent_declared = wave_pcm16([0] * len(witness_frames([])))
appended_impulses = wave_pcm16(
    witness_frames([0.0625, 1.0, 1.0625, 1.125, 2.0, 2.0625, 3.125])
)
appended_data_offset = appended_impulses.index(b"data")
expect_failure(
    silent_declared + appended_impulses[appended_data_offset:],
    "trailing bytes beyond declared RIFF extent",
)

duplicate_data = bytearray(valid)
duplicate_payload = struct.pack("<hhhh", 16000, -16000, 8000, -8000)
duplicate_data += b"data" + struct.pack("<I", len(duplicate_payload)) + duplicate_payload
struct.pack_into("<I", duplicate_data, 4, len(duplicate_data) - 8)
expect_failure(bytes(duplicate_data), "duplicate WAV data chunk")

predispatch_observer_failure = {
    "outcome": "inconclusive",
    "command_verified": True,
    "command_dispatched": False,
    "dialog_verified": False,
    "controls_configured": False,
    "save_invoked": False,
    "output": None,
    "observed_output": None,
    "process_exited": False,
    "process_exit_code": None,
    "diagnostics": [
        "could not initialize render-dialog observer: synthetic hook failure"
    ],
}
predispatch = module.validate_inconclusive_runtime(
    Path("."),
    {},
    {"deterministic": False, "renders": []},
    [predispatch_observer_failure],
)
assert predispatch["inconclusive_reason"] == "render-observer-initialization-failure"
assert predispatch["completed_renders"] == []

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    precommand_exit = {
        "outcome": "inconclusive",
        "command_verified": False,
        "command_dispatched": False,
        "dialog_verified": False,
        "controls_configured": False,
        "save_invoked": False,
        "output": None,
        "observed_output": None,
        "process_exited": True,
        "process_exit_code": -1073741819,
        "diagnostics": [module.PRECOMMAND_EXIT_DIAGNOSTIC],
    }
    precommand = module.validate_inconclusive_runtime(
        root,
        {"exit_code_before_termination": -1073741819},
        {"deterministic": False, "renders": []},
        [precommand_exit],
    )
    assert precommand["inconclusive_reason"] == (
        "process-exit-before-render-command-verification"
    )
    assert precommand["process_exit_code"] == -1073741819
    assert precommand["completed_renders"] == []
    assert precommand["binding_error"] is None

presave_automation_failure = {
    **valid_render_event_binding(),
    "outcome": "inconclusive",
    "command_verified": True,
    "command_dispatched": True,
    "dialog_verified": True,
    "controls_configured": False,
    "save_invoked": False,
    "output": None,
    "observed_output": None,
    "process_exited": False,
    "process_exit_code": None,
    "diagnostics": ["Render as Wav File controls were not found"],
}
presave = module.validate_presave_render_quarantine(
    presave_automation_failure
)
assert presave["inconclusive_reason"] == "pre-save-render-automation-failure"
assert presave["diagnostics"] == presave_automation_failure["diagnostics"]

presave_sealing_failure = dict(presave_automation_failure)
presave_sealing_failure["diagnostics"] = [
    "post-dispatch Render as Wav File EVENT_OBJECT_SHOW not observed",
    "could not seal render-dialog observer: synthetic pre-Save seal failure",
]
presave_sealing = module.validate_presave_render_quarantine(
    presave_sealing_failure
)
assert presave_sealing["inconclusive_reason"] == "render-observer-sealing-failure"
assert presave_sealing["diagnostics"] == presave_sealing_failure["diagnostics"]

verified_exit_attempt = {
    **valid_render_event_binding(),
    "outcome": "inconclusive",
    "command_verified": True,
    "command_dispatched": True,
    "dialog_verified": True,
    "controls_configured": True,
    "save_invoked": True,
}
module.validate_render_attempt(verified_exit_attempt, "inconclusive")

unbound_primary_attempt = dict(verified_exit_attempt)
unbound_primary_attempt.update(
    {
        "render_dialog_native_event_hook_armed": False,
        "render_dialog_event_message_pump_started": False,
        "dialog_discovery": "window-opened-event-after-successful-command-dispatch",
        "render_dialog_post_dispatch_observed_window_event_count": 0,
        "render_dialog_post_dispatch_event_count": 0,
    }
)
try:
    module.validate_render_attempt(unbound_primary_attempt, "inconclusive")
except ValueError as exc:
    assert "bound post-dispatch dialog evidence" in str(exc)
else:
    raise AssertionError("expected primary render event-binding rejection")

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    output_dir = root / "delayed-retrigger-execution"
    output_dir.mkdir()
    output = output_dir / "original-delayed-retrigger-execution-1.wav"
    output.write_bytes(b"")
    observed = {
        "path": output.name,
        "size_bytes": 0,
        "sha256": hashlib.sha256(b"").hexdigest(),
    }
    validated = module.validate_observed_output(root, observed)
    assert validated["size_bytes"] == 0

    bad = dict(observed)
    bad["size_bytes"] = 1
    try:
        module.validate_observed_output(root, bad)
    except ValueError as exc:
        assert "binding mismatch" in str(exc)
    else:
        raise AssertionError("expected observed-output binding failure")

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    output_dir = root / "delayed-retrigger-execution"
    output_dir.mkdir()

    # Ambiguous fresh dialog binding after dispatch is retained as UNKNOWN.
    ambiguous_name = "original-delayed-retrigger-execution-1.wav"
    ambiguous_path = output_dir / ambiguous_name
    ambiguous_path.write_bytes(b"partial")
    ambiguous_attempt = {
        **valid_render_event_binding(),
        "outcome": "inconclusive",
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": False,
        "controls_configured": False,
        "save_invoked": False,
        "process_exited": False,
        "process_exit_code": None,
        "output": None,
        "diagnostics": ["multiple post-dispatch Psycle window-show events observed"],
        "observed_output": {
            "path": ambiguous_name,
            "size_bytes": len(b"partial"),
            "sha256": hashlib.sha256(b"partial").hexdigest(),
        },
        "render_dialog_post_dispatch_observed_window_event_count": 2,
    }
    ambiguous = module.validate_inconclusive_runtime(
        root,
        {"exit_code_before_termination": None},
        {"deterministic": False, "renders": []},
        [ambiguous_attempt],
    )
    assert ambiguous["inconclusive_reason"] == "ambiguous-render-dialog-binding"
    assert ambiguous["binding_error"] is not None
    assert ambiguous["observed_output"]["path"].endswith(ambiguous_name)

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    (root / "delayed-retrigger-execution").mkdir()

    # A real reference exit before Save Wave is non-evidentiary but preserved.
    pre_save_exit = {
        **valid_render_event_binding(),
        "outcome": "inconclusive",
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": False,
        "save_invoked": False,
        "process_exited": True,
        "process_exit_code": -1073741819,
        "output": None,
        "diagnostics": ["reference exited before Save Wave invocation"],
        "observed_output": None,
    }
    exited = module.validate_inconclusive_runtime(
        root,
        {"exit_code_before_termination": -1073741819},
        {"deterministic": False, "renders": []},
        [pre_save_exit],
    )
    assert exited["inconclusive_reason"] == "process-exit-before-completed-save"
    assert exited["process_exit_code"] == -1073741819
    assert exited["binding_error"] is None

    clean_pre_save_exit = dict(pre_save_exit)
    clean_pre_save_exit["process_exit_code"] = 0
    clean_exited = module.validate_inconclusive_runtime(
        root,
        {"exit_code_before_termination": 0},
        {"deterministic": False, "renders": []},
        [clean_pre_save_exit],
    )
    assert clean_exited["inconclusive_reason"] == "process-exit-before-completed-save"
    assert clean_exited["process_exit_code"] == 0
    assert clean_exited["binding_error"] is None

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    (root / "delayed-retrigger-execution").mkdir()

    # A bound dialog can fail later in UI automation without invalidating evidence.
    post_binding = {
        **valid_render_event_binding(),
        "outcome": "inconclusive",
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": False,
        "save_invoked": False,
        "process_exited": False,
        "process_exit_code": None,
        "output": None,
        "diagnostics": ["Render as Wav File controls were not found"],
        "observed_output": None,
    }
    quarantined = module.validate_inconclusive_runtime(
        root,
        {},
        {"deterministic": False, "renders": []},
        [post_binding],
    )
    assert quarantined["inconclusive_reason"] == "post-binding-render-automation-failure"
    assert quarantined["binding_error"] is None

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    output_dir = root / "delayed-retrigger-execution"
    output_dir.mkdir()

    first_name = "original-delayed-retrigger-execution-1.wav"
    first_path = output_dir / first_name
    first_data = b"completed-render"
    first_path.write_bytes(first_data)
    first_hash = hashlib.sha256(first_data).hexdigest()
    first_binding = {
        "path": "delayed-retrigger-execution/" + first_name,
        "sha256": first_hash,
    }
    first_attempt = {
        **valid_render_event_binding(),
        "outcome": "rendered",
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": True,
        "save_invoked": True,
        "process_exited": False,
        "process_exit_code": None,
        "stable_output_polls": 4,
        "dialog_closed": True,
        "close_control_seen": True,
        "close_uia_invoked": True,
        "diagnostics": [],
        "output": {"path": first_name, "sha256": first_hash},
    }

    incomplete_terminal_attempt = dict(first_attempt)
    incomplete_terminal_attempt.update(
        {
            "stable_output_polls": 0,
            "dialog_closed": False,
            "close_control_seen": False,
            "close_uia_invoked": False,
            "diagnostics": ["offline render did not reach a stable completed output"],
        }
    )
    try:
        module.validate_completed_render_attempt(
            root, incomplete_terminal_attempt, first_binding, 1
        )
    except ValueError as exc:
        assert "terminal completion evidence" in str(exc)
    else:
        raise AssertionError(
            "expected incomplete primary render terminal-evidence rejection"
        )

    try:
        module.validate_inconclusive_runtime(
            root,
            {},
            {"deterministic": False, "renders": [first_binding]},
            [incomplete_terminal_attempt],
        )
    except ValueError as exc:
        assert "terminal completion evidence" in str(exc)
    else:
        raise AssertionError(
            "expected inconclusive path to reject incomplete retained render"
        )

    second_name = "original-delayed-retrigger-execution-2.wav"
    second_path = output_dir / second_name
    second_path.write_bytes(b"")
    second_hash = hashlib.sha256(b"").hexdigest()
    second_attempt = {
        **valid_render_event_binding(),
        "outcome": "inconclusive",
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": True,
        "save_invoked": True,
        "process_exited": True,
        "process_exit_code": -1073741819,
        "output": None,
        "diagnostics": ["reference exited during offline render"],
        "observed_output": {
            "path": second_name,
            "size_bytes": 0,
            "sha256": second_hash,
        },
    }
    receipt = {
        "load_result": "inconclusive",
        "observation": "reference-process-exited-before-harness-termination",
        "exit_code_before_termination": -1073741819,
    }
    runtime = {
        "deterministic": False,
        "renders": [first_binding],
    }
    exit_evidence = module.validate_process_exit_runtime(
        root, receipt, runtime, [first_attempt, second_attempt]
    )
    assert exit_evidence["attempt_number"] == 2
    assert exit_evidence["completed_render_count"] == 1
    assert exit_evidence["completed_render_sha256"] == [first_hash]
    assert exit_evidence["observed_output"]["path"].endswith(second_name)

    second_path.unlink()
    no_output_attempt = dict(second_attempt)
    no_output_attempt["process_exit_code"] = 0
    no_output_attempt["observed_output"] = None
    no_output_receipt = dict(receipt)
    no_output_receipt["exit_code_before_termination"] = 0
    no_output_exit = module.validate_process_exit_runtime(
        root, no_output_receipt, runtime, [first_attempt, no_output_attempt]
    )
    assert no_output_exit["attempt_number"] == 2
    assert no_output_exit["process_exit_code"] == 0
    assert no_output_exit["observed_output"] is None

    second_path.write_bytes(b"unbound")
    try:
        module.validate_process_exit_runtime(
            root, no_output_receipt, runtime, [first_attempt, no_output_attempt]
        )
    except ValueError as exc:
        assert "unbound output file" in str(exc)
    else:
        raise AssertionError(
            "expected process-exit receipt with unbound output file rejection"
        )
    second_path.unlink()

    try:
        module.validate_process_exit_runtime(
            root, receipt, {"deterministic": False, "renders": []},
            [first_attempt, second_attempt],
        )
    except ValueError as exc:
        assert "partial-render shape" in str(exc)
    else:
        raise AssertionError("expected second-render exit shape failure")

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    output_dir = root / "delayed-retrigger-execution"
    output_dir.mkdir()
    first_name = "original-delayed-retrigger-execution-1.wav"
    first_path = output_dir / first_name
    first_data = wave_pcm16([0] * 64)
    first_path.write_bytes(first_data)
    first_hash = hashlib.sha256(first_data).hexdigest()
    first_binding = {
        "path": "delayed-retrigger-execution/" + first_name,
        "sha256": first_hash,
    }
    teardown_attempt = {
        **valid_render_event_binding(),
        "outcome": "rendered",
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": True,
        "save_invoked": True,
        "process_exited": False,
        "process_exit_code": None,
        "stable_output_polls": 4,
        "dialog_closed": False,
        "close_control_seen": True,
        "close_uia_invoked": True,
        "diagnostics": [
            "render output finalized and Close control was verified, "
            "but dialog teardown did not complete"
        ],
        "output": {"path": first_name, "sha256": first_hash},
    }
    retained = module.validate_inconclusive_runtime(
        root,
        {},
        {"deterministic": False, "renders": [first_binding]},
        [teardown_attempt],
    )
    assert retained["inconclusive_reason"] == "incomplete-repeated-render-procedure"
    assert retained["completed_renders"] == [first_binding]
    assert retained["diagnostics"] == teardown_attempt["diagnostics"]
    assert retained["completed_render_analyses"][0]["frame_count"] == 64

    clean_attempt = dict(teardown_attempt)
    clean_attempt["dialog_closed"] = True
    clean_attempt["diagnostics"] = []

    inspection_failure_attempt = dict(clean_attempt)
    inspection_failure_attempt["diagnostics"] = [
        "could not inspect reference process after render attempt: synthetic refresh failure"
    ]
    inspection_failure_attempt["observed_output"] = {
        "path": first_name,
        "size_bytes": len(first_data),
        "sha256": first_hash,
    }
    inspection_failure = module.validate_inconclusive_runtime(
        root,
        {},
        {"deterministic": False, "renders": [first_binding]},
        [inspection_failure_attempt],
    )
    assert inspection_failure["inconclusive_reason"] == (
        "post-render-process-inspection-failure"
    )
    assert inspection_failure["completed_renders"] == [first_binding]
    assert inspection_failure["process_exit_code"] is None
    assert inspection_failure["observed_output"]["sha256"] == first_hash

    second_init_failure = {
        "outcome": "inconclusive",
        "command_verified": True,
        "command_dispatched": False,
        "dialog_verified": False,
        "controls_configured": False,
        "save_invoked": False,
        "output": None,
        "observed_output": None,
        "process_exited": False,
        "process_exit_code": None,
        "diagnostics": [
            "could not initialize render-dialog observer: synthetic second-attempt failure",
            "could not inspect reference process after render attempt: synthetic refresh failure",
        ],
    }
    combined_predispatch = module.validate_predispatch_observer_failure(
        second_init_failure
    )
    assert combined_predispatch["diagnostics"] == second_init_failure["diagnostics"]
    invalid_second_init_failure = dict(second_init_failure)
    invalid_second_init_failure["diagnostics"] = [
        second_init_failure["diagnostics"][0],
        "unrelated infrastructure diagnostic",
    ]
    try:
        module.validate_predispatch_observer_failure(
            invalid_second_init_failure
        )
    except ValueError as exc:
        assert "pre-dispatch observer initialization failure shape is invalid" in str(exc)
    else:
        raise AssertionError(
            "expected unrelated secondary pre-dispatch diagnostic rejection"
        )

    second_init = module.validate_inconclusive_runtime(
        root,
        {},
        {"deterministic": False, "renders": [first_binding]},
        [clean_attempt, second_init_failure],
    )
    assert second_init["inconclusive_reason"] == (
        "render-observer-initialization-failure"
    )
    assert second_init["completed_renders"] == [first_binding]
    assert second_init["completed_render_analyses"][0]["frame_count"] == 64
    assert second_init["diagnostics"] == second_init_failure["diagnostics"]

    second_name = "original-delayed-retrigger-execution-2.wav"
    second_path = output_dir / second_name
    second_data = wave_pcm16([1] + [0] * 63)
    second_path.write_bytes(second_data)
    second_hash = hashlib.sha256(second_data).hexdigest()
    second_binding = {
        "path": "delayed-retrigger-execution/" + second_name,
        "sha256": second_hash,
    }
    second_attempt = dict(clean_attempt)
    second_attempt["output"] = {"path": second_name, "sha256": second_hash}
    try:
        module.validate_inconclusive_runtime(
            root,
            {},
            {
                "deterministic": False,
                "renders": [first_binding, second_binding],
            },
            [teardown_attempt, second_attempt],
        )
    except ValueError as exc:
        assert "primary later render follows a non-clean completed render" in str(exc)
    else:
        raise AssertionError(
            "expected impossible second render after teardown failure rejection"
        )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    output_dir = root / "delayed-retrigger-execution"
    output_dir.mkdir()
    name = "original-delayed-retrigger-execution-1.wav"
    path = output_dir / name
    data = wave_pcm16([0] * 64)
    path.write_bytes(data)
    sha = hashlib.sha256(data).hexdigest()
    binding = {
        "path": "delayed-retrigger-execution/" + name,
        "sha256": sha,
    }
    post_completion_exit_attempt = {
        **valid_render_event_binding(),
        "outcome": "rendered",
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": True,
        "save_invoked": True,
        "process_exited": True,
        "process_exit_code": -1073741819,
        "stable_output_polls": 4,
        "dialog_closed": True,
        "close_control_seen": True,
        "close_uia_invoked": True,
        "diagnostics": [],
        "output": {"path": name, "sha256": sha},
        "observed_output": {
            "path": name,
            "size_bytes": len(data),
            "sha256": sha,
        },
    }
    retained_exit = module.validate_inconclusive_runtime(
        root,
        {"exit_code_before_termination": -1073741819},
        {"deterministic": False, "renders": [binding]},
        [post_completion_exit_attempt],
    )
    assert retained_exit["inconclusive_reason"] == "process-exit-after-completed-render"
    assert retained_exit["completed_renders"] == [binding]
    assert retained_exit["process_exit_code"] == -1073741819
    assert retained_exit["observed_output"]["sha256"] == sha

    clean_exit_attempt = dict(post_completion_exit_attempt)
    clean_exit_attempt["process_exit_code"] = 0
    retained_clean_exit = module.validate_inconclusive_runtime(
        root,
        {"exit_code_before_termination": 0},
        {"deterministic": False, "renders": [binding]},
        [clean_exit_attempt],
    )
    assert retained_clean_exit["inconclusive_reason"] == (
        "process-exit-after-completed-render"
    )
    assert retained_clean_exit["completed_renders"] == [binding]
    assert retained_clean_exit["process_exit_code"] == 0

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    missing_attempt = {"observed_output": None}
    missing = isolation.validate_failed_observed_output(
        root,
        "control",
        missing_attempt,
        "original-delayed-retrigger-isolation-control-1.wav",
    )
    assert missing is None

    output_dir = root / "delayed-retrigger-isolation-control"
    output_dir.mkdir()
    output = output_dir / "original-delayed-retrigger-isolation-control-1.wav"
    output.write_bytes(b"")
    try:
        isolation.validate_failed_observed_output(
            root,
            "control",
            missing_attempt,
            output.name,
        )
    except ValueError as exc:
        assert "unbound output file" in str(exc)
    else:
        raise AssertionError("expected unbound failed-render output rejection")

    bound_attempt = {
        "observed_output": {
            "path": output.name,
            "size_bytes": 0,
            "sha256": hashlib.sha256(b"").hexdigest(),
        }
    }
    bound = isolation.validate_failed_observed_output(
        root,
        "control",
        bound_attempt,
        output.name,
    )
    assert bound["size_bytes"] == 0
    assert bound["path"].endswith(output.name)

for consumer_path in (
    ROOT / "scripts" / "phase6c-delayed-retrigger-render-isolation.py",
    ROOT / "scripts" / "phase6c-delayed-retrigger-render-substrate.py",
    ROOT / "scripts" / "phase6c-sampler-voice-startup.py",
    ROOT / "scripts" / "phase6c-sampler-work-boundary.py",
):
    consumer_source = consumer_path.read_text(encoding="utf-8")
    assert "validate_precommand_process_exit(" in consumer_source
    assert "post-render-process-inspection-failure" in consumer_source

render_helper_source = ORIGINAL_AUDIO_RENDER_SCRIPT.read_text(encoding="utf-8")
observer_builder_source = (
    ROOT / "scripts" / "phase6c-build-delayed-retrigger-observer.py"
).read_text(encoding="utf-8")
assert observer_builder_source.count("$postRenderInspectionFailure = @(") >= 5
assert observer_builder_source.count(
    '$runtimeOutcome = if ($postRenderInspectionFailure) {'
) >= 5

assert "$preexistingRenderDialogHandles" not in render_helper_source
assert "Test-Phase6cSameAutomationElement" not in render_helper_source
assert "[System.Windows.Automation.Automation]::Compare" not in render_helper_source
assert "Automation.AddAutomationEventHandler" not in render_helper_source
assert "WindowPattern.WindowOpenedEvent" not in render_helper_source
assert "SetWinEventHook" in render_helper_source
assert "EVENT_OBJECT_SHOW" in render_helper_source
assert "WINEVENT_OUTOFCONTEXT" in render_helper_source
assert "GetMessage(" in render_helper_source
assert "PeekMessage(" in render_helper_source
assert "PM_NOREMOVE" in render_helper_source
assert "DispatchMessage(" in render_helper_source
assert "PostThreadMessage(" in render_helper_source
assert "WM_QUIT" not in render_helper_source
assert "WM_APP_STOP" in render_helper_source
assert "WM_APP_DRAINED" in render_helper_source
assert "Phase6cRenderWinEventPump" in render_helper_source
assert "render_dialog_event_message_pump_started" in render_helper_source
assert "eventTime" in render_helper_source
assert "GetTickCount" in render_helper_source
assert "TickStrictlyAfter(eventTime, dispatchBoundaryTick)" in render_helper_source
assert "return unchecked((int)(value - boundary)) > 0;" in render_helper_source
assert "Phase6cRenderWindowOpenedObserver" in render_helper_source
assert "could not initialize render-dialog observer:" in render_helper_source
assert "BeginDispatchBoundary" in render_helper_source
assert "CancelDispatchBoundary" in render_helper_source
assert "render_dialog_native_event_hook_armed" in render_helper_source
assert "render_dialog_dispatch_boundary_set" in render_helper_source
assert "render_dialog_dispatch_boundary_tick" in render_helper_source
assert "render_dialog_post_dispatch_observed_window_event_count" in render_helper_source
assert "render_dialog_unresolved_post_dispatch_event_count" in render_helper_source
assert "render_dialog_post_dispatch_event_count" in render_helper_source
assert "catch [System.Windows.Automation.ElementNotAvailableException]" in render_helper_source
assert "selected_render_dialog_native_handle" in render_helper_source
assert "selected_render_dialog_runtime_id" in render_helper_source
assert (
    "pumped-win-event-object-show-strictly-after-dispatch-tick"
    in render_helper_source
)

record_start = render_helper_source.index(
    "private bool RecordQualifiedObservedWindow("
)
observer_start = render_helper_source.index("private void OnWinEvent(")
observer_end = render_helper_source.index("private void FlushPump()", observer_start)
record_source = render_helper_source[record_start:observer_start]
observer_source = render_helper_source[observer_start:observer_end]
assert "postDispatchObservedWindowEventCount += 1;" in record_source
assert "unresolvedPostDispatchEventCount += 1;" in record_source
owner_index = observer_source.index("GetWindowThreadProcessId(window, out owner)")
root_index = observer_source.index("IntPtr root = GetAncestor(window, GA_ROOT)")
child_filter_index = observer_source.index("if (root != window)")
top_level_count_index = observer_source.index(
    "RecordQualifiedObservedWindow(eventTime, false)"
)
assert owner_index < root_index < child_filter_index < top_level_count_index
assert observer_source.count(
    "RecordQualifiedObservedWindow(eventTime, true)"
) == 2
assert "postDispatchObservedWindowEventCount > 1" in render_helper_source

pump_start = render_helper_source.index("private void Pump()")
pump_end = render_helper_source.index(
    "private static bool TickStrictlyAfter", pump_start
)
pump_source = render_helper_source[pump_start:pump_end]
assert "PeekMessage(" in pump_source
assert "SetWinEventHook(" in pump_source
assert "GetMessage(" in pump_source
assert "DispatchMessage(" in pump_source
stop_message_index = pump_source.index(
    "message.message == WM_APP_STOP"
)
unhook_index = pump_source.index(
    "UnhookWinEvent(current)", stop_message_index
)
drain_post_index = pump_source.index(
    "WM_APP_DRAINED", unhook_index
)
drained_message_index = pump_source.index(
    "message.message == WM_APP_DRAINED", drain_post_index
)
assert (
    stop_message_index
    < unhook_index
    < drain_post_index
    < drained_message_index
)

seal_start = render_helper_source.index("public int[] Seal()")
seal_end = render_helper_source.index("public void Dispose()", seal_start)
seal_source = render_helper_source[seal_start:seal_end]
stop_request_index = seal_source.index("WM_APP_STOP")
stop_wait_index = seal_source.index("if (!stopped.Wait(5000))")
stop_error_index = seal_source.index("if (stopError != 0)")
disposed_assignment_index = seal_source.index("disposed = true;")
boundary_cancel_index = seal_source.index("dispatchBoundarySet = false;")
assert (
    stop_request_index
    < stop_wait_index
    < stop_error_index
    < disposed_assignment_index
    < boundary_cancel_index
)

invoke_start = render_helper_source.index("public static bool Invoke(")
invoke_end = render_helper_source.index("public static string SetText(", invoke_start)
invoke_source = render_helper_source[invoke_start:invoke_end]
boundary_index = invoke_source.index("observer.BeginDispatchBoundary();")
post_index = invoke_source.index("PostMessage(")
assert boundary_index < post_index
assert "observer.CancelDispatchBoundary();" in invoke_source

dispatch_index = render_helper_source.index("$result.command_dispatched = $true")
tick_index = render_helper_source.index(
    "$result.render_dialog_dispatch_boundary_tick = [uint32]("
)
wait_index = render_helper_source.index(
    "$dialog = Wait-Phase6cOpenedRenderDialog $Process $dialogObserver 40"
)
assert dispatch_index < tick_index < wait_index

wait_start = render_helper_source.index("function Wait-Phase6cOpenedRenderDialog(")
wait_end = render_helper_source.index(
    "function Get-Phase6cLiveRenderDialog(", wait_start
)
wait_source = render_helper_source[wait_start:wait_end]
assert "Start-Sleep -Milliseconds 100" not in wait_source
assert "$Observer.ThrowIfAmbiguous()" in wait_source
assert "TakeNextHandle" in wait_source

assert render_helper_source.count("$dialogObserver.ThrowIfAmbiguous()") >= 3
assert "$eventSnapshot = $dialogObserver.Seal()" in render_helper_source
assert (
    "$result.render_dialog_post_dispatch_observed_window_event_count = "
    "[int]$eventSnapshot[0]"
    in render_helper_source
)
assert (
    "$result.render_dialog_unresolved_post_dispatch_event_count = "
    "[int]$eventSnapshot[1]"
    in render_helper_source
)
assert (
    "$result.render_dialog_post_dispatch_event_count = [int]$eventSnapshot[2]"
    in render_helper_source
)
assert (
    'render_dialog_unresolved_post_dispatch_event_count -ne 0'
    in render_helper_source
)
assert (
    'render_dialog_post_dispatch_event_count -ne 1'
    in render_helper_source
)

valid_substrate_completed = {
    **valid_render_event_binding(),
    "outcome": "rendered",
    "command_verified": True,
    "command_dispatched": True,
    "dialog_verified": True,
    "controls_configured": True,
    "save_invoked": True,
    "process_exited": False,
    "process_exit_code": None,
    "stable_output_polls": 4,
    "dialog_closed": True,
    "close_control_seen": True,
    "close_uia_invoked": True,
    "diagnostics": [],
}
substrate.require_attempt_prefix(valid_substrate_completed, "master-only")
substrate.validate_completed_render_attempt(valid_substrate_completed, "master-only")
isolation.validate_completed_render_attempt(valid_substrate_completed, "control")
post_exit_completed = {
    **valid_substrate_completed,
    "process_exited": True,
    "process_exit_code": -1073741819,
}
substrate.validate_completed_render_attempt(
    post_exit_completed,
    "master-only",
    allow_post_completion_exit=True,
)
isolation.validate_completed_render_attempt(
    post_exit_completed,
    "control",
    allow_post_completion_exit=True,
)
inspection_failure_completed = {
    **valid_substrate_completed,
    "diagnostics": [
        "could not inspect reference process after render attempt: synthetic refresh failure"
    ],
}
for check, name in (
    (isolation.validate_completed_render_attempt, "control"),
    (substrate.validate_completed_render_attempt, "master-only"),
    (startup.validate_completed_render_attempt, "note-sample-default-inst"),
    (work_boundary.validate_completed_render_attempt, "release-no-active-voice"),
):
    try:
        check(inspection_failure_completed, name)
    except ValueError as exc:
        assert "process-inspection failure" in str(exc)
    else:
        raise AssertionError(
            "expected unquarantined process-inspection diagnostic rejection"
        )
    check(
        inspection_failure_completed,
        name,
        allow_process_inspection_failure=True,
    )

clean_post_exit_completed = {
    **valid_substrate_completed,
    "process_exited": True,
    "process_exit_code": 0,
}
substrate.validate_completed_render_attempt(
    clean_post_exit_completed,
    "master-only",
    allow_post_completion_exit=True,
)
isolation.validate_completed_render_attempt(
    clean_post_exit_completed,
    "control",
    allow_post_completion_exit=True,
)
for altered, phrase in (
    ({"stable_output_polls": 0}, "terminal completion evidence"),
    ({"close_control_seen": False}, "terminal completion evidence"),
    ({"close_uia_invoked": False}, "terminal completion evidence"),
    (
        {"render_dialog_post_dispatch_event_count": 2},
        "bound post-dispatch dialog evidence",
    ),
):
    invalid = {**valid_substrate_completed, **altered}
    try:
        substrate.validate_completed_render_attempt(invalid, "master-only")
    except ValueError as exc:
        assert phrase in str(exc)
    else:
        raise AssertionError("expected invalid substrate terminal state rejection")

assert substrate.diagnose({
    "master-only": "reference-process-exited-during-render",
    "sampler-empty": "reference-process-exited-during-render",
    "sample-state": "reference-process-exited-during-render",
    "ordinary-note": "reference-process-exited-during-render",
}) == "master-only-associated-reference-exit"

assert substrate.diagnose({
    "master-only": "stable-finalized-output-process-alive",
    "sampler-empty": "reference-process-exited-during-render",
    "sample-state": "reference-process-exited-during-render",
    "ordinary-note": "reference-process-exited-during-render",
}) == "sampler-presence-associated-reference-exit"

assert substrate.diagnose({
    "master-only": "stable-finalized-output-process-alive",
    "sampler-empty": "stable-finalized-output-process-alive",
    "sample-state": "reference-process-exited-during-render",
    "ordinary-note": "reference-process-exited-during-render",
}) == "sample-state-associated-reference-exit"

assert substrate.diagnose({
    "master-only": "stable-finalized-output-process-alive",
    "sampler-empty": "stable-finalized-output-process-alive",
    "sample-state": "stable-finalized-output-process-alive",
    "ordinary-note": "reference-process-exited-during-render",
}) == "ordinary-note-associated-reference-exit-after-stable-output-controls"

assert substrate.diagnose({
    "master-only": "stable-finalized-output-process-alive",
    "sampler-empty": "stable-finalized-output-process-alive",
    "sample-state": "stable-finalized-output-process-alive",
    "ordinary-note": "stable-finalized-output-process-alive",
}) == "all-substrate-controls-survive-full-witness-detail-remains"

assert substrate.diagnose({
    "master-only": "reference-process-exited-during-render",
    "sampler-empty": "stable-finalized-output-process-alive",
    "sample-state": "reference-process-exited-during-render",
    "ordinary-note": "reference-process-exited-during-render",
}) == "nonmonotonic-substrate-result"

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    attempt = {"observed_output": None}
    filename = "original-delayed-retrigger-substrate-master-only-1.wav"
    assert substrate.validate_failed_observed_output(
        root, "master-only", attempt, filename
    ) is None

    output_dir = root / "delayed-retrigger-substrate-master-only"
    output_dir.mkdir()
    output = output_dir / filename
    output.write_bytes(b"")
    try:
        substrate.validate_failed_observed_output(
            root, "master-only", attempt, filename
        )
    except ValueError as exc:
        assert "unbound output file" in str(exc)
    else:
        raise AssertionError("expected substrate unbound-output rejection")

    attempt["observed_output"] = {
        "path": filename,
        "size_bytes": 0,
        "sha256": hashlib.sha256(b"").hexdigest(),
    }
    bound = substrate.validate_failed_observed_output(
        root, "master-only", attempt, filename
    )
    assert bound["size_bytes"] == 0

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    output_dir = root / "delayed-retrigger-substrate-master-only"
    output_dir.mkdir()
    filename = "original-delayed-retrigger-substrate-master-only-1.wav"
    output = output_dir / filename
    data = wave_pcm16([0] * 64)
    output.write_bytes(data)
    attempt = {
        "process_exited": False,
        "process_exit_code": None,
        "dialog_closed": False,
        "stable_output_polls": 4,
        "observed_output": {
            "path": filename,
            "size_bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        },
    }
    stable = substrate.validate_stable_alive_output(
        root,
        "master-only",
        attempt,
        filename,
        module,
    )
    assert stable["frame_count"] == 64
    assert stable["nonzero_frame_count"] == 0
    assert stable["observed_output"]["sha256"] == hashlib.sha256(data).hexdigest()

assert startup.diagnose({
    "note-no-previous-inst": "reference-process-exited-during-render",
    "note-missing-sample": "reference-process-exited-during-render",
    "note-sample-default-inst": "reference-process-exited-during-render",
    "note-sample-serialized-inst": "reference-process-exited-during-render",
}) == "sampler-dispatch-before-instrument-resolution-associated-exit"

assert startup.diagnose({
    "note-no-previous-inst": "stable-finalized-output-process-alive",
    "note-missing-sample": "reference-process-exited-during-render",
    "note-sample-default-inst": "reference-process-exited-during-render",
    "note-sample-serialized-inst": "reference-process-exited-during-render",
}) == "explicit-instrument-or-sample-gate-associated-exit"

assert startup.diagnose({
    "note-no-previous-inst": "stable-finalized-output-process-alive",
    "note-missing-sample": "stable-finalized-output-process-alive",
    "note-sample-default-inst": "reference-process-exited-during-render",
    "note-sample-serialized-inst": "reference-process-exited-during-render",
}) == "enabled-sample-voice-startup-associated-exit"

assert startup.diagnose({
    "note-no-previous-inst": "stable-finalized-output-process-alive",
    "note-missing-sample": "stable-finalized-output-process-alive",
    "note-sample-default-inst": "stable-finalized-output-process-alive",
    "note-sample-serialized-inst": "reference-process-exited-during-render",
}) == "serialized-instrument-state-associated-exit"

assert startup.diagnose({
    "note-no-previous-inst": "stable-finalized-output-process-alive",
    "note-missing-sample": "stable-finalized-output-process-alive",
    "note-sample-default-inst": "stable-finalized-output-process-alive",
    "note-sample-serialized-inst": "stable-finalized-output-process-alive",
}) == "all-startup-gates-survive-known-crash-not-reproduced"

assert startup.diagnose({
    "note-no-previous-inst": "reference-process-exited-during-render",
    "note-missing-sample": "stable-finalized-output-process-alive",
    "note-sample-default-inst": "reference-process-exited-during-render",
    "note-sample-serialized-inst": "reference-process-exited-during-render",
}) == "nonmonotonic-startup-result"

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    name = "note-no-previous-inst"
    output_dir = root / ("sampler-voice-startup-" + name)
    output_dir.mkdir()
    filename = "original-sampler-voice-startup-note-no-previous-inst-1.wav"
    output = output_dir / filename
    data = wave_pcm16([0] * 64)
    output.write_bytes(data)
    attempt = {
        "process_exited": False,
        "process_exit_code": None,
        "dialog_closed": False,
        "stable_output_polls": 4,
        "observed_output": {
            "path": filename,
            "size_bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        },
    }
    stable = startup.validate_stable_alive_output(
        root, name, attempt, filename, module
    )
    assert stable["frame_count"] == 64
    assert stable["nonzero_frame_count"] == 0
    assert stable["observed_output"]["sha256"] == hashlib.sha256(data).hexdigest()

    bad = dict(attempt)
    bad["observed_output"] = dict(attempt["observed_output"])
    bad["observed_output"]["size_bytes"] += 1
    try:
        startup.validate_stable_alive_output(
            root, name, bad, filename, module
        )
    except ValueError as exc:
        assert "binding mismatch" in str(exc)
    else:
        raise AssertionError("expected startup stable-output binding failure")

valid_completed_attempt = {
    **valid_render_event_binding(),
    "outcome": "rendered",
    "process_exited": False,
    "process_exit_code": None,
    "dialog_closed": True,
    "close_control_seen": True,
    "close_uia_invoked": True,
    "stable_output_polls": 4,
    "diagnostics": [],
}
startup.validate_completed_render_attempt(
    valid_completed_attempt, "note-sample-default-inst"
)
post_exit_completed_attempt = {
    **valid_completed_attempt,
    "process_exited": True,
    "process_exit_code": -1073741819,
}
startup.validate_completed_render_attempt(
    post_exit_completed_attempt,
    "note-sample-default-inst",
    allow_post_completion_exit=True,
)
work_boundary.validate_completed_render_attempt(
    post_exit_completed_attempt,
    "release-no-active-voice",
    allow_post_completion_exit=True,
)
clean_exit_completed_attempt = {
    **valid_completed_attempt,
    "process_exited": True,
    "process_exit_code": 0,
}
startup.validate_completed_render_attempt(
    clean_exit_completed_attempt,
    "note-sample-default-inst",
    allow_post_completion_exit=True,
)
work_boundary.validate_completed_render_attempt(
    clean_exit_completed_attempt,
    "release-no-active-voice",
    allow_post_completion_exit=True,
)

teardown_only_completed_attempt = {
    "outcome": "rendered",
    "process_exited": False,
    "process_exit_code": None,
    "dialog_closed": False,
    "close_control_seen": True,
    "close_uia_invoked": True,
    "close_native_fallback_invoked": True,
    "close_wm_close_invoked": True,
    "stable_output_polls": 4,
    "render_dialog_native_event_hook_armed": True,
    "render_dialog_event_message_pump_started": True,
    "render_dialog_dispatch_boundary_set": True,
    "render_dialog_dispatch_boundary_tick": 123456,
    "render_dialog_post_dispatch_observed_window_event_count": 1,
    "render_dialog_unresolved_post_dispatch_event_count": 0,
    "render_dialog_post_dispatch_event_count": 1,
    "dialog_discovery": "pumped-win-event-object-show-strictly-after-dispatch-tick",
    "preexisting_render_dialog_count": 0,
    "selected_render_dialog_native_handle": 12345,
    "selected_render_dialog_runtime_id": [1, 2, 3],
    "diagnostics": [
        "render output finalized and Close control was verified, "
        "but dialog teardown did not complete"
    ],
}
startup.validate_completed_render_attempt(
    teardown_only_completed_attempt, "note-sample-default-inst"
)
work_boundary.validate_completed_render_attempt(
    teardown_only_completed_attempt, "release-no-active-voice"
)

closed_without_close_evidence = dict(valid_completed_attempt)
closed_without_close_evidence["close_control_seen"] = False
closed_without_close_evidence["close_uia_invoked"] = False
for check, name in (
    (startup.validate_completed_render_attempt, "note-sample-default-inst"),
    (work_boundary.validate_completed_render_attempt, "release-no-active-voice"),
):
    try:
        check(closed_without_close_evidence, name)
    except ValueError as exc:
        assert "terminal completion evidence" in str(exc)
    else:
        raise AssertionError(
            "expected clean-close receipt without verified Close evidence rejection"
        )

fresh_attempt = {
    **teardown_only_completed_attempt,
    "command_verified": True,
    "command_dispatched": True,
    "dialog_verified": True,
    "controls_configured": True,
    "save_invoked": True,
}
work_boundary.require_attempt_prefix(fresh_attempt, "release-no-active-voice")
startup.require_attempt_prefix(fresh_attempt, "note-sample-default-inst")
for altered in (
    {
        "render_dialog_native_event_hook_armed": False,
        "render_dialog_event_message_pump_started": False,
        "render_dialog_post_dispatch_observed_window_event_count": 0,
        "render_dialog_post_dispatch_event_count": 0,
        "render_dialog_unresolved_post_dispatch_event_count": 9,
    },
    {"render_dialog_post_dispatch_observed_window_event_count": 2},
    {"selected_render_dialog_native_handle": None},
):
    unbound = {**fresh_attempt, **altered}
    for check, name in (
        (work_boundary.require_attempt_prefix, "release-no-active-voice"),
        (work_boundary.validate_completed_render_attempt, "release-no-active-voice"),
        (startup.require_attempt_prefix, "note-sample-default-inst"),
        (startup.validate_completed_render_attempt, "note-sample-default-inst"),
    ):
        try:
            check(unbound, name)
        except ValueError as exc:
            assert "bound post-dispatch dialog evidence" in str(exc)
        else:
            raise AssertionError("expected unbound fresh render attempt rejection")

invalid_teardown_completed_attempt = dict(teardown_only_completed_attempt)
invalid_teardown_completed_attempt["close_control_seen"] = False
try:
    startup.validate_completed_render_attempt(
        invalid_teardown_completed_attempt, "note-sample-default-inst"
    )
except ValueError as exc:
    assert "terminal completion evidence" in str(exc)
else:
    raise AssertionError("expected unverified teardown-only completion rejection")

valid_exit_receipt = {
    "exit_code_before_termination": startup.EXPECTED_ACCESS_VIOLATION_EXIT_CODE,
}
valid_exit_attempt = {
    "process_exit_code": startup.EXPECTED_ACCESS_VIOLATION_EXIT_CODE,
    "diagnostics": ["reference exited during offline render"],
}
assert startup.validate_expected_access_violation(
    valid_exit_receipt,
    valid_exit_attempt,
    "note-sample-default-inst",
) == startup.EXPECTED_ACCESS_VIOLATION_EXIT_CODE

bad_exit_receipt = {"exit_code_before_termination": 1}
bad_exit_attempt = {
    "process_exit_code": 1,
    "diagnostics": ["reference exited during offline render"],
}
try:
    startup.validate_expected_access_violation(
        bad_exit_receipt,
        bad_exit_attempt,
        "note-sample-default-inst",
    )
except ValueError as exc:
    assert "0xC0000005" in str(exc)
else:
    raise AssertionError("expected unrelated nonzero exit-code rejection")

invalid_completed_attempt = dict(valid_completed_attempt)
invalid_completed_attempt.update(
    {
        "dialog_closed": False,
        "stable_output_polls": 0,
        "diagnostics": ["completion was never observed"],
    }
)
try:
    startup.validate_completed_render_attempt(
        invalid_completed_attempt, "note-sample-default-inst"
    )
except ValueError as exc:
    assert "terminal completion evidence" in str(exc)
else:
    raise AssertionError("expected incomplete rendered-attempt rejection")

with tempfile.TemporaryDirectory() as candidate_temp, tempfile.TemporaryDirectory() as original_temp:
    candidate_root = Path(candidate_temp)
    original_root = Path(original_temp)
    source_receipt = startup.expected_source_receipt()
    source_path = candidate_root / startup.SOURCE_RECEIPT
    source_path.write_text(
        json.dumps(source_receipt, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    relative = startup.materialize_source_receipt(
        candidate_root, original_root
    )
    assert relative == startup.SOURCE_RECEIPT
    original_source = original_root / relative
    assert original_source.is_file()
    assert original_source.read_bytes() == source_path.read_bytes()
    assert startup.validate_source(original_root) == source_receipt

    corrupted = startup.expected_source_receipt()
    corrupted["files"]["Sampler.cpp"]["markers"] = []
    corrupted["files"]["Song.cpp"]["markers"] = []
    corrupted["source_boundary"] = [
        "enabled samples return before voice selection"
    ]
    original_source.write_text(
        json.dumps(corrupted, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    try:
        startup.validate_source(original_root)
    except ValueError as exc:
        assert "canonical source semantics" in str(exc)
    else:
        raise AssertionError("expected corrupted source receipt rejection")

assert work_boundary.diagnose({
    "release-no-active-voice": "reference-process-exited-during-render",
    "delayed-note-short": "reference-process-exited-during-render",
    "delayed-note-long": "reference-process-exited-during-render",
    "ordinary-note-short": "reference-process-exited-during-render",
}) == "post-sample-gate-pre-voice-tick-associated-exit"

assert work_boundary.diagnose({
    "release-no-active-voice": "stable-finalized-output-process-alive",
    "delayed-note-short": "reference-process-exited-during-render",
    "delayed-note-long": "reference-process-exited-during-render",
    "ordinary-note-short": "reference-process-exited-during-render",
}) == "enabled-note-startup-pre-controller-work-associated-exit"

assert work_boundary.HISTORICAL_DIAGNOSIS == "voice-tick-initialization-associated-exit"
assert work_boundary.QUALIFIED_DIAGNOSIS == (
    "enabled-note-startup-pre-controller-work-associated-exit"
)
projection = json.loads(work_boundary.PROJECTION.read_text(encoding="utf-8"))
assert projection["historical_diagnosis"] == work_boundary.HISTORICAL_DIAGNOSIS
assert projection["qualified_diagnosis"] == work_boundary.QUALIFIED_DIAGNOSIS
assert projection["qualified_interpretation"]["voice_work_entry"] == "unresolved"
assert projection["qualified_interpretation"]["voice_tick_fault_location"] == "unresolved"

frozen_projection = work_boundary.validate_frozen_projection()
assert frozen_projection["validation"] == "durable-repository-manifest"
assert frozen_projection["workflow_run_id"] == work_boundary.FROZEN_PROJECTION_RUN_ID

with tempfile.TemporaryDirectory() as manifest_temp:
    corrupted_manifest_path = Path(manifest_temp) / "historical-manifest.json"
    corrupted_manifest = work_boundary.expected_frozen_manifest()
    corrupted_manifest["fixtures"]["delayed-note-short"][
        "original_receipt_sha256"
    ] = "0" * 64
    corrupted_manifest_path.write_text(
        json.dumps(corrupted_manifest, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    saved_manifest = work_boundary.FROZEN_MANIFEST
    work_boundary.FROZEN_MANIFEST = corrupted_manifest_path
    try:
        try:
            work_boundary.validate_frozen_projection()
        except ValueError as exc:
            assert "durable historical manifest mismatch" in str(exc)
        else:
            raise AssertionError("expected corrupted durable manifest rejection")
    finally:
        work_boundary.FROZEN_MANIFEST = saved_manifest

with tempfile.TemporaryDirectory() as semantic_temp:
    semantic_root = Path(semantic_temp)
    saved_projection = work_boundary.PROJECTION
    try:
        for field, value in (
            ("parity_status", "PASS"),
            ("historical_diagnosis", "corrupted-diagnosis"),
        ):
            corrupted_projection = json.loads(
                saved_projection.read_text(encoding="utf-8")
            )
            corrupted_projection[field] = value
            corrupted_path = semantic_root / f"{field}.json"
            corrupted_path.write_text(
                json.dumps(corrupted_projection, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            work_boundary.PROJECTION = corrupted_path
            try:
                work_boundary.validate_frozen_projection()
            except ValueError as exc:
                assert "projection identity mismatch" in str(exc)
            else:
                raise AssertionError(
                    f"expected frozen projection {field} corruption rejection"
                )
    finally:
        work_boundary.PROJECTION = saved_projection

with tempfile.TemporaryDirectory() as projection_temp:
    projection_root = Path(projection_temp)
    candidate_root = projection_root / "candidate"
    original_root = projection_root / "original"
    projection_path = (
        projection_root
        / "phase6c"
        / "evidence"
        / "sequencer-sampler-work-boundary"
        / "observation.json"
    )
    candidate_root.mkdir()
    original_root.mkdir()
    projection_path.parent.mkdir(parents=True)

    fixtures = {}
    results = {}
    original_bytes = {}
    for name in work_boundary.VARIANTS:
        fixture_sha256 = hashlib.sha256(name.encode("utf-8")).hexdigest()
        candidate_path = candidate_root / work_boundary.candidate_receipt_name(name)
        candidate_path.write_text(
            json.dumps({"fixture_sha256": fixture_sha256}, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        original_path = original_root / work_boundary.original_receipt_name(name)
        raw_original = (
            json.dumps({"variant": name, "source": "synthetic-original"}, sort_keys=True)
            + "\n"
        ).encode("utf-8")
        original_path.write_bytes(raw_original)
        original_bytes[name] = raw_original

        result = {
            "outcome": "reference-process-exited-during-render",
            "process_exit_code": work_boundary.EXPECTED_ACCESS_VIOLATION_EXIT_CODE,
            "observed_output": None,
        }
        results[name] = result
        fixtures[name] = {
            "fixture_sha256": fixture_sha256,
            "candidate_receipt_sha256": work_boundary.digest(
                candidate_path.read_bytes()
            ),
            "original_receipt_sha256": work_boundary.digest(raw_original),
            **result,
        }

    synthetic_projection = {
        "schema_version": 1,
        "phase": "6C",
        "contract": work_boundary.CONTRACT,
        "parity_status": "UNKNOWN",
        "historical_diagnosis": work_boundary.HISTORICAL_DIAGNOSIS,
        "qualified_diagnosis": work_boundary.QUALIFIED_DIAGNOSIS,
        "evidence_run": {},
        "historical_receipt": {},
        "source_identity": {
            "source_commit": work_boundary.SOURCE_COMMIT,
            "Sampler.cpp": work_boundary.SAMPLER_CPP_BLOB,
            "Sampler.hpp": work_boundary.SAMPLER_HPP_BLOB,
            "SongStructs.hpp": work_boundary.SONG_STRUCTS_BLOB,
        },
        "fixtures": fixtures,
        "qualified_interpretation": {
            "controller_work_reachable": False,
            "voice_work_entry": "unresolved",
            "voice_tick_fault_location": "unresolved",
            "voice_selection_fault_location": "unresolved",
        },
    }
    projection_path.write_text(
        json.dumps(synthetic_projection, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    summary = {"results": results}

    saved_projection = work_boundary.PROJECTION
    saved_root = work_boundary.ROOT
    work_boundary.PROJECTION = projection_path
    work_boundary.ROOT = projection_root
    try:
        assert work_boundary.validate_projection(
            candidate_root, original_root, summary
        ) == "phase6c/evidence/sequencer-sampler-work-boundary/observation.json"

        missing_name = "ordinary-note-short"
        missing_path = (
            original_root / work_boundary.original_receipt_name(missing_name)
        )
        missing_path.unlink()
        try:
            work_boundary.validate_projection(candidate_root, original_root, summary)
        except ValueError as exc:
            assert "original receipt missing" in str(exc)
            assert missing_name in str(exc)
        else:
            raise AssertionError("expected missing original receipt rejection")

        missing_path.write_bytes(original_bytes[missing_name])
        modified_name = "delayed-note-short"
        modified_path = (
            original_root / work_boundary.original_receipt_name(modified_name)
        )
        modified_path.write_bytes(original_bytes[modified_name] + b" ")
        try:
            work_boundary.validate_projection(candidate_root, original_root, summary)
        except ValueError as exc:
            assert "projection mismatch" in str(exc)
            assert modified_name in str(exc)
        else:
            raise AssertionError("expected modified original receipt rejection")
    finally:
        work_boundary.PROJECTION = saved_projection
        work_boundary.ROOT = saved_root

with tempfile.TemporaryDirectory() as rerun_candidate_temp, tempfile.TemporaryDirectory() as rerun_original_temp:
    projection_path, projection_mode = work_boundary.projection_for_run(
        Path(rerun_candidate_temp),
        Path(rerun_original_temp),
        {},
        False,
    )
    assert projection_path == (
        "phase6c/evidence/sequencer-sampler-work-boundary/observation.json"
    )
    assert projection_mode == "fresh-rerun-semantic-only"

assert work_boundary.diagnose({
    "release-no-active-voice": "stable-finalized-output-process-alive",
    "delayed-note-short": "stable-finalized-output-process-alive",
    "delayed-note-long": "reference-process-exited-during-render",
    "ordinary-note-short": "reference-process-exited-during-render",
}) == "controller-work-associated-exit"

assert work_boundary.diagnose({
    "release-no-active-voice": "stable-finalized-output-process-alive",
    "delayed-note-short": "stable-finalized-output-process-alive",
    "delayed-note-long": "stable-finalized-output-process-alive",
    "ordinary-note-short": "reference-process-exited-during-render",
}) == "immediate-undelayed-work-associated-exit"

assert work_boundary.diagnose({
    "release-no-active-voice": "stable-finalized-output-process-alive",
    "delayed-note-short": "stable-finalized-output-process-alive",
    "delayed-note-long": "reference-process-exited-during-render",
    "ordinary-note-short": "stable-finalized-output-process-alive",
}) == "delayed-work-transition-associated-exit"

assert work_boundary.diagnose({
    "release-no-active-voice": "stable-finalized-output-process-alive",
    "delayed-note-short": "stable-finalized-output-process-alive",
    "delayed-note-long": "stable-finalized-output-process-alive",
    "ordinary-note-short": "stable-finalized-output-process-alive",
}) == "known-enabled-sample-crash-not-reproduced"

fresh_all_live_outcomes = {
    "release-no-active-voice": "stable-finalized-output-process-alive",
    "delayed-note-short": "stable-finalized-output-process-alive",
    "delayed-note-long": "stable-finalized-output-process-alive",
    "ordinary-note-short": "stable-finalized-output-process-alive",
}
fresh_all_live_diagnosis = work_boundary.diagnose(fresh_all_live_outcomes)
fresh_all_live_interpretation = work_boundary.interpretation_for_outcomes(
    fresh_all_live_outcomes, fresh_all_live_diagnosis
)
assert "known-enabled-sample-crash-not-reproduced" in fresh_all_live_interpretation
assert "current work-boundary receipt set" in fresh_all_live_interpretation
assert "pinned original access violation is associated" not in fresh_all_live_interpretation

with tempfile.TemporaryDirectory() as candidate_temp, tempfile.TemporaryDirectory() as original_temp:
    candidate_root = Path(candidate_temp)
    original_root = Path(original_temp)
    source_receipt = work_boundary.expected_source_receipt()
    source_path = candidate_root / work_boundary.SOURCE_RECEIPT
    source_path.write_text(
        json.dumps(source_receipt, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    relative = work_boundary.materialize_source_receipt(
        candidate_root, original_root
    )
    assert relative == work_boundary.SOURCE_RECEIPT
    assert work_boundary.validate_source(original_root) == source_receipt

    corrupted = work_boundary.expected_source_receipt()
    corrupted["files"]["Sampler.cpp"]["markers"] = []
    corrupted["source_boundary"] = [
        "Voice::Work always reaches controller.Work before delay checks"
    ]
    (original_root / work_boundary.SOURCE_RECEIPT).write_text(
        json.dumps(corrupted, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    try:
        work_boundary.validate_source(original_root)
    except ValueError as exc:
        assert "canonical semantics" in str(exc)
    else:
        raise AssertionError("expected work-boundary source semantic rejection")

fresh_ambiguous_attempt = {
    "outcome": "inconclusive",
    "command_verified": True,
    "command_dispatched": True,
    "dialog_verified": True,
    "controls_configured": True,
    "save_invoked": True,
    "render_dialog_native_event_hook_armed": True,
    "render_dialog_event_message_pump_started": True,
    "render_dialog_dispatch_boundary_set": True,
    "render_dialog_dispatch_boundary_tick": 1234,
    "dialog_discovery": "pumped-win-event-object-show-strictly-after-dispatch-tick",
    "preexisting_render_dialog_count": 0,
    "render_dialog_post_dispatch_observed_window_event_count": 2,
    "render_dialog_unresolved_post_dispatch_event_count": 0,
    "render_dialog_post_dispatch_event_count": 1,
    "selected_render_dialog_native_handle": 12345,
    "selected_render_dialog_runtime_id": [42, 12345],
    "output": None,
    "diagnostics": ["multiple post-dispatch Psycle window-show events observed"],
}
binding_error = work_boundary.validate_fresh_render_event_binding_or_quarantine(
    fresh_ambiguous_attempt,
    "release-no-active-voice",
    "inconclusive",
    [],
)
assert binding_error is not None
assert "fresh render lacks bound post-dispatch dialog evidence" in binding_error

startup.require_attempt_dispatch_prefix(
    fresh_ambiguous_attempt,
    "note-no-previous-inst",
)
startup_binding_error = (
    startup.validate_fresh_render_event_binding_or_quarantine(
        fresh_ambiguous_attempt,
        "note-no-previous-inst",
        "inconclusive",
        [],
    )
)
assert startup_binding_error is not None
assert "fresh render lacks bound post-dispatch dialog evidence" in startup_binding_error

early_fresh_ambiguous_attempt = dict(fresh_ambiguous_attempt)
early_fresh_ambiguous_attempt.update(
    {
        "dialog_verified": False,
        "controls_configured": False,
        "save_invoked": False,
        "selected_render_dialog_native_handle": None,
        "selected_render_dialog_runtime_id": [],
        "dialog_discovery": None,
    }
)
work_boundary.require_attempt_dispatch_prefix(
    early_fresh_ambiguous_attempt,
    "release-no-active-voice",
)
early_binding_error = (
    work_boundary.validate_fresh_render_event_binding_or_quarantine(
        early_fresh_ambiguous_attempt,
        "release-no-active-voice",
        "inconclusive",
        [],
    )
)
assert early_binding_error is not None
assert "fresh render lacks bound post-dispatch dialog evidence" in early_binding_error

startup.require_attempt_dispatch_prefix(
    early_fresh_ambiguous_attempt,
    "note-no-previous-inst",
)
early_startup_binding_error = (
    startup.validate_fresh_render_event_binding_or_quarantine(
        early_fresh_ambiguous_attempt,
        "note-no-previous-inst",
        "inconclusive",
        [],
    )
)
assert early_startup_binding_error is not None
assert "fresh render lacks bound post-dispatch dialog evidence" in early_startup_binding_error

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)

    startup_name = "note-sample-default-inst"
    startup_filename = (
        "original-sampler-voice-startup-note-sample-default-inst-1.wav"
    )
    startup_dir = root / f"sampler-voice-startup-{startup_name}"
    startup_dir.mkdir()
    startup_bytes = b"voice-startup-partial"
    (startup_dir / startup_filename).write_bytes(startup_bytes)
    startup_attempt = dict(fresh_ambiguous_attempt)
    startup_attempt["observed_output"] = {
        "path": startup_filename,
        "size_bytes": len(startup_bytes),
        "sha256": hashlib.sha256(startup_bytes).hexdigest(),
    }
    startup_observed = startup.validate_observed_output(
        root, startup_name, startup_attempt, startup_filename
    )
    assert startup_observed["sha256"] == hashlib.sha256(startup_bytes).hexdigest()

    work_name = "release-no-active-voice"
    work_filename = "original-sampler-work-boundary-release-no-active-voice-1.wav"
    work_dir = root / f"sampler-work-boundary-{work_name}"
    work_dir.mkdir()
    work_bytes = b"work-boundary-partial"
    (work_dir / work_filename).write_bytes(work_bytes)
    work_attempt = dict(fresh_ambiguous_attempt)
    work_attempt["observed_output"] = {
        "path": work_filename,
        "size_bytes": len(work_bytes),
        "sha256": hashlib.sha256(work_bytes).hexdigest(),
    }
    work_observed = work_boundary.validate_observed_output(
        root, work_name, work_attempt, work_filename
    )
    assert work_observed["sha256"] == hashlib.sha256(work_bytes).hexdigest()

assert work_boundary.diagnose({
    "release-no-active-voice": "inconclusive",
    "delayed-note-short": "reference-process-exited-during-render",
    "delayed-note-long": "reference-process-exited-during-render",
    "ordinary-note-short": "reference-process-exited-during-render",
}) == "inconclusive"

promoted_ambiguous_attempt = dict(fresh_ambiguous_attempt)
promoted_ambiguous_attempt["outcome"] = "rendered"
try:
    work_boundary.validate_fresh_render_event_binding_or_quarantine(
        promoted_ambiguous_attempt,
        "release-no-active-voice",
        "rendered-once",
        [{"path": "x.wav", "sha256": "0" * 64}],
    )
except ValueError as exc:
    assert "fresh render lacks bound post-dispatch dialog evidence" in str(exc)
else:
    raise AssertionError("expected promoted ambiguous render evidence rejection")

bad_work_exit_receipt = {"exit_code_before_termination": 1}
bad_work_exit_attempt = {
    "process_exit_code": 1,
    "diagnostics": ["reference exited during offline render"],
}
try:
    work_boundary.validate_expected_access_violation(
        bad_work_exit_receipt,
        bad_work_exit_attempt,
        "ordinary-note-short",
    )
except ValueError as exc:
    assert "0xC0000005" in str(exc)
else:
    raise AssertionError("expected unrelated work-boundary exit rejection")

print("phase6c-delayed-retrigger-render: PASS")
