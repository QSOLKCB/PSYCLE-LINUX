#!/usr/bin/env python3
"""Negative controls for the Phase 6C BPM/LPB/tick evidence lane."""
from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
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
        beat, tick, line = m.expected_sample_timing(rate, 8, True)
        rates.append({
            "sample_rate": rate,
            "samples_per_beat": beat,
            "samples_per_tick": tick,
            "samples_per_fixture_line": line,
        })
    return {
        "schema_version": 1,
        "load_returned": True,
        "song_name": m.TITLE,
        "bpm": m.BPM,
        "tick_speed": 8,
        "is_ticks": True,
        "marker_positions": list(m.POSITIONS),
        "derived_lpb": float(m.LPB),
        "sample_rates": rates,
        "reports": ["Load Warning: " + m.WARNING],
    }


def clean_log():
    return (
        "log: 0us: T: thread-id-123: # universalis # ../src/universalis/os/thread_name.cpp:56 # void universalis::os::thread_name::set_tls()\n"
        "log: 1us: T: thread-id-123: setting name for thread: id: 123, name: bpm-lpb-tick-probe\n"
        "log: 2us: T: bpm-lpb-tick-probe: # psycle-core # ../src/psycle/core/player.cpp:66 # void psycle::core::Player::start_threads()\n"
        "log: 3us: T: bpm-lpb-tick-probe: psycle: core: player: starting scheduler threads\n"
        "log: 4us: I: bpm-lpb-tick-probe: psycle: core: player: using 1 threads\n"
        "log: 5us: T: bpm-lpb-tick-probe: psycle: core: psy3 loader: loading psycle song fileformat version 3: /tmp/" + m.FIXTURE + "\n"
        "log: 6us: W: bpm-lpb-tick-probe: " + m.WARNING + "\n"
        "log: 7us: I: bpm-lpb-tick-probe: psycle: core: machine factory: create machine: loading with host: 0, plugin: <master>\n"
        "log: 8us: T: thread-id-123: # psycle-core # ../src/psycle/core/player.cpp:452 # void psycle::core::Player::stop_threads()\n"
        "log: 9us: T: thread-id-123: terminating and joining scheduler threads ...\n"
        "log: 10us: T: thread-id-123: # psycle-core # ../src/psycle/core/player.cpp:454 # void psycle::core::Player::stop_threads()\n"
        "log: 11us: T: thread-id-123: scheduler threads were not running\n"
    ).encode()



class CandidateParser(unittest.TestCase):
    def parse(self, value=None, log=None, exit_code=0):
        if value is None:
            value = valid_value()
        return m.parse_probe(json.dumps(value).encode(), clean_log() if log is None else log, exit_code)

    def test_valid_observation(self):
        self.assertEqual(self.parse()["observation"], "timing-model-observed")

    def test_player_time_info_float_precision_is_accepted(self):
        probe = valid_value()
        self.assertEqual(probe["sample_rates"][0]["samples_per_beat"], 19313.869140625)
        self.assertEqual(self.parse(probe)["observation"], "timing-model-observed")

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
        for level in ("T", "I", "W", "E"):
            with self.subTest(level=level):
                log = clean_log() + (
                    f"log: 4us: {level}: bpm-lpb-tick-probe: contaminated\n"
                ).encode()
                self.assertEqual(self.parse(log=log)["observation"], "inconclusive")

    def test_workflow_gate_requires_observed_candidate(self):
        observed = self.parse()
        self.assertIs(m.require_candidate_observed(observed), observed)
        with self.assertRaises(ValueError):
            m.require_candidate_observed(self.parse(exit_code=1))

    def test_source_identity_normalizes_only_crlf(self):
        self.assertEqual(
            m.canonical_text_bytes(b"one\r\ntwo\r\n"),
            b"one\ntwo\n",
        )
        with self.assertRaises(ValueError):
            m.canonical_text_bytes(b"one\rtwo\n")

    def test_probe_identity_is_bound_to_preserved_binary(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            probe = root / m.PROBE_ARTIFACT
            probe.parent.mkdir(parents=True)
            probe.write_bytes(b"candidate-probe-bytes")
            sha = m.digest(probe.read_bytes())
            receipt = {
                "probe": {"path": m.PROBE_ARTIFACT, "sha256": sha},
                "probe_sha256": sha,
            }
            self.assertEqual(
                m.validate_probe_identity(root, receipt),
                b"candidate-probe-bytes",
            )
            receipt["probe_sha256"] = "0" * 64
            with self.assertRaises(ValueError):
                m.validate_probe_identity(root, receipt)


class GeneratedObserver(unittest.TestCase):
    def generate(self, source: Path, output: Path):
        return subprocess.run(
            [
                sys.executable,
                str(ROOT / "scripts/phase6c-build-timing-observer.py"),
                str(source),
                str(output),
            ],
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )

    def test_dialog_is_opened_before_timing_poll(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "generated.ps1"
            process = self.generate(
                ROOT / "scripts/phase6c-original-windows-fixtures-v2.ps1",
                output,
            )
            self.assertEqual(process.returncode, 0, process.stderr)
            generated = output.read_text(encoding="utf-8")
            open_call = generated.index(
                "$timingDialogBootstrap = Open-Phase6cSongInformationDialog $process"
            )
            poll_call = generated.index(
                "$timingObservation = Get-Phase6cTimingUiObservation $process"
            )
            self.assertLess(open_call, poll_call)
            self.assertIn(
                "dialog_bootstrap = $timingDialogBootstrap",
                generated,
            )

    def test_builder_accepts_crlf_checkout_of_pinned_base(self):
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            source = temp_path / "base.ps1"
            output = temp_path / "generated.ps1"
            canonical = (
                ROOT / "scripts/phase6c-original-windows-fixtures-v2.ps1"
            ).read_text(encoding="utf-8")
            source.write_bytes(canonical.replace("\n", "\r\n").encode("utf-8"))
            process = self.generate(source, output)
            self.assertEqual(process.returncode, 0, process.stderr)
            self.assertTrue(output.is_file())


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
            "dialog_bootstrap": {
                "opened": True, "outcome": "opened", "diagnostics": [],
            },
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

    def test_stable_original_difference_is_preserved(self):
        def change(timing):
            timing["observed_values"]["ticks_per_beat"] = 8
            timing["matches_fixture_expected"] = False
        self.mutate(change)
        result = self.validate()
        self.assertEqual(result["timing_result"], "observed")
        self.assertEqual(result["observed_values"]["ticks_per_beat"], 8)
        self.assertIs(result["matches_fixture_expected"], False)
        self.assertEqual(result["parity_status"], "UNKNOWN")

    def test_original_values_cannot_be_relabelled(self):
        self.mutate(lambda timing: timing["observed_values"].update(ticks_per_beat=8))
        with self.assertRaises(ValueError): self.validate()

    def test_original_match_flag_cannot_claim_difference_when_values_match(self):
        self.mutate(lambda timing: timing.update(matches_fixture_expected=False))
        with self.assertRaises(ValueError): self.validate()

    def test_observed_original_requires_verified_dialog_bootstrap(self):
        self.mutate(lambda timing: timing.update(dialog_bootstrap={
            "opened": False, "outcome": "menu-item-missing", "diagnostics": ["missing"]
        }))
        with self.assertRaises(ValueError): self.validate()


if __name__ == "__main__":
    unittest.main()
