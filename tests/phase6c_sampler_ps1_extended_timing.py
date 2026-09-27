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
import zlib

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/phase6c-sampler-ps1-extended-timing.py"
spec = importlib.util.spec_from_file_location("phase6c_ps1_extended_timing", SCRIPT)
assert spec is not None and spec.loader is not None
timing = importlib.util.module_from_spec(spec)
spec.loader.exec_module(timing)

for variant in timing.VARIANTS:
    expected = timing.expected_probe(variant)
    assert timing.validate_probe(dict(expected), variant) == expected

    wrong = dict(expected)
    wrong["trigger_samples"] = timing.EXPECTED_TRIGGER + 1
    try:
        timing.validate_probe(wrong, variant)
    except ValueError as exc:
        assert "trigger_samples" in str(exc)
    else:
        raise AssertionError("off-by-one PS1 trigger must fail")

    early = dict(expected)
    early["before_boundary_preserved"] = False
    try:
        timing.validate_probe(early, variant)
    except ValueError as exc:
        assert "before_boundary_preserved" in str(exc)
    else:
        raise AssertionError("early PS1 trigger must fail")

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
