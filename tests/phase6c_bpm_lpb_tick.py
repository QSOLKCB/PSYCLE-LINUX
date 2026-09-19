#!/usr/bin/env python3
"""Negative controls for the Phase 6C BPM/LPB/tick evidence lane."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import types
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "phase6c_bpm_lpb_tick", ROOT / "scripts/phase6c-bpm-lpb-tick-evidence.py"
)
m = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(m)


def valid_value():
    rates = []
    for rate in m.RATES:
        beat = rate * 60.0 / m.BPM
        rates.append({
            "sample_rate": rate,
            "samples_per_beat": beat,
            "samples_per_tick": beat / 8,
            "samples_per_fixture_line": beat / m.LPB,
        })
    return {
        "schema_version": 1,
        "load_returned": True,
        "song_name": m.TITLE,
        "bpm": m.BPM,
        "tick_speed": 8,
        "is_ticks": True,
        "marker_positions": m.POSITIONS,
        "derived_lpb": float(m.LPB),
        "sample_rates": rates,
        "reports": ["Load Warning: " + m.WARNING],
    }


def clean_log():
    return (
        "log: 1us: T: bpm-lpb-tick-probe: psycle: core: psy3 loader: loading psycle song fileformat version 3: /tmp/" + m.FIXTURE + "\n"
        "log: 2us: W: bpm-lpb-tick-probe: " + m.WARNING + "\n"
        "log: 3us: I: bpm-lpb-tick-probe: psycle: core: machine factory: create machine: loading with host: 0, plugin: <master>\n"
    ).encode()


class CandidateParser(unittest.TestCase):
    def parse(self, value=None, log=None, exit_code=0):
        if value is None:
            value = valid_value()
        return m.parse_probe(json.dumps(value).encode(), clean_log() if log is None else log, exit_code)

    def test_valid_observation(self):
        self.assertEqual(self.parse()["observation"], "timing-model-observed")

    def test_exit_code_requires_integer_zero(self):
        for value in (False, 0.0, 1):
            with self.subTest(value=value):
                self.assertEqual(self.parse(exit_code=value)["observation"], "inconclusive")

    def test_tick_speed_requires_non_boolean_integer(self):
        for value in (True, 8.0, 0):
            probe = valid_value(); probe["tick_speed"] = value
            with self.subTest(value=value):
                self.assertEqual(self.parse(probe)["observation"], "inconclusive")

    def test_marker_spacing_is_frozen(self):
        probe = valid_value(); probe["marker_positions"][2] = 0.3
        self.assertEqual(self.parse(probe)["observation"], "inconclusive")

    def test_derived_lpb_is_frozen(self):
        probe = valid_value(); probe["derived_lpb"] = 4
        self.assertEqual(self.parse(probe)["observation"], "inconclusive")

    def test_sample_formula_is_rederived(self):
        probe = valid_value(); probe["sample_rates"][1]["samples_per_tick"] += 1
        self.assertEqual(self.parse(probe)["observation"], "inconclusive")

    def test_unknown_diagnostic_is_rejected(self):
        log = clean_log() + b"log: 4us: E: bpm-lpb-tick-probe: contaminated\n"
        self.assertEqual(self.parse(log=log)["observation"], "inconclusive")


class OriginalProjection(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.candidate = Path(self.temp.name) / "candidate"; self.original = Path(self.temp.name) / "original"
        self.candidate.mkdir(); self.original.mkdir()
        timing = {
            "expected_values": {"tempo": 137, "lines_per_beat": 8, "ticks_per_beat": 24,
                "extra_tick_per_line": 0, "real_tempo": 137, "real_ticks_per_beat": 24},
            "observed_values": {"tempo": 137, "lines_per_beat": 8, "ticks_per_beat": 24,
                "extra_tick_per_line": 0, "real_tempo": 137, "real_ticks_per_beat": 24},
            "stable_polls": 4, "result": "observed", "matches_fixture_expected": True,
        }
        (self.original / "original-bpm-lpb-tick.json").write_text(json.dumps({"timing_ui": timing}))

    def validate(self, load_result="accepted"):
        fake = types.SimpleNamespace(validate_pair=lambda *args: load_result)
        with patch.object(m, "load_module", return_value=fake):
            return m.validate_original(self.candidate, self.original)

    def mutate(self, change):
        path = self.original / "original-bpm-lpb-tick.json"
        value = json.loads(path.read_text()); change(value["timing_ui"])
        path.write_text(json.dumps(value))

    def test_stable_original_observation(self):
        self.assertEqual(self.validate()["timing_result"], "observed")

    def test_original_requires_clean_accepted_load(self):
        with self.assertRaises(ValueError): self.validate("inconclusive")

    def test_original_requires_four_stable_polls(self):
        self.mutate(lambda timing: timing.update(stable_polls=3))
        with self.assertRaises(ValueError): self.validate()

    def test_original_values_cannot_be_relabelled(self):
        self.mutate(lambda timing: timing["observed_values"].update(ticks_per_beat=8))
        with self.assertRaises(ValueError): self.validate()


if __name__ == "__main__":
    unittest.main()
