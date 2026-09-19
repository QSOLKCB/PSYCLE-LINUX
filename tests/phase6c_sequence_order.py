#!/usr/bin/env python3
"""Synthetic negative controls for Phase 6C sequence-order evidence."""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import types
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "sequence_order", ROOT / "scripts/phase6c-sequence-order-evidence.py"
)
m = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(m)

RAW = {
    "schema_version": 1,
    "load_returned": True,
    "song_name": m.TITLE,
    "sequence_lines": [
        {
            "line_index": 0,
            "entries": [
                {"position": 0.0, "pattern_id": -1, "pattern_name": "master"},
            ],
        },
        {
            "line_index": 1,
            "entries": [
                {"position": 0.0, "pattern_id": 0, "pattern_name": "Order Alpha0"},
                {"position": 1.0, "pattern_id": 2, "pattern_name": "Order Gamma2"},
                {"position": 4.0, "pattern_id": 1, "pattern_name": "Order Beta1"},
                {"position": 6.0, "pattern_id": 2, "pattern_name": "Order Gamma2"},
            ],
        },
    ],
    "reports": ["Load Warning: " + m.WARNING],
}

CLEAN_LOG = b"""log:       0us: T: thread-id-140586135613824: # universalis # ../src/universalis/os/thread_name.cpp:56 # void universalis::os::thread_name::set_tls()
log:      23us: T: thread-id-140586135613824: setting name for thread: id: 140586135613824, name: sequence-order-probe
log:      43us: T: sequence-order-probe: # psycle-core # ../src/psycle/core/player.cpp:66 # void psycle::core::Player::start_threads()
log:      52us: T: sequence-order-probe: psycle: core: player: starting scheduler threads
log:      74us: I: sequence-order-probe: psycle: core: player: using 1 threads
log:     161us: T: sequence-order-probe: psycle: core: psy3 loader: loading psycle song fileformat version 3: /tmp/evidence/sequence-order/phase6c-sequence-order.psy
log:     180us: W: sequence-order-probe: This file is from a newer version of Psycle! This process will try to load it anyway.
log:     351us: I: sequence-order-probe: psycle: core: machine factory: create machine: loading with host: 0, plugin: <master>
log:   23750us: T: thread-id-140586135613824: # psycle-core # ../src/psycle/core/player.cpp:452 # void psycle::core::Player::stop_threads()
log:   23759us: T: thread-id-140586135613824: terminating and joining scheduler threads ...
log:   23766us: T: thread-id-140586135613824: # psycle-core # ../src/psycle/core/player.cpp:454 # void psycle::core::Player::stop_threads()
log:   23771us: T: thread-id-140586135613824: scheduler threads were not running
"""


class CandidateProbe(unittest.TestCase):
    def derive(self, raw=None, log=CLEAN_LOG, exit_code=0):
        value = RAW if raw is None else raw
        return m.parse_probe(json.dumps(value).encode(), log, exit_code)

    def test_clean_model_observation(self):
        result = self.derive()
        self.assertEqual(result["observation"], "sequence-model-observed")
        self.assertEqual(result["play_order"], m.EXPECTED_ORDER)
        self.assertEqual(
            [entry["pattern_id"] for entry in result["sequence_lines"][1]["entries"]],
            m.EXPECTED_ORDER,
        )

    def test_quiet_or_unknown_diagnostics_are_inconclusive(self):
        self.assertEqual(self.derive(log=b"")["observation"], "inconclusive")
        contaminated = CLEAN_LOG + (
            b"log: 999us: I: sequence-order-probe: unexpected loader state\n"
        )
        self.assertEqual(
            self.derive(log=contaminated)["observation"], "inconclusive"
        )

    def test_required_loader_markers_cannot_be_removed(self):
        for marker in (
            b"loading psycle song fileformat version 3",
            b"This file is from a newer version of Psycle!",
            b"plugin: <master>",
        ):
            lines = [
                line
                for line in CLEAN_LOG.splitlines(keepends=True)
                if marker not in line
            ]
            with self.subTest(marker=marker):
                self.assertEqual(
                    self.derive(log=b"".join(lines))["observation"],
                    "inconclusive",
                )

    def test_abnormal_process_and_bad_load_never_observe(self):
        for code in (1, 65, 70, 124, -11, None, False):
            with self.subTest(code=code):
                self.assertEqual(
                    self.derive(exit_code=code)["observation"], "inconclusive"
                )
        bad = copy.deepcopy(RAW)
        bad["load_returned"] = False
        self.assertEqual(self.derive(bad)["observation"], "inconclusive")

    def test_structure_must_be_well_formed_and_ordered(self):
        bad = copy.deepcopy(RAW)
        bad["sequence_lines"][1]["entries"][2]["position"] = 0.5
        self.assertEqual(self.derive(bad)["observation"], "inconclusive")

        bad = copy.deepcopy(RAW)
        bad["sequence_lines"][0]["line_index"] = 4
        self.assertEqual(self.derive(bad)["observation"], "inconclusive")

        bad = copy.deepcopy(RAW)
        bad["sequence_lines"][1]["entries"][1]["pattern_id"] = True
        self.assertEqual(self.derive(bad)["observation"], "inconclusive")

    def test_master_line_is_separate_from_canonical_play_order(self):
        bad = copy.deepcopy(RAW)
        bad["sequence_lines"][0]["entries"][0]["pattern_id"] = 99
        result = self.derive(bad)
        self.assertEqual(result["observation"], "inconclusive")
        self.assertEqual(result["play_order"], [])

    def test_missing_master_line_is_inconclusive(self):
        bad = copy.deepcopy(RAW)
        bad["sequence_lines"] = [bad["sequence_lines"][1]]
        bad["sequence_lines"][0]["line_index"] = 0
        result = self.derive(bad)
        self.assertEqual(result["observation"], "inconclusive")
        self.assertEqual(result["play_order"], [])

    def test_duplicate_master_lines_are_inconclusive(self):
        bad = copy.deepcopy(RAW)
        duplicate = copy.deepcopy(bad["sequence_lines"][0])
        duplicate["line_index"] = 1
        bad["sequence_lines"].insert(1, duplicate)
        bad["sequence_lines"][2]["line_index"] = 2
        result = self.derive(bad)
        self.assertEqual(result["observation"], "inconclusive")
        self.assertEqual(result["play_order"], [])

    def test_master_entry_must_be_exactly_canonical(self):
        mutations = (
            ("position", 1.0),
            ("pattern_name", "Master"),
        )
        for field, value in mutations:
            bad = copy.deepcopy(RAW)
            bad["sequence_lines"][0]["entries"][0][field] = value
            with self.subTest(field=field):
                result = self.derive(bad)
                self.assertEqual(result["observation"], "inconclusive")
                self.assertEqual(result["play_order"], [])

        bad = copy.deepcopy(RAW)
        bad["sequence_lines"][0]["entries"].append(
            {"position": 1.0, "pattern_id": -1, "pattern_name": "master"}
        )
        result = self.derive(bad)
        self.assertEqual(result["observation"], "inconclusive")
        self.assertEqual(result["play_order"], [])

    def test_wrong_song_or_reports_are_inconclusive(self):
        for field, value in (
            ("song_name", "other"),
            ("reports", []),
            ("schema_version", True),
        ):
            bad = copy.deepcopy(RAW)
            bad[field] = value
            with self.subTest(field=field):
                self.assertEqual(self.derive(bad)["observation"], "inconclusive")

    def test_artifact_paths_cannot_escape(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for value in ("../escape", "/tmp/escape", "a\\b"):
                with self.subTest(value=value), self.assertRaises(ValueError):
                    m.child(root, value)


class OriginalSequenceOrder(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.candidate = Path(self.temp.name) / "candidate"
        self.original = Path(self.temp.name) / "original"
        self.candidate.mkdir()
        self.original.mkdir()

        (self.candidate / "candidate-sequence-order.json").write_text(
            json.dumps(
                {
                    "fixture_expected_ui_labels": m.EXPECTED_UI_LABELS,
                }
            )
        )
        self.receipt = {
            "sequence_order_ui": {
                "expected_labels": m.EXPECTED_UI_LABELS,
                "observed_labels": m.EXPECTED_UI_LABELS,
                "stable_polls": 4,
                "result": "observed",
                "matches_fixture_expected": True,
            }
        }

    def run_validation(self, load_result="accepted"):
        (self.original / "original-sequence-order.json").write_text(
            json.dumps(self.receipt)
        )
        fake = types.SimpleNamespace(
            validate_pair=lambda *args, **kwargs: load_result
        )
        with patch.object(m, "load_module", return_value=fake):
            return m.validate_original(self.candidate, self.original)

    def test_matching_stable_original_order_is_recorded(self):
        result = self.run_validation()
        self.assertEqual(result["sequence_order_result"], "observed")
        self.assertTrue(result["matches_fixture_expected"])
        self.assertEqual(result["parity_status"], "UNKNOWN")

    def test_stable_difference_is_valid_observation_not_self_promoted(self):
        labels = ["00: 00", "01: 01", "02: 02", "03: 01"]
        self.receipt["sequence_order_ui"].update(
            observed_labels=labels,
            stable_polls=5,
            result="observed",
            matches_fixture_expected=False,
        )
        result = self.run_validation()
        self.assertEqual(result["observed_labels"], labels)
        self.assertFalse(result["matches_fixture_expected"])
        self.assertEqual(result["parity_status"], "UNKNOWN")

    def test_observed_order_requires_clean_accepted_load(self):
        for load_result in ("inconclusive", "rejected"):
            with self.subTest(load_result=load_result), self.assertRaises(ValueError):
                self.run_validation(load_result)

    def test_unstable_or_partial_order_cannot_be_conclusive(self):
        for labels, polls in (
            (m.EXPECTED_UI_LABELS, 3),
            (m.EXPECTED_UI_LABELS[:3], 4),
        ):
            self.receipt["sequence_order_ui"].update(
                observed_labels=labels,
                stable_polls=polls,
                result="observed",
                matches_fixture_expected=True,
            )
            with self.subTest(labels=labels, polls=polls), self.assertRaises(
                ValueError
            ):
                self.run_validation()

    def test_match_flag_must_agree_with_actual_labels(self):
        labels = ["00: 00", "01: 01", "02: 02", "03: 01"]
        self.receipt["sequence_order_ui"].update(
            observed_labels=labels,
            stable_polls=4,
            result="observed",
            matches_fixture_expected=True,
        )
        with self.assertRaises(ValueError):
            self.run_validation()

    def test_inconclusive_order_cannot_claim_match(self):
        self.receipt["sequence_order_ui"].update(
            observed_labels=[],
            stable_polls=0,
            result="inconclusive",
            matches_fixture_expected=False,
        )
        with self.assertRaises(ValueError):
            self.run_validation()

    def test_invalid_order_label_is_rejected(self):
        self.receipt["sequence_order_ui"]["observed_labels"] = [
            "00: 00",
            "order one",
            "02: 01",
            "03: 02",
        ]
        with self.assertRaises(ValueError):
            self.run_validation()


class MatrixSequenceOrder(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        shutil.copytree(ROOT / "phase6c", self.root / "phase6c")
        (self.root / "scripts").mkdir()
        for name in ("phase6c-validate-matrix.py", "phase6c-candidate-parse.py"):
            shutil.copyfile(ROOT / "scripts" / name, self.root / "scripts" / name)
        self.evidence = self.root / "phase6c/evidence/sequencer-pattern-order"

    def check(self, success):
        result = subprocess.run(
            ["python3", "scripts/phase6c-validate-matrix.py"],
            cwd=self.root,
            capture_output=True,
            text=True,
        )
        self.assertEqual(
            result.returncode == 0,
            success,
            result.stdout + result.stderr,
        )

    def mutate(self, name, change):
        path = self.evidence / name
        data = json.loads(path.read_text())
        change(data)
        path.write_text(json.dumps(data))

    def test_versioned_sequence_order_pass_validates(self):
        self.check(True)

    def test_original_observed_order_is_frozen(self):
        self.mutate(
            "original-sequence-order.json",
            lambda d: d["sequence_order_ui"].update(
                observed_labels=["00: 00", "01: 01", "02: 02", "03: 01"]
            ),
        )
        self.check(False)

    def test_original_order_requires_stability(self):
        self.mutate(
            "original-sequence-order.json",
            lambda d: d["sequence_order_ui"].update(stable_polls=3),
        )
        self.check(False)

    def test_candidate_play_order_is_frozen(self):
        self.mutate(
            "candidate-sequence-order.json",
            lambda d: d.update(observed_play_order=[0, 1, 2, 1]),
        )
        self.check(False)

    def test_candidate_master_line_is_frozen(self):
        self.mutate(
            "candidate-sequence-order.json",
            lambda d: d["observed_sequence_lines"][0]["entries"][0].update(
                pattern_id=0
            ),
        )
        self.check(False)

    def test_comparison_must_bind_both_orders(self):
        self.mutate(
            "comparison.json",
            lambda d: d.update(candidate_observed_play_order=[0, 1, 2, 1]),
        )
        self.check(False)

    def test_source_workflow_attribution_is_frozen(self):
        self.mutate(
            "comparison.json",
            lambda d: d["source_evidence"].update(workflow_run_id=1),
        )
        self.check(False)

    def test_pass_cannot_be_relabelled_different(self):
        matrix_path = self.root / "phase6c/compatibility-matrix.json"
        matrix = json.loads(matrix_path.read_text())
        next(
            row
            for row in matrix["contracts"]
            if row["id"] == "sequencer-pattern-order"
        )["status"] = "DIFFERENT"
        matrix_path.write_text(json.dumps(matrix))
        self.mutate("comparison.json", lambda d: d.update(verdict="DIFFERENT"))
        self.check(False)


if __name__ == "__main__":
    unittest.main()
