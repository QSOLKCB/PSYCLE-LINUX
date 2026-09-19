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
            "16 contracts remain UNKNOWN",
            "116 contracts remain UNKNOWN",
        )
        self.check(False)

    def test_sequence_order_name_without_pass_claim_is_rejected(self):
        self.mutate_report(
            "sequence/pattern order are scoped PASS results",
            "sequence/pattern order remains UNKNOWN",
        )
        self.check(False)


if __name__ == "__main__":
    unittest.main()
