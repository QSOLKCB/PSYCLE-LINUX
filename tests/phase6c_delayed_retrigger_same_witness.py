#!/usr/bin/env python3
"""Negative controls for the same-witness Sampulse runtime evidence."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import struct
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "phase6c-delayed-retrigger-same-witness.py"
spec = importlib.util.spec_from_file_location("same_witness", SCRIPT)
assert spec is not None and spec.loader is not None
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def pcm_wave(onsets: list[int], frames: int = 90000) -> bytes:
    values = [0] * frames
    for onset in onsets:
        for offset in range(4):
            values[onset + offset] = 12000
    payload = struct.pack("<" + "h" * len(values), *values)
    fmt = struct.pack("<HHIIHH", 1, 1, 44100, 88200, 2, 16)
    riff_size = 4 + (8 + len(fmt)) + (8 + len(payload))
    return (
        b"RIFF"
        + struct.pack("<I", riff_size)
        + b"WAVE"
        + b"fmt "
        + struct.pack("<I", len(fmt))
        + fmt
        + b"data"
        + struct.pack("<I", len(payload))
        + payload
    )


def expect_value_error(fn, phrase: str) -> None:
    try:
        fn()
    except ValueError as exc:
        assert phrase in str(exc), exc
    else:
        raise AssertionError("expected ValueError")


beat = 44100.0 * 60.0 / 137.0
onsets = [
    int(round(0.0625 * beat)),
    int(round(1.0 * beat)),
    int(round(1.0625 * beat)),
    int(round(1.125 * beat)),
    int(round(2.0 * beat)),
    int(round(2.0625 * beat)),
    int(round(3.125 * beat)),
]
valid_wave = pcm_wave(onsets)


def write_provenance(root: Path) -> None:
    runtime = root / m.NAME
    runtime.mkdir(exist_ok=True)
    for name in (
        "phase6c-delayed-retrigger-sampulse-render",
        "render-probe.cpp",
        "render-probe.pro",
        "fixture-generator.c",
        "eins-compat.py",
    ):
        (runtime / name).write_bytes(("fixture:" + name).encode())
    delayed = root / "delayed-retrigger"
    delayed.mkdir(exist_ok=True)
    (delayed / "sampulse-eins-compat.log").write_text("EINS PASS\n")
    for index in (1, 2):
        (delayed / f"sampulse-candidate-render-{index}.log").write_text(
            f"render {index} PASS\n"
        )


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(b"PSY3SONG" + b"same-witness-sampulse")

    render_dir = root / m.NAME
    write_provenance(root)
    for index in (1, 2):
        (render_dir / f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav").write_bytes(
            valid_wave
        )

    collected = m.collect_candidate(root)
    assert collected["runtime_command_execution_observed"] is True
    assert collected["timing_interpretation"] == "deferred"
    assert collected["analysis"]["window_onset_counts"]["retrigger_beat_1"] == 3
    assert collected["analysis"]["window_onset_counts"]["retr_cont_beat_2"] == 2
    assert m.validate_candidate(root)["parity_status"] == "UNKNOWN"

    receipt_path = root / m.CANDIDATE_RECEIPT
    receipt = json.loads(receipt_path.read_text())
    receipt["timing_interpretation"] = "PASS"
    receipt_path.write_text(json.dumps(receipt) + "\n")
    expect_value_error(
        lambda: m.validate_candidate(root),
        "candidate receipt identity mismatch",
    )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(b"PSY3SONG" + b"same-witness-sampulse")
    render_dir = root / m.NAME
    write_provenance(root)
    (render_dir / "candidate-delayed-retrigger-sampulse-runtime-1.wav").write_bytes(
        valid_wave
    )
    modified = bytearray(valid_wave)
    modified[-2:] = struct.pack("<h", 1)
    (render_dir / "candidate-delayed-retrigger-sampulse-runtime-2.wav").write_bytes(
        bytes(modified)
    )
    expect_value_error(
        lambda: m.collect_candidate(root),
        "not byte-identical",
    )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    render_dir = root / m.NAME
    render_dir.mkdir(parents=True)
    original_path = render_dir / "original-delayed-retrigger-sampulse-runtime-1.wav"
    original_path.write_bytes(valid_wave)

    completed_attempt = {
        "command_verified": True,
        "command_dispatched": True,
        "dialog_verified": True,
        "controls_configured": True,
        "save_invoked": True,
        "outcome": "rendered",
        "process_exited": False,
        "process_exit_code": None,
        "dialog_closed": False,
        "close_control_seen": True,
        "close_uia_invoked": True,
        "stable_output_polls": 4,
        "diagnostics": [
            "render output finalized and Close control was verified, "
            "but dialog teardown did not complete"
        ],
        "output": {
            "path": original_path.name,
            "sha256": m.digest(valid_wave),
        },
    }
    binding, data = m.validate_original_attempt(root, completed_attempt, 1)
    assert data == valid_wave
    assert binding["sha256"] == m.digest(valid_wave)

    incomplete_attempt = dict(completed_attempt)
    incomplete_attempt["stable_output_polls"] = 3
    expect_value_error(
        lambda: m.validate_original_attempt(root, incomplete_attempt, 1),
        "did not complete cleanly",
    )

valid_original_runtime = {
    "schema_version": 1,
    "outcome": "rendered-twice",
    "deterministic": True,
    "settings": dict(m.ORIGINAL_RENDER_SETTINGS),
    "pre_render_load": {
        "schema_version": 1,
        "clean_accepted_load": True,
        "stable_marker_polls": 4,
        "matched_marker": Path(m.FIXTURE).name,
        "load_warning_dismissed": True,
        "process_running_before_render": True,
    },
    "renders": [],
    "attempts": [{}, {}],
}
assert len(m.validate_original_runtime_procedure(valid_original_runtime)) == 2

missing_pre_render_load = dict(valid_original_runtime)
missing_pre_render_load.pop("pre_render_load")
expect_value_error(
    lambda: m.validate_original_runtime_procedure(missing_pre_render_load),
    "render procedure mismatch",
)

wrong_dither_runtime = dict(valid_original_runtime)
wrong_dither_runtime["settings"] = {
    **m.ORIGINAL_RENDER_SETTINGS,
    "dither": True,
}
expect_value_error(
    lambda: m.validate_original_runtime_procedure(wrong_dither_runtime),
    "render procedure mismatch",
)

wrong_range_runtime = dict(valid_original_runtime)
wrong_range_runtime["settings"] = {
    **m.ORIGINAL_RENDER_SETTINGS,
    "range": "selection",
}
expect_value_error(
    lambda: m.validate_original_runtime_procedure(wrong_range_runtime),
    "render procedure mismatch",
)

print("phase6c-delayed-retrigger-same-witness: PASS")
