#!/usr/bin/env python3
"""Negative controls for the same-witness Sampulse runtime evidence."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import struct
import subprocess
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



def synthetic_eins_payload() -> bytes:
    sample_body = bytearray()
    sample_body += b"Phase 6C deterministic impulse\0"
    sample_body += struct.pack("<I", 512)
    sample_body += struct.pack("<f", 1.0)
    sample_body += struct.pack("<H", 128)
    sample_body += struct.pack("<III", 0, 0, 0)
    sample_body += struct.pack("<III", 0, 0, 0)
    sample_body += struct.pack("<I", 44100)
    sample_body += struct.pack("<hh", 0, 0)
    sample_body += struct.pack("<?", False)
    sample_body += struct.pack("<?", False)
    sample_body += struct.pack("<f", 0.5)
    sample_body += struct.pack("<?", False)
    sample_body += struct.pack("<BBBB", 0, 0, 0, 0)
    compressed = bytes(range(1, 17))
    sample_body += struct.pack("<I", len(compressed))
    sample_body += compressed
    sample = (
        b"SMPD"
        + struct.pack("<I", 12 + len(sample_body))
        + struct.pack("<I", 1)
        + bytes(sample_body)
    )
    return (
        struct.pack("<I", 1)
        + struct.pack("<i", 0)
        + m.eins_converter_module.historical_instrument()
        + struct.pack("<I", 1)
        + struct.pack("<i", 0)
        + sample
    )


def candidate_fixture_bytes(eins_payload: bytes | None = None) -> bytes:
    def chunk(fourcc: bytes, version: int, payload: bytes) -> bytes:
        return fourcc + struct.pack("<II", version, len(payload)) + payload

    info = m.TITLE.encode("utf-8") + b"\0"
    sngi = struct.pack("<iii", 16, 137, 8)
    patd = bytearray()
    patd += struct.pack("<iii", 0, 32, 16)
    patd += b"Execution Witness\0"
    patd += struct.pack("<I", 0)
    patd += struct.pack("<IIiii", 0, 0, 480, 1920, len(m.EXPECTED_FIXTURE_EVENTS))
    for track, offset, note, inst, mach, volume, command, parameter in m.EXPECTED_FIXTURE_EVENTS:
        patd += struct.pack("<iii", track, offset, 1)
        patd += struct.pack(
            "<iiiiii", note, inst, mach, volume, command, parameter
        )
    if eins_payload is None:
        eins_payload = synthetic_eins_payload()
    chunks = [
        chunk(b"INFO", 0, info),
        chunk(b"SNGI", 4, sngi),
        chunk(b"PATD", 2, bytes(patd)),
        chunk(b"MACD", 3, struct.pack("<ii", 0, 12)),
        chunk(b"EINS", 0x00010000, eins_payload),
    ]
    return (
        b"PSY3SONG"
        + struct.pack("<I", 0x11)
        + struct.pack("<I", 4)
        + struct.pack("<I", len(chunks))
        + b"".join(chunks)
    )


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
valid_wave = pcm_wave(onsets, frames=m.CANDIDATE_TARGET_FRAMES)
valid_fixture = candidate_fixture_bytes()


def candidate_render_summary() -> dict:
    compiled = m.expected_compiled_provenance()
    return {
        "schema_version": 1,
        "fixed_frame_render": True,
        "sample_rate": 44100,
        "channels": 1,
        "bits_per_sample": 16,
        "target_beats": m.CANDIDATE_TARGET_BEATS,
        "target_frames": m.CANDIDATE_TARGET_FRAMES,
        "threads": 1,
        "sequencer_work_calls": 1,
        "player_work_direct": False,
        "renderer_source_sha256": m.reviewed_digest(m.REVIEWED_RENDERER_SOURCE),
        "renderer_project_sha256": m.reviewed_digest(m.REVIEWED_RENDERER_PROJECT),
        "engine_sequencer_sha256": compiled["engine_sequencer_sha256"],
        "engine_psy3_loader_sha256": compiled["engine_psy3_loader_sha256"],
        "engine_xmsampler_sha256": compiled["engine_xmsampler_sha256"],
        "master_buffer_float_count": 2 * m.CANDIDATE_TARGET_FRAMES,
        "final_play_beat": m.CANDIDATE_TARGET_FRAMES / m.CANDIDATE_BEAT_FRAMES,
    }


def write_provenance(root: Path) -> None:
    runtime = root / m.NAME
    runtime.mkdir(exist_ok=True)
    binary = runtime / "phase6c-delayed-retrigger-sampulse-render"
    provenance_json = json.dumps(
        m.expected_compiled_provenance(), sort_keys=True, separators=(",", ":")
    )
    escaped = provenance_json.replace("\\", "\\\\").replace('"', '\\"')
    stub = runtime / "phase6c-provenance-stub.c"
    stub.write_text(
        "#include <stdio.h>\n"
        "#include <string.h>\n"
        "int main(int argc, char **argv) {\n"
        "  if (argc == 2 && strcmp(argv[1], \"--phase6c-provenance\") == 0) {\n"
        f'    puts("{escaped}");\n'
        "    return 0;\n"
        "  }\n"
        "  return 64;\n"
        "}\n",
        encoding="utf-8",
    )
    subprocess.run(["cc", str(stub), "-O2", "-o", str(binary)], check=True)
    stub.unlink()
    (runtime / "render-probe.cpp").write_bytes(
        m.REVIEWED_RENDERER_SOURCE.read_bytes()
    )
    (runtime / "render-probe.pro").write_bytes(
        m.REVIEWED_RENDERER_PROJECT.read_bytes()
    )
    (runtime / "fixture-generator.c").write_bytes(
        m.REVIEWED_FIXTURE_GENERATOR.read_bytes()
    )
    (runtime / "eins-compat.py").write_bytes(
        m.REVIEWED_EINS_CONVERTER.read_bytes()
    )
    (runtime / "renderer-build-provenance.hpp").write_bytes(
        m.expected_build_header()
    )

    delayed = root / "delayed-retrigger"
    delayed.mkdir(exist_ok=True)
    qmake_log = delayed / "sampulse-render-qmake.log"
    build_log = delayed / "sampulse-render-build.log"
    qmake_log.write_text("Project MESSAGE: phase6c Sampulse render\n")
    build_log.write_text(
        "g++ tests/phase6c_delayed_retrigger_sampulse_render.cpp "
        "-o phase6c-delayed-retrigger-sampulse-render\n"
    )
    provenance_log = delayed / "sampulse-render-provenance.log"
    provenance_log.write_bytes(
        subprocess.check_output([str(binary), "--phase6c-provenance"])
    )
    attestation = {
        "schema_version": 2,
        "reviewed_inputs": {
            "renderer_source": {
                "path": "tests/phase6c_delayed_retrigger_sampulse_render.cpp",
                "sha256": m.reviewed_digest(m.REVIEWED_RENDERER_SOURCE),
            },
            "renderer_project": {
                "path": "tests/phase6c_delayed_retrigger_sampulse_render.pro",
                "sha256": m.reviewed_digest(m.REVIEWED_RENDERER_PROJECT),
            },
            "fixture_generator": {
                "path": "tests/phase6c_delayed_retrigger_sampulse_execution_fixture.c",
                "sha256": m.reviewed_digest(m.REVIEWED_FIXTURE_GENERATOR),
            },
            "eins_converter": {
                "path": "scripts/phase6c-sampulse-eins-compat.py",
                "sha256": m.reviewed_digest(m.REVIEWED_EINS_CONVERTER),
            },
        },
        "engine_anchors": m.reviewed_engine_bindings(),
        "binary": {
            "path": f"{m.NAME}/phase6c-delayed-retrigger-sampulse-render",
            "sha256": m.digest(binary.read_bytes()),
        },
        "qmake_log": {
            "path": "delayed-retrigger/sampulse-render-qmake.log",
            "sha256": m.digest(qmake_log.read_bytes()),
        },
        "build_log": {
            "path": "delayed-retrigger/sampulse-render-build.log",
            "sha256": m.digest(build_log.read_bytes()),
        },
        "provenance_challenge_log": {
            "path": "delayed-retrigger/sampulse-render-provenance.log",
            "sha256": m.digest(provenance_log.read_bytes()),
        },
    }
    (runtime / "renderer-build-provenance.json").write_text(
        json.dumps(attestation, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (delayed / "sampulse-eins-compat.log").write_text("EINS PASS\n")
    for index in (1, 2):
        (delayed / f"sampulse-candidate-render-{index}.log").write_text(
            json.dumps(candidate_render_summary(), sort_keys=True) + "\n",
            encoding="utf-8",
        )


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(valid_fixture)

    render_dir = root / m.NAME
    write_provenance(root)
    for index in (1, 2):
        (render_dir / f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav").write_bytes(
            valid_wave
        )

    collected = m.collect_candidate(root)
    assert collected["runtime_command_execution_observed"] is True
    assert len(collected["render_observations"]) == 2
    assert collected["render_observations"][0]["threads"] == 1
    assert collected["timing_interpretation"] == "deferred"
    assert collected["analysis"]["window_onset_counts"]["retrigger_beat_1"] == 3
    assert collected["analysis"]["window_onset_counts"]["retr_cont_beat_2"] == 2
    assert collected["analysis"]["window_onset_counts"]["note_delay_beat_0"] > 0
    assert collected["analysis"]["window_onset_counts"]["extended_marker_beat_3"] > 0
    assert collected["analysis"]["frame_count"] == m.CANDIDATE_TARGET_FRAMES
    assert collected["fixture_identity"]["machine_type"] == 12
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
    fixture.write_bytes(candidate_fixture_bytes(b""))
    expect_value_error(
        lambda: m.validate_fixture_identity(fixture.read_bytes()),
        "EINS payload",
    )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(b"PSY3SONGx")
    render_dir = root / m.NAME
    write_provenance(root)
    for index in (1, 2):
        (
            render_dir
            / f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
        ).write_bytes(valid_wave)
    expect_value_error(
        lambda: m.collect_candidate(root),
        "fixture",
    )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(valid_fixture)
    render_dir = root / m.NAME
    write_provenance(root)
    truncated = pcm_wave(onsets, frames=60360)
    for index in (1, 2):
        (
            render_dir
            / f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
        ).write_bytes(truncated)
    expect_value_error(
        lambda: m.collect_candidate(root),
        "frame count does not match fixed-frame target",
    )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(valid_fixture)
    render_dir = root / m.NAME
    write_provenance(root)
    missing_windows = pcm_wave(
        [
            int(round(1.0 * beat)),
            int(round(1.0625 * beat)),
            int(round(1.125 * beat)),
            int(round(2.0 * beat)),
            int(round(2.0625 * beat)),
        ],
        frames=m.CANDIDATE_TARGET_FRAMES,
    )
    for index in (1, 2):
        (
            render_dir
            / f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
        ).write_bytes(missing_windows)
    expect_value_error(
        lambda: m.collect_candidate(root),
        "does not expose every command-bearing window",
    )


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(valid_fixture)
    render_dir = root / m.NAME
    write_provenance(root)
    late_only = pcm_wave(
        [
            int(round(0.0625 * beat)),
            int(round(1.0 * beat)),
            int(round(1.0625 * beat)),
            int(round(1.125 * beat)),
            int(round(2.0 * beat)),
            int(round(2.0625 * beat)),
            int(round(4.1 * beat)),
        ],
        frames=m.CANDIDATE_TARGET_FRAMES,
    )
    for index in (1, 2):
        (
            render_dir
            / f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
        ).write_bytes(late_only)
    expect_value_error(
        lambda: m.collect_candidate(root),
        "bounded beat-3 command window",
    )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(valid_fixture)
    render_dir = root / m.NAME
    write_provenance(root)
    for index in (1, 2):
        (
            render_dir
            / f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
        ).write_bytes(valid_wave)
    (render_dir / "render-probe.cpp").write_bytes(b"x")
    (render_dir / "render-probe.pro").write_bytes(b"x")
    (render_dir / "phase6c-delayed-retrigger-sampulse-render").write_bytes(b"x")
    expect_value_error(
        lambda: m.collect_candidate(root),
        "retained renderer source differs",
    )


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(valid_fixture)
    render_dir = root / m.NAME
    write_provenance(root)
    for index in (1, 2):
        (
            render_dir
            / f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
        ).write_bytes(valid_wave)
    binary = render_dir / "phase6c-delayed-retrigger-sampulse-render"
    binary.write_bytes(b"\x7fELFphase6c-test-renderer")
    attestation_path = render_dir / "renderer-build-provenance.json"
    attestation = json.loads(attestation_path.read_text(encoding="utf-8"))
    attestation["binary"]["sha256"] = m.digest(binary.read_bytes())
    attestation_path.write_text(
        json.dumps(attestation, sort_keys=True) + "\n", encoding="utf-8"
    )
    expect_value_error(
        lambda: m.collect_candidate(root),
        "executable provenance challenge failed",
    )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(valid_fixture)
    render_dir = root / m.NAME
    write_provenance(root)
    for index in (1, 2):
        (
            render_dir
            / f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
        ).write_bytes(valid_wave)
    (
        root / "delayed-retrigger" / "sampulse-candidate-render-1.log"
    ).write_text("render 1 PASS\n", encoding="utf-8")
    expect_value_error(
        lambda: m.collect_candidate(root),
        "renderer JSON summary",
    )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / m.FIXTURE
    fixture.parent.mkdir(parents=True)
    fixture.write_bytes(valid_fixture)
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
        "render_dialog_native_event_hook_armed": True,
        "render_dialog_event_message_pump_started": True,
        "render_dialog_dispatch_boundary_set": True,
        "render_dialog_dispatch_boundary_tick": 123456,
        "render_dialog_post_dispatch_observed_window_event_count": 1,
        "render_dialog_unresolved_post_dispatch_event_count": 0,
        "render_dialog_post_dispatch_event_count": 1,
        "dialog_discovery": "pumped-win-event-object-show-strictly-after-dispatch-tick",
        "preexisting_render_dialog_count": 1,
        "selected_render_dialog_native_handle": 12345,
        "selected_render_dialog_runtime_id": [1, 2, 3],
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

    unarmed_attempt = dict(completed_attempt)
    unarmed_attempt["render_dialog_native_event_hook_armed"] = False
    expect_value_error(
        lambda: m.validate_original_attempt(root, unarmed_attempt, 1),
        "post-dispatch dialog evidence",
    )

    unpumped_attempt = dict(completed_attempt)
    unpumped_attempt["render_dialog_event_message_pump_started"] = False
    expect_value_error(
        lambda: m.validate_original_attempt(root, unpumped_attempt, 1),
        "post-dispatch dialog evidence",
    )

    stale_discovery_attempt = dict(completed_attempt)
    stale_discovery_attempt["dialog_discovery"] = (
        "window-opened-event-after-successful-command-dispatch"
    )
    expect_value_error(
        lambda: m.validate_original_attempt(root, stale_discovery_attempt, 1),
        "post-dispatch dialog evidence",
    )

    ambiguous_event_attempt = dict(completed_attempt)
    ambiguous_event_attempt["render_dialog_post_dispatch_event_count"] = 2
    expect_value_error(
        lambda: m.validate_original_attempt(root, ambiguous_event_attempt, 1),
        "post-dispatch dialog evidence",
    )

    unresolved_event_attempt = dict(completed_attempt)
    unresolved_event_attempt["render_dialog_unresolved_post_dispatch_event_count"] = 1
    unresolved_event_attempt["render_dialog_post_dispatch_observed_window_event_count"] = 2
    expect_value_error(
        lambda: m.validate_original_attempt(root, unresolved_event_attempt, 1),
        "post-dispatch dialog evidence",
    )

    empty_observed_attempt = dict(completed_attempt)
    empty_observed_attempt["render_dialog_post_dispatch_observed_window_event_count"] = 0
    expect_value_error(
        lambda: m.validate_original_attempt(root, empty_observed_attempt, 1),
        "post-dispatch dialog evidence",
    )

    extra_observed_attempt = dict(completed_attempt)
    extra_observed_attempt["render_dialog_post_dispatch_observed_window_event_count"] = 2
    expect_value_error(
        lambda: m.validate_original_attempt(root, extra_observed_attempt, 1),
        "post-dispatch dialog evidence",
    )

    missing_boundary_attempt = dict(completed_attempt)
    missing_boundary_attempt["render_dialog_dispatch_boundary_set"] = False
    expect_value_error(
        lambda: m.validate_original_attempt(root, missing_boundary_attempt, 1),
        "post-dispatch dialog evidence",
    )

    invalid_tick_attempt = dict(completed_attempt)
    invalid_tick_attempt["render_dialog_dispatch_boundary_tick"] = -1
    expect_value_error(
        lambda: m.validate_original_attempt(root, invalid_tick_attempt, 1),
        "post-dispatch dialog evidence",
    )

    missing_tick_attempt = dict(completed_attempt)
    missing_tick_attempt.pop("render_dialog_dispatch_boundary_tick")
    expect_value_error(
        lambda: m.validate_original_attempt(root, missing_tick_attempt, 1),
        "post-dispatch dialog evidence",
    )

    ambiguous_attempt = dict(completed_attempt)
    ambiguous_attempt.update(
        {
            "outcome": "inconclusive",
            "process_exited": False,
            "process_exit_code": None,
            "output": None,
            "dialog_closed": False,
            "close_control_seen": False,
            "close_uia_invoked": False,
            "stable_output_polls": 0,
            "render_dialog_post_dispatch_observed_window_event_count": 2,
            "render_dialog_unresolved_post_dispatch_event_count": 0,
            "render_dialog_post_dispatch_event_count": 1,
            "diagnostics": [
                "multiple post-dispatch Psycle window-show events observed"
            ],
            "observed_output": {
                "path": original_path.name,
                "size_bytes": len(valid_wave),
                "sha256": m.digest(valid_wave),
            },
        }
    )
    inconclusive_runtime = {
        "schema_version": 1,
        "outcome": "inconclusive",
        "deterministic": False,
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
        "attempts": [ambiguous_attempt],
    }
    quarantine = m.validate_original_inconclusive_runtime(
        root, inconclusive_runtime
    )
    assert "post-dispatch dialog evidence" in quarantine["binding_error"]
    assert quarantine["observed_output"]["path"].endswith(original_path.name)
    assert quarantine["observed_output"]["sha256"] == m.digest(valid_wave)
    assert quarantine["retained_renders"] == []
    assert quarantine["inconclusive_reason"] == "ambiguous-final-render-attempt"

    first_binding = {
        "path": f"{m.NAME}/{original_path.name}",
        "sha256": m.digest(valid_wave),
    }

    teardown_runtime = dict(inconclusive_runtime)
    teardown_runtime["renders"] = [first_binding]
    teardown_runtime["attempts"] = [completed_attempt]
    teardown = m.validate_original_inconclusive_runtime(
        root, teardown_runtime
    )
    assert teardown["inconclusive_reason"] == (
        "first-render-dialog-teardown-failure"
    )
    assert teardown["binding_error"] is None
    assert teardown["retained_renders"] == [first_binding]
    assert len(teardown["retained_render_analyses"]) == 1

    closed_first_attempt = dict(completed_attempt)
    closed_first_attempt["dialog_closed"] = True
    closed_first_attempt["diagnostics"] = []

    partial_second_attempt = dict(ambiguous_attempt)
    partial_second_attempt["observed_output"] = None
    partial_runtime = dict(inconclusive_runtime)
    partial_runtime["renders"] = [first_binding]
    partial_runtime["attempts"] = [closed_first_attempt, partial_second_attempt]
    partial_quarantine = m.validate_original_inconclusive_runtime(
        root, partial_runtime
    )
    assert partial_quarantine["retained_renders"] == partial_runtime["renders"]
    assert len(partial_quarantine["retained_render_analyses"]) == 1
    assert "post-dispatch dialog evidence" in partial_quarantine["binding_error"]

    second_path = (
        render_dir / "original-delayed-retrigger-sampulse-runtime-2.wav"
    )
    different_wave = bytearray(valid_wave)
    different_wave[-2:] = struct.pack("<h", 1)
    different_wave = bytes(different_wave)
    second_path.write_bytes(different_wave)
    closed_second_attempt = dict(closed_first_attempt)
    closed_second_attempt["output"] = {
        "path": second_path.name,
        "sha256": m.digest(different_wave),
    }
    nondeterministic_runtime = dict(inconclusive_runtime)
    nondeterministic_runtime["renders"] = [
        first_binding,
        {
            "path": f"{m.NAME}/{second_path.name}",
            "sha256": m.digest(different_wave),
        },
    ]
    nondeterministic_runtime["attempts"] = [
        closed_first_attempt,
        closed_second_attempt,
    ]
    nondeterministic = m.validate_original_inconclusive_runtime(
        root, nondeterministic_runtime
    )
    assert nondeterministic["inconclusive_reason"] == (
        "nondeterministic-completed-render-pair"
    )
    assert nondeterministic["binding_error"] is None
    assert len(nondeterministic["retained_renders"]) == 2
    assert len(nondeterministic["retained_render_analyses"]) == 2
    assert (
        nondeterministic["retained_renders"][0]["sha256"]
        != nondeterministic["retained_renders"][1]["sha256"]
    )

    crash_attempt = dict(completed_attempt)
    crash_attempt.update(
        {
            "outcome": "inconclusive",
            "process_exited": True,
            "process_exit_code": -1073741819,
            "output": None,
            "dialog_closed": False,
            "stable_output_polls": 0,
            "diagnostics": ["reference exited during offline render"],
            "observed_output": {
                "path": original_path.name,
                "size_bytes": len(valid_wave),
                "sha256": m.digest(valid_wave),
            },
        }
    )
    crash_runtime = {
        **inconclusive_runtime,
        "outcome": "reference-process-exited-during-render",
        "attempts": [crash_attempt],
        "renders": [],
    }
    crash = m.validate_original_process_exit_runtime(
        root,
        crash_runtime,
        {"exit_code_before_termination": -1073741819},
    )
    assert crash["process_exit_code"] == -1073741819
    assert crash["observed_output"]["sha256"] == m.digest(valid_wave)
    assert crash["retained_renders"] == []

    early_ambiguous_attempt = dict(ambiguous_attempt)
    early_ambiguous_attempt.update(
        {
            "dialog_verified": False,
            "controls_configured": False,
            "save_invoked": False,
            "selected_render_dialog_native_handle": None,
            "selected_render_dialog_runtime_id": [],
            "dialog_discovery": None,
            "observed_output": None,
        }
    )
    early_runtime = dict(inconclusive_runtime)
    early_runtime["attempts"] = [early_ambiguous_attempt]
    early_root = root / "early-ambiguity"
    early_root.mkdir()
    early_quarantine = m.validate_original_inconclusive_runtime(
        early_root, early_runtime
    )
    assert "post-dispatch dialog evidence" in early_quarantine["binding_error"]
    assert early_quarantine["observed_output"] is None

    valid_binding_attempt = dict(ambiguous_attempt)
    valid_binding_attempt[
        "render_dialog_post_dispatch_observed_window_event_count"
    ] = 1
    valid_binding_runtime = dict(inconclusive_runtime)
    valid_binding_runtime["attempts"] = [valid_binding_attempt]
    expect_value_error(
        lambda: m.validate_original_inconclusive_runtime(
            root, valid_binding_runtime
        ),
        "valid post-dispatch dialog binding",
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
