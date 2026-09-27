#!/usr/bin/env python3
"""Build/check dual-generation PSY3 PS1 E-D3/E-C3 timing fixtures."""
from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "scripts/phase6c-sampler-ps1-pitch-compat.py"
spec = importlib.util.spec_from_file_location("phase6c_ps1_pitch_compat", BASE)
assert spec is not None and spec.loader is not None
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)

base.AUTHORED_SAMPLE_RATE = 44100
base.AUTHORED_SAMPLE_FRAMES = 44100

VARIANTS = ("delay", "noteoff")


def paths(root: Path, variant: str) -> tuple[Path, Path, Path]:
    return (
        root / f"phase6c-sampler-ps1-extended-{variant}-original.psy",
        root / f"phase6c-sampler-ps1-extended-{variant}-candidate.psy",
        root / f"phase6c-sampler-ps1-extended-{variant}-candidate.pcm16le",
    )


def process(mode: str, root: Path) -> None:
    for variant in VARIANTS:
        original, candidate, pcm_path = paths(root, variant)
        source = original.read_bytes()
        if mode == "convert":
            if candidate.exists() or pcm_path.exists():
                raise ValueError("refusing existing extended-timing bridge output")
            candidate.write_bytes(base.convert(source))
            pcm_path.write_bytes(base.extract_pcm16le(source))
        info = base.inspect_pair(
            source, candidate.read_bytes(), pcm_path.read_bytes()
        )
        if (
            info["authored_sample_rate"] != 44100
            or info["authored_sample_frames"] != 44100
            or info["pcm16le_bytes"] != 88200
            or info["candidate_pre_injection_wave_state"] != "empty"
            or info["candidate_modern_sample_chunk_removed"] is not True
        ):
            raise ValueError("extended-timing bridge identity changed")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("convert", "check"))
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    process(args.mode, args.root)
    print(
        "phase6c-sampler-ps1-extended-timing-compat: PASS "
        "variants=delay,noteoff sample_rate=44100 frames=44100 "
        "candidate_smsb_removed=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
