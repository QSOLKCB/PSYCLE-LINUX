#!/usr/bin/env python3
"""Negative controls for Phase 6C delayed/retrigger observations."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "phase6c_delayed_retrigger",
    ROOT / "scripts/phase6c-delayed-retrigger-evidence.py",
)
m = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(m)


def capture(offset, track, note):
    return {
        "offset": offset,
        "track": track,
        "note": note,
        "command": 0,
        "parameter": 0,
    }


def valid_value():
    beat, tick = m.expected_time()
    return {
        "schema_version": 1,
        "load_returned": True,
        "song_name": m.TITLE,
        "bpm": 137.0,
        "tick_speed": 8,
        "is_ticks": True,
        "sample_rate": 44100,
        "samples_per_beat": beat,
        "samples_per_tick": tick,
        "loaded_parameters": dict(m.LOADED_PARAMETERS),
        "marker_position_after_extended": 0.25,
        "note_delay_events": [
            capture(m.LOADED_PARAMETERS["note_delay"] / 256.0, 0, 48)
        ],
        "retrigger_events": [
            capture(offset, 1, 50) for offset in m.expected_retrigger_offsets()
        ],
        "retr_cont_events": [
            capture(offset, 2, 52) for offset in m.expected_retr_cont_offsets()
        ],
        "reports": ["Load Warning: " + m.WARNING],
    }


def clean_log():
    return (
        "log: 0us: T: thread-id-123: # universalis # ../src/universalis/os/thread_name.cpp:56 # void universalis::os::thread_name::set_tls()\n"
        "log: 1us: T: thread-id-123: setting name for thread: id: 123, name: delayed-retrigger-probe\n"
        "log: 2us: T: delayed-retrigger-probe: # psycle-core # ../src/psycle/core/player.cpp:66 # void psycle::core::Player::start_threads()\n"
        "log: 3us: T: delayed-retrigger-probe: psycle: core: player: starting scheduler threads\n"
        "log: 4us: I: delayed-retrigger-probe: psycle: core: player: using 1 threads\n"
        "log: 5us: T: delayed-retrigger-probe: psycle: core: psy3 loader: loading psycle song fileformat version 3: /tmp/" + m.FIXTURE + "\n"
        "log: 6us: W: delayed-retrigger-probe: " + m.WARNING + "\n"
        "log: 7us: I: delayed-retrigger-probe: psycle: core: machine factory: create machine: loading with host: 0, plugin: <sampler>\n"
        "log: 8us: I: delayed-retrigger-probe: psycle: core: machine factory: create machine: loading with host: 0, plugin: <master>\n"
        "log: 9us: T: thread-id-123: # psycle-core # ../src/psycle/core/player.cpp:452 # void psycle::core::Player::stop_threads()\n"
        "log: 10us: T: thread-id-123: terminating and joining scheduler threads ...\n"
        "log: 11us: T: thread-id-123: # psycle-core # ../src/psycle/core/player.cpp:454 # void psycle::core::Player::stop_threads()\n"
        "log: 12us: T: thread-id-123: scheduler threads were not running\n"
    ).encode()


class CandidateParser(unittest.TestCase):
    def parse(self, value=None, log=None, exit_code=0):
        if value is None:
            value = valid_value()
        return m.parse_probe(
            json.dumps(value).encode(),
            clean_log() if log is None else log,
            exit_code,
        )

    def test_clean_scheduling_observation(self):
        result = self.parse()
        self.assertEqual(result["observation"], "command-scheduling-observed")
        self.assertEqual(result["loaded_parameters"]["note_delay"], 15)
        self.assertEqual(result["marker_position_after_extended"], 0.25)

    def test_note_delay_load_conversion_is_frozen(self):
        value = valid_value()
        value["loaded_parameters"]["note_delay"] = 127
        self.assertEqual(self.parse(value)["observation"], "inconclusive")

    def test_note_delay_scheduled_offset_is_rederived(self):
        value = valid_value()
        value["note_delay_events"][0]["offset"] += 0.001
        self.assertEqual(self.parse(value)["observation"], "inconclusive")

    def test_retrigger_series_is_rederived(self):
        value = valid_value()
        value["retrigger_events"].pop()
        self.assertEqual(self.parse(value)["observation"], "inconclusive")

    def test_scheduled_tracks_are_bound_to_fixture_lanes(self):
        for field, expected_track in (
            ("note_delay_events", 0),
            ("retrigger_events", 1),
            ("retr_cont_events", 2),
        ):
            with self.subTest(field=field):
                value = valid_value()
                value[field][0]["track"] = expected_track + 99
                self.assertEqual(self.parse(value)["observation"], "inconclusive")

    def test_retr_cont_series_is_rederived(self):
        value = valid_value()
        value["retr_cont_events"][1]["offset"] += 0.001
        self.assertEqual(self.parse(value)["observation"], "inconclusive")

    def test_extended_lpb_changes_loaded_marker_geometry(self):
        value = valid_value()
        value["marker_position_after_extended"] = 0.125
        self.assertEqual(self.parse(value)["observation"], "inconclusive")

    def test_unknown_diagnostic_is_rejected(self):
        contaminated = clean_log() + (
            b"log: 13us: W: delayed-retrigger-probe: unexpected warning\n"
        )
        self.assertEqual(
            self.parse(log=contaminated)["observation"], "inconclusive"
        )

    def test_abnormal_exit_is_inconclusive(self):
        for code in (1, 65, 70, 124, None, False):
            with self.subTest(code=code):
                self.assertEqual(
                    self.parse(exit_code=code)["observation"], "inconclusive"
                )

    def test_artifact_path_cannot_escape(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for value in ("../escape", "/tmp/escape", "a\\b"):
                with self.subTest(value=value), self.assertRaises(ValueError):
                    m.child(root, value)


class OriginalSourceContract(unittest.TestCase):
    def test_git_blob_is_sensitive_to_source_bytes(self):
        self.assertNotEqual(m.git_blob(b"a"), m.git_blob(b"b"))

    def test_source_receipt_identity_is_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            receipt = {
                "schema_version": 1,
                "phase": "6C",
                "scope": "original-source-observation",
                "contract": m.CONTRACT,
                "evidence_role": "original-source",
                "source_repository": "jpaquim/psycle",
                "source_commit": m.SOURCE_COMMIT,
                "command_ids": {
                    "note_delay": 0xFD,
                    "retrigger": 0xFB,
                    "retr_cont": 0xFA,
                    "extended": 0xFE,
                    "set_lpb_range": [0x00, 0x1F],
                },
                "semantics": {
                    "note_delay_counter": "((parameter+1)*SamplesPerRow())/256",
                    "retrigger_rate": "parameter+1",
                    "retr_cont_rate_override": "parameter high nibble when nonzero",
                    "extended_lpb": "FE00..FE1F calls SetBPM(-1, parameter)",
                },
                "files": {
                    "psycle/src/psycle/host/Player.cpp": {
                        "git_blob": m.PLAYER_BLOB,
                    },
                    "psycle/src/psycle/host/SongStructs.hpp": {
                        "git_blob": m.SONGSTRUCTS_BLOB,
                    },
                },
                "original_psycle_executed": False,
                "parity_status": "UNKNOWN",
            }
            (root / "original-source-delayed-retrigger.json").write_text(
                json.dumps(receipt)
            )
            self.assertEqual(
                m.validate_source(root)["source_commit"], m.SOURCE_COMMIT
            )
            receipt["files"]["psycle/src/psycle/host/Player.cpp"]["sha256"] = "0" * 64
            (root / "original-source-delayed-retrigger.json").write_text(
                json.dumps(receipt)
            )
            with self.assertRaises(ValueError):
                m.validate_source(root)
            receipt["files"]["psycle/src/psycle/host/Player.cpp"].pop("sha256")
            receipt["files"]["psycle/src/psycle/host/Player.cpp"]["git_blob"] = "0" * 40
            (root / "original-source-delayed-retrigger.json").write_text(
                json.dumps(receipt)
            )
            with self.assertRaises(ValueError):
                m.validate_source(root)
            receipt["files"]["psycle/src/psycle/host/Player.cpp"]["git_blob"] = m.PLAYER_BLOB
            receipt["command_ids"]["note_delay"] = 0
            (root / "original-source-delayed-retrigger.json").write_text(
                json.dumps(receipt)
            )
            with self.assertRaises(ValueError):
                m.validate_source(root)


class MatrixDelayedRetriggerComparison(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        shutil.copytree(ROOT / "phase6c", self.root / "phase6c")
        (self.root / "scripts").mkdir()
        for name in ("phase6c-validate-matrix.py", "phase6c-candidate-parse.py"):
            shutil.copyfile(ROOT / "scripts" / name, self.root / "scripts" / name)
        self.evidence = (
            self.root / "phase6c/evidence/sequencer-delayed-retrigger"
        )

    def check(self, success):
        result = subprocess.run(
            ["python3", "scripts/phase6c-validate-matrix.py"],
            cwd=self.root,
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(
            result.returncode == 0,
            success,
            result.stdout + result.stderr,
        )

    def mutate(self, name, change):
        path = self.evidence / name
        value = json.loads(path.read_text())
        change(value)
        path.write_text(json.dumps(value))

    def delayed_row(self):
        path = self.root / "phase6c/compatibility-matrix.json"
        matrix = json.loads(path.read_text())
        row = next(
            item
            for item in matrix["contracts"]
            if item["id"] == "sequencer-delayed-retrigger"
        )
        return path, matrix, row

    def test_versioned_deferred_comparison_validates(self):
        self.check(True)

    def test_original_runtime_trace_cannot_be_invented(self):
        self.mutate(
            "original-delayed-retrigger.json",
            lambda d: d.update(runtime_execution_trace="observed"),
        )
        self.check(False)

    def test_source_inspection_cannot_be_relabelled_execution(self):
        self.mutate(
            "original-source-delayed-retrigger.json",
            lambda d: d.update(original_psycle_executed=True),
        )
        self.check(False)

    def test_candidate_schedule_is_frozen(self):
        self.mutate(
            "candidate-delayed-retrigger.json",
            lambda d: d["note_delay_events"][0].update(track=99),
        )
        self.check(False)

    def test_final_run_attribution_is_frozen(self):
        self.mutate(
            "comparison.json",
            lambda d: d["source_evidence"].update(workflow_run_id=1),
        )
        self.check(False)

    def test_original_liveness_projection_is_frozen(self):
        self.mutate(
            "original-delayed-retrigger.json",
            lambda d: d.update(process_running_before_termination=False),
        )
        self.check(False)

    def test_candidate_timing_context_is_frozen(self):
        self.mutate(
            "candidate-delayed-retrigger.json",
            lambda d: d.update(tick_speed=24),
        )
        self.check(False)

    def test_original_source_artifact_digest_is_frozen(self):
        self.mutate(
            "original-source-delayed-retrigger.json",
            lambda d: d["source_evidence"].update(artifact_digest="sha256:" + "0" * 64),
        )
        self.check(False)

    def test_projection_procedure_digest_is_frozen(self):
        self.mutate(
            "candidate-delayed-retrigger.json",
            lambda d: d["source_evidence"].update(procedure_sha256="0" * 64),
        )
        self.check(False)

    def test_deferred_verdict_cannot_be_relabelled_pass(self):
        self.mutate(
            "comparison.json",
            lambda d: d.update(verdict="PASS", classification_allowed=True),
        )
        self.check(False)

    def test_matrix_status_cannot_promote_without_new_observer(self):
        path, matrix, row = self.delayed_row()
        row["status"] = "PASS"
        path.write_text(json.dumps(matrix))
        self.check(False)


class GeneratedObserver(unittest.TestCase):
    def generate(self, source: Path, output: Path):
        return subprocess.run(
            [
                "python3",
                str(ROOT / "scripts/phase6c-build-delayed-retrigger-observer.py"),
                str(source),
                str(output),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )

    def test_generated_observer_adds_only_explicit_delayed_fixture(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "generated.ps1"
            process = self.generate(
                ROOT / "scripts/phase6c-original-windows-fixtures-v2.ps1",
                output,
            )
            self.assertEqual(process.returncode, 0, process.stderr)
            text = output.read_text(encoding="utf-8")
            self.assertIn("[switch]$ObserveDelayedRetrigger", text)
            self.assertIn('name = "delayed-retrigger"', text)
            self.assertIn(
                'expected_contract = "sequencer-delayed-retrigger"', text
            )
            self.assertIn(
                'if (-not $process.HasExited -and [bool]$renderOne.dialog_closed) {',
                text,
            )

    def test_builder_accepts_crlf_checkout_of_pinned_base(self):
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            source = temp_path / "base.ps1"
            output = temp_path / "generated.ps1"
            canonical = (
                ROOT / "scripts/phase6c-original-windows-fixtures-v2.ps1"
            ).read_text(encoding="utf-8")
            source.write_bytes(canonical.replace("\n", "\r\n").encode())
            process = self.generate(source, output)
            self.assertEqual(process.returncode, 0, process.stderr)


if __name__ == "__main__":
    unittest.main()
