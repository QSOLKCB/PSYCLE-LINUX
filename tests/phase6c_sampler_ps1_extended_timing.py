#!/usr/bin/env python3
"""Negative controls for the Phase 6C PS1 extended-timing lane."""
from __future__ import annotations

import base64
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import wave
import zlib

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/phase6c-sampler-ps1-extended-timing.py"
spec = importlib.util.spec_from_file_location("phase6c_ps1_extended_timing", SCRIPT)
assert spec is not None and spec.loader is not None
timing = importlib.util.module_from_spec(spec)
spec.loader.exec_module(timing)

for variant in timing.VARIANTS:
    expected = timing.expected_probe(variant)
    valid = {
        **expected,
        "audio_active": True,
        "active_frame_count": 100,
        "first_active_frame": 10,
        "last_active_frame": 109,
        "final_play_beat": 1.0,
    }
    assert timing.validate_probe(dict(valid), variant) == valid

    wrong = dict(valid)
    wrong["trigger_samples"] = timing.EXPECTED_TRIGGER + 1
    try:
        timing.validate_probe(wrong, variant)
    except ValueError as exc:
        assert "trigger_samples" in str(exc)
    else:
        raise AssertionError("off-by-one PS1 semantic trigger must fail")

    tiny_blocks = dict(valid)
    tiny_blocks["player_max_work_block_samples"] = 1
    try:
        timing.validate_probe(tiny_blocks, variant)
    except ValueError as exc:
        assert "player_max_work_block_samples" in str(exc)
    else:
        raise AssertionError("non-production timing work-block contract must fail")

    silent = {
        **expected,
        "audio_active": False,
        "active_frame_count": 0,
        "first_active_frame": None,
        "last_active_frame": None,
        "final_play_beat": 1.0,
    }
    assert timing.validate_probe(dict(silent), variant) == silent

clean_receipt = {
    "load_result": "inconclusive",
    "ui_automation_diagnostics": [],
    "runtime_identity_diagnostics": [],
    "error_marker": None,
    "application_error_marker": None,
    "main_window_seen": True,
    "stable_marker_polls": 8,
    "load_evidence_marker": "marker",
}
clean_runtime = {
    "outcome": "reference-process-exited-during-render",
    "pre_render_load": {
        "clean_accepted_load": True,
        "load_warning_dismissed": True,
        "process_running_before_render": True,
        "stable_marker_polls": 8,
        "matched_marker": "marker",
    },
}
timing.clean_load_gate(clean_receipt, clean_runtime)

completed_with_inconclusive_load = dict(clean_runtime)
completed_with_inconclusive_load["outcome"] = "rendered-twice"
try:
    timing.clean_load_gate(clean_receipt, completed_with_inconclusive_load)
except ValueError as exc:
    assert "clean pre-render load gate" in str(exc)
else:
    raise AssertionError("completed renders must retain accepted final load result")

with tempfile.TemporaryDirectory() as temporary:
    silent_wav = Path(temporary) / "silent.wav"
    with wave.open(str(silent_wav), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(timing.OUTPUT_RATE)
        handle.writeframes(b"\x00\x00" * timing.RENDER_FRAMES)
    analysis = timing.analyze_timing_wave(silent_wav)
    assert analysis["audio_active"] is False
    assert analysis["first_active_frame"] is None
    assert analysis["last_active_frame"] is None
    assert analysis["active_frame_count"] == 0
    assert analysis["active_span_frames"] == 0

    short_wav = Path(temporary) / "short.wav"
    with wave.open(str(short_wav), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(timing.OUTPUT_RATE)
        handle.writeframes(b"\x00\x00" * (timing.RENDER_FRAMES - 1))
    try:
        timing.analyze_timing_wave(short_wav)
    except ValueError as exc:
        assert "shorter than the complete fixture interval" in str(exc)
    else:
        raise AssertionError("truncated full-song timing render must fail")

    fake_probe = Path(temporary) / "fake-probe"
    fake_probe.write_text("#!/usr/bin/env python3\nprint('{}')\n", encoding="utf-8")
    fake_probe.chmod(0o755)
    try:
        timing.verify_probe_executable(fake_probe)
    except ValueError as exc:
        assert "audited ELF build" in str(exc)
    else:
        raise AssertionError("non-audited timing probe executable must fail")

cpp_source = (ROOT / "tests/phase6c_sampler_ps1_extended_timing.cpp").read_text(
    encoding="utf-8"
)
assert timing.PROBE_IDENTITY in cpp_source
assert timing.BOUNDARY_TOLERANCES_FRAMES == {"delay": 4, "noteoff": 6}
assert timing.BOUNDARY_FIELDS == {
    "delay": "first_active_frame",
    "noteoff": "last_active_frame",
}

known_log = (
    b"log:      52us: T: ps1-extended-timing: "
    b"psycle: core: player: starting scheduler threads\n"
    b"log:      73us: I: ps1-extended-timing: "
    b"psycle: core: player: using 1 threads\n"
    b"log:     183us: W: ps1-extended-timing: "
    b"This file is from a newer version of Psycle! "
    b"This process will try to load it anyway.\n"
)
assert timing.classify_probe_diagnostics(known_log) == [
    "player-thread-count",
    "player-thread-start",
    "psy3-newer-version-warning",
]
try:
    timing.classify_probe_diagnostics(b"WARNING: damaged PSY3 chunk\n")
except ValueError as exc:
    assert "unclassified diagnostic" in str(exc)
else:
    raise AssertionError("unclassified zero-exit probe diagnostic must fail")

archive = ROOT / "phase6c/evidence/sampler-ps1/pitch-hosted"
manifest = json.loads((archive / "raw-manifest.json").read_text(encoding="utf-8"))
for item in manifest["files"]:
    encoded = (archive / item["path"]).read_text(encoding="ascii").strip()
    raw = zlib.decompress(base64.b64decode(encoded, validate=True))
    assert len(raw) == item["decoded_size_bytes"]
    assert hashlib.sha256(raw).hexdigest() == item["decoded_sha256"]

observation = json.loads((archive / "observation.json").read_text(encoding="utf-8"))
assert observation["status"] == "UNKNOWN"
assert observation["comparison_ready"] is False
assert observation["original"]["process_exit_hex"] == "0xC0000005"
assert observation["original"]["outcome"] == "reference-process-exited-during-render"
assert observation["original"]["load_result"] == "inconclusive"
assert observation["original"]["pre_render_clean_accepted_load"] is True
assert observation["candidate"]["deterministic"] is True
assert len(set(observation["candidate"]["render_sha256s"])) == 1

with tempfile.TemporaryDirectory() as temporary:
    generated = Path(temporary) / "observer.ps1"
    result = subprocess.run(
        [
            "python3",
            str(ROOT / "scripts/phase6c-build-sampler-ps1-extended-timing-observer.py"),
            str(ROOT / "scripts/phase6c-original-windows-fixtures-v2.ps1"),
            str(generated),
        ],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    observer = generated.read_text(encoding="utf-8")
    assert "[switch]$ObserveSamplerPs1ExtendedTiming" in observer
    assert 'name = "sampler-ps1-extended-delay"' in observer
    assert 'name = "sampler-ps1-extended-noteoff"' in observer
    assert "Invoke-Phase6cAudioRender" in observer
    assert "runtime_execution = $runtimeExecution" in observer

print("phase6c-sampler-ps1-extended-timing: PASS")
