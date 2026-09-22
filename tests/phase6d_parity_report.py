#!/usr/bin/env python3
"""Negative controls for the Phase 6D human parity-report projection."""
from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ParityReportSummary(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "phase6c").mkdir()
        (self.root / "scripts").mkdir()
        shutil.copyfile(
            ROOT / "phase6c/compatibility-matrix.json",
            self.root / "phase6c/compatibility-matrix.json",
        )
        shutil.copyfile(
            ROOT / "scripts/phase6d-validate-parity-report.py",
            self.root / "scripts/phase6d-validate-parity-report.py",
        )
        shutil.copyfile(
            ROOT / "PSYCLE_CORE_PARITY.md",
            self.root / "PSYCLE_CORE_PARITY.md",
        )
        self.report = self.root / "PSYCLE_CORE_PARITY.md"

    def check(self, success):
        result = subprocess.run(
            ["python3", "scripts/phase6d-validate-parity-report.py"],
            cwd=self.root,
            capture_output=True,
            text=True,
        )
        self.assertEqual(
            result.returncode == 0,
            success,
            result.stdout + result.stderr,
        )

    def mutate_report(self, old, new):
        text = self.report.read_text()
        self.assertIn(old, text)
        self.report.write_text(text.replace(old, new, 1))

    def test_current_report_validates(self):
        self.check(True)

    def test_unknown_count_must_match_complete_numeral(self):
        self.mutate_report(
            "15 contracts remain UNKNOWN",
            "116 contracts remain UNKNOWN",
        )
        self.check(False)

    def test_sequence_order_name_without_pass_claim_is_rejected(self):
        self.mutate_report(
            "sequence/pattern order are scoped PASS results",
            "sequence/pattern order remains UNKNOWN",
        )
        self.check(False)

    def test_sequence_order_contradictory_pass_and_unknown_is_rejected(self):
        self.mutate_report(
            "sequence/pattern order are scoped PASS results",
            "sequence/pattern order are scoped PASS results but remain UNKNOWN",
        )
        self.check(False)

    def test_sequence_order_later_comma_contradiction_is_rejected(self):
        self.mutate_report(
            "sequence/pattern order are scoped PASS results, serialization/save capability",
            "sequence/pattern order are scoped PASS results, yet it remains UNKNOWN; serialization/save capability",
        )
        self.check(False)

    def test_sequence_order_later_semicolon_contradiction_is_rejected(self):
        self.mutate_report(
            "sequence/pattern order are scoped PASS results, serialization/save capability",
            "sequence/pattern order are scoped PASS results; nevertheless it remains UNKNOWN, serialization/save capability",
        )
        self.check(False)

    def test_delayed_early_controls_negated_survival_is_rejected(self):
        self.mutate_report(
            "no-previous-instrument and missing-sample controls survive",
            "no-previous-instrument and missing-sample controls do not survive",
        )
        self.check(False)

    def test_delayed_enabled_sample_negated_exit_is_rejected(self):
        self.mutate_report(
            "both enabled-sample variants (constructor-default original Instrument and serialized instrument state) exit",
            "both enabled-sample variants (constructor-default original Instrument and serialized instrument state) do not exit",
        )
        self.check(False)

    def test_delayed_early_controls_neither_nor_is_rejected(self):
        self.mutate_report(
            "no-previous-instrument and missing-sample controls survive",
            "neither no-previous-instrument nor missing-sample controls survive",
        )
        self.check(False)

    def test_delayed_enabled_sample_neither_variant_exits_is_rejected(self):
        self.mutate_report(
            "both enabled-sample variants (constructor-default original Instrument and serialized instrument state) exit",
            "neither enabled-sample variant (constructor-default original Instrument nor serialized instrument state) exits",
        )
        self.check(False)

    def test_delayed_non_zero_byte_output_is_rejected(self):
        self.mutate_report(
            "exit with `0xC0000005` and zero-byte output",
            "exit with `0xC0000005` and non-zero-byte output",
        )
        self.check(False)

    def test_delayed_enabled_sample_no_variants_exit_is_rejected(self):
        self.mutate_report(
            "both enabled-sample variants (constructor-default original Instrument and serialized instrument state) exit",
            "no enabled-sample variants (constructor-default original Instrument and serialized instrument state) exit",
        )
        self.check(False)

    def test_delayed_early_controls_cannot_survive_is_rejected(self):
        self.mutate_report(
            "no-previous-instrument and missing-sample controls survive",
            "no-previous-instrument and missing-sample controls cannot survive",
        )
        self.check(False)

    def test_delayed_early_controls_unable_to_survive_is_rejected(self):
        self.mutate_report(
            "no-previous-instrument and missing-sample controls survive",
            "no-previous-instrument and missing-sample controls are unable to survive",
        )
        self.check(False)

    def test_delayed_work_boundary_release_control_negation_is_rejected(self):
        self.mutate_report(
            "enabled-sample release/no-active-voice control survives",
            "enabled-sample release/no-active-voice control does not survive",
        )
        self.check(False)

    def test_delayed_work_boundary_exit_negation_is_rejected(self):
        self.mutate_report(
            "one-row `E-DF`, four-beat `E-DF`, and one-row ordinary-note fixtures all exit",
            "one-row `E-DF`, four-beat `E-DF`, and one-row ordinary-note fixtures do not exit",
        )
        self.check(False)

    def test_delayed_work_boundary_non_zero_output_is_rejected(self):
        self.mutate_report(
            "all exit with `0xC0000005` and zero-byte output",
            "all exit with `0xC0000005` and non-zero-byte output",
        )
        self.check(False)

    def test_delayed_work_boundary_controller_work_reversal_is_rejected(self):
        self.mutate_report(
            "it cannot reach `controller.Work`",
            "it can reach `controller.Work`",
        )
        self.check(False)

    def test_delayed_work_boundary_definitive_voice_tick_location_is_rejected(self):
        self.mutate_report(
            "Voice selection/setup, `Voice::Tick` initialization, and pre-`controller.Work` `Voice::Work` entry remain unresolved.",
            "The current fault boundary is inside `Voice::Tick` initialization before first `Voice::Work`.",
        )
        self.check(False)

    def test_delayed_work_boundary_unresolved_alternatives_are_required(self):
        self.mutate_report(
            "Voice selection/setup, `Voice::Tick` initialization, and pre-`controller.Work` `Voice::Work` entry remain unresolved.",
            "Voice selection/setup, `Voice::Tick` initialization, and pre-`controller.Work` `Voice::Work` entry are excluded.",
        )
        self.check(False)

    def test_delayed_work_boundary_short_fixture_survival_is_rejected(self):
        self.mutate_report(
            "the enabled-sample release/no-active-voice control survives with a finalized silent WAV, while one-row `E-DF`, four-beat `E-DF`, and one-row ordinary-note fixtures all exit with `0xC0000005` and zero-byte output",
            "one-row Sampler-local `E-DF` remains running, the enabled-sample release/no-active-voice control survives, and four-beat `E-DF` and one-row ordinary-note fixtures exit with `0xC0000005` and zero-byte output",
        )
        self.check(False)


if __name__ == "__main__":
    unittest.main()
