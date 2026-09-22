#!/usr/bin/env python3
"""Negative controls for the Phase 6C offline-render execution analyzer."""
from __future__ import annotations

import hashlib
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

print("phase6c-delayed-retrigger-render: PASS")
