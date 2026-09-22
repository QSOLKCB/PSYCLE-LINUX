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

verified_exit_attempt = {
    "outcome": "inconclusive",
    "command_verified": True,
    "command_dispatched": True,
    "dialog_verified": True,
    "controls_configured": True,
    "save_invoked": True,
}
module.validate_render_attempt(verified_exit_attempt, "inconclusive")

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
        "outcome": "rendered",
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": True,
        "save_invoked": True,
        "process_exited": False,
        "process_exit_code": None,
        "output": {"path": first_name, "sha256": first_hash},
    }

    second_name = "original-delayed-retrigger-execution-2.wav"
    second_path = output_dir / second_name
    second_path.write_bytes(b"")
    second_hash = hashlib.sha256(b"").hexdigest()
    second_attempt = {
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

render_helper_source = ORIGINAL_AUDIO_RENDER_SCRIPT.read_text(encoding="utf-8")
assert "$preexistingRenderDialogHandles" not in render_helper_source
assert "Test-Phase6cSameAutomationElement" not in render_helper_source
assert "[System.Windows.Automation.Automation]::Compare" not in render_helper_source
assert "Automation.AddAutomationEventHandler" not in render_helper_source
assert "WindowPattern.WindowOpenedEvent" not in render_helper_source
assert "SetWinEventHook" in render_helper_source
assert "EVENT_OBJECT_SHOW" in render_helper_source
assert "eventTime" in render_helper_source
assert "GetTickCount" in render_helper_source
assert "TickAtOrAfter(eventTime, dispatchBoundaryTick)" in render_helper_source
assert "Phase6cRenderWindowOpenedObserver" in render_helper_source
assert "BeginDispatchBoundary" in render_helper_source
assert "CancelDispatchBoundary" in render_helper_source
assert "render_dialog_native_event_hook_armed" in render_helper_source
assert "render_dialog_dispatch_boundary_set" in render_helper_source
assert "render_dialog_dispatch_boundary_tick" in render_helper_source
assert "render_dialog_post_dispatch_event_count" in render_helper_source
assert "catch [System.Windows.Automation.ElementNotAvailableException]" in render_helper_source
assert "selected_render_dialog_native_handle" in render_helper_source
assert "selected_render_dialog_runtime_id" in render_helper_source
assert (
    "win-event-object-show-after-dispatch-tick-boundary"
    in render_helper_source
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
assert "$dialogObserver.Seal()" in render_helper_source
assert (
    'render_dialog_post_dispatch_event_count -ne 1'
    in render_helper_source
)

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
    "outcome": "rendered",
    "process_exited": False,
    "process_exit_code": None,
    "dialog_closed": True,
    "stable_output_polls": 4,
    "diagnostics": [],
}
startup.validate_completed_render_attempt(
    valid_completed_attempt, "note-sample-default-inst"
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
