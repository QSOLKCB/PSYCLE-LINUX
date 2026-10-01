#!/usr/bin/env python3
"""Behavioral/static contracts for the Phase 6C real-world MIDI corpus lane."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / "scripts/phase6c-midi-corpus.py"
MANIFEST = ROOT / "phase6c/reference-corpus/manifest.json"
PROBE = ROOT / "tests/phase6c_midi_corpus_probe.c"
WORKFLOW = ROOT / ".github/workflows/phase6c-midi-corpus-private.yml"
LEGACY_WORKFLOW = ROOT / ".github/workflows/phase6c-legacy-playback-import.yml"

spec = importlib.util.spec_from_file_location("phase6c_midi_corpus_helper_test", HELPER)
helper = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(helper)

manifest = helper.corpus_manifest()
assert helper.MANIFEST.resolve() == MANIFEST.resolve()
assert manifest["schema_version"] == 2
assert manifest["contract"] == "legacy-midi-real-world-corpus"
assert manifest["progression_order"] == helper.PROGRESSION
assert [item["name"] for item in manifest["sets"]] != helper.PROGRESSION
assert set(item["name"] for item in manifest["sets"]) == set(helper.PROGRESSION)
assert manifest["private_workflow"]["bundle_url_secret"] == "PSYCLE_PHASE6C_MIDI_CORPUS_URL"
assert manifest["shared_observation"]["ppqn"] == 480
assert "deterministic Sampulse" in manifest["shared_observation"]["playback_boundary"]

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    midi = root / "synthetic.mid"
    helper.write_synthetic_fixture(midi)
    parsed = helper.parse_smf(midi)
    assert parsed["format"] == 1
    assert parsed["tracks"] == 2
    assert parsed["division"] == 480
    assert parsed["note_ons"] == 2
    assert parsed["note_offs"] == 2
    assert parsed["same_note_overlaps"] == 0
    assert parsed["max_polyphony"] == 2
    assert parsed["tempo_events"] == 1
    assert parsed["channels"] == [0, 1]
    assert parsed["malformed_key_signatures"] == [(9, 1)]
    assert parsed["balanced_note_pairs"] is True
    assert parsed["zero_duration_pairs"] == 0
    assert parsed["sysex_events"] == 0
    assert parsed["pitch_bend_events"] == 0
    assert parsed["aftertouch_events"] == 0
    assert parsed["time_signature_events"] == 0
    assert abs(parsed["duration_seconds"] - 0.5) < 1e-9

with tempfile.TemporaryDirectory() as temporary:
    public = Path(temporary)
    disguised_midi = public / "renamed-midi.log"
    helper.write_synthetic_fixture(disguised_midi)
    try:
        helper.audit_public_tree(public)
    except SystemExit as exc:
        assert "raw corpus bytes" in str(exc)
    else:
        raise AssertionError("renamed MThd payload must fail public-tree audit")

with tempfile.TemporaryDirectory() as temporary:
    public = Path(temporary)
    disguised_zip = public / "renamed-archive.txt"
    disguised_zip.write_bytes(b"PK\x03\x04" + b"synthetic zip marker")
    try:
        helper.audit_public_tree(public)
    except SystemExit as exc:
        assert "raw corpus bytes" in str(exc)
    else:
        raise AssertionError("renamed ZIP payload must fail public-tree audit")

probe_source = PROBE.read_text(encoding="utf-8")
assert '"\\\"phase\\\":\\\"6C\\\","' in probe_source
assert '"\\\"contract\\\":\\\"legacy-midi-real-world-donor\\\","' in probe_source
assert "UINT64_C(14695981039346656037)" in probe_source
assert "PROJECTION_BEATS 16.0" in probe_source
assert "psy_audio_XMSAMPLER" in probe_source
assert "psy_audio_create_fileout_driver" in probe_source
assert "machines_before_projection" in probe_source
assert "import_event_digest_fnv64" in probe_source
assert "non_silent_projection" in probe_source

# The sanitized C++ candidate has no retained Standard MIDI File loader entry point.
candidate_core = ROOT / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core"
candidate_mthd_hits = []
for path in candidate_core.rglob("*"):
    if not path.is_file() or path.suffix.lower() not in {".c", ".cc", ".cpp", ".h", ".hpp"}:
        continue
    text = path.read_text(encoding="utf-8", errors="ignore")
    if "MThd" in text or "MidiLoader" in text or "midiloader" in text:
        candidate_mthd_hits.append(path.relative_to(ROOT).as_posix())
assert candidate_mthd_hits == []

assert WORKFLOW.exists()
workflow = WORKFLOW.read_text(encoding="utf-8")
assert "workflow_dispatch:" in workflow
assert "PSYCLE_PHASE6C_MIDI_CORPUS_URL" in workflow
assert "prepare-bundle" in workflow
assert "tests/phase6c_midi_corpus_probe.c" in workflow
assert "donor-summary" in workflow
assert "candidate-boundary" in workflow
assert "audit-public" in workflow
assert "phase6c-midi-real-world-corpus-summary" in workflow
assert "original_psycle_1_12_0_x86" not in workflow
for upload_block in workflow.split("uses: actions/upload-artifact@v4")[1:]:
    block = upload_block.split("\n      - name:", 1)[0]
    assert ".mid" not in block.lower(), block
    assert ".midi" not in block.lower(), block
    assert ".zip" not in block.lower(), block

legacy_workflow = LEGACY_WORKFLOW.read_text(encoding="utf-8")
for path in (
    "scripts/phase6c-midi-corpus.py",
    "tests/phase6c_midi_corpus.py",
    "tests/phase6c_midi_corpus_probe.c",
    ".github/workflows/phase6c-midi-corpus-private.yml",
):
    assert legacy_workflow.count(f"      - '{path}'\n") == 2, path
assert "python3 tests/phase6c_midi_corpus.py" in legacy_workflow
assert "Build synthetic MIDI corpus execution probe" in legacy_workflow
assert "Prove synthetic SMF import and non-silent projection" in legacy_workflow
assert "tests/phase6c_midi_corpus_probe.c" in legacy_workflow
assert "phase6c-midi-synthetic.mid" in legacy_workflow
assert 'assert observation["sequence_tracks"] == 2' in legacy_workflow
assert 'assert observation["machines_before_projection"] == 0' in legacy_workflow
assert 'assert observation["projection_notes"] == 2' in legacy_workflow
assert 'assert observation["non_silent_projection"] is True' in legacy_workflow
assert (
    "phase6c-generated/phase6c-midi-synthetic.mid \\\n"
    "            phase6c-generated/phase6c-midi-synthetic.wav"
) in legacy_workflow
for upload_block in legacy_workflow.split("uses: actions/upload-artifact@v4")[1:]:
    block = upload_block.split("\n      - name:", 1)[0]
    assert "phase6c-midi-synthetic.mid" not in block
    assert "phase6c-midi-synthetic.wav" not in block

# Summary validation must reject malformed candidate progression evidence.
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    donor_dir = root / "donor"
    candidate_dir = root / "candidate"
    donor_dir.mkdir()
    candidate_dir.mkdir()
    donor_source = donor_dir / "midiloader.c"
    donor_probe = donor_dir / "phase6c-midi-corpus-probe"
    candidate_player = candidate_dir / "candidate-psycle-player"
    donor_source.write_text("source", encoding="utf-8")
    donor_probe.write_text("probe", encoding="utf-8")
    candidate_player.write_text("player", encoding="utf-8")

    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    donor = {
        "schema_version": 1,
        "phase": "6C",
        "contract": helper.DONOR_CONTRACT,
        "evidence_role": "cpsycle-donor-corpus-summary",
        "source_file": donor_source.name,
        "source_file_sha256": sha(donor_source),
        "probe_executable": donor_probe.name,
        "probe_executable_sha256": sha(donor_probe),
        "progression_order": helper.PROGRESSION,
        "sets": [{"name": name} for name in helper.PROGRESSION],
        "all_sets_imported": True,
        "all_sets_non_silent_projection": True,
        "parity_status": "UNKNOWN",
    }
    candidate = {
        "schema_version": 1,
        "phase": "6C",
        "contract": helper.CANDIDATE_CONTRACT,
        "evidence_role": "candidate-observation",
        "player": candidate_player.name,
        "player_sha256": sha(candidate_player),
        "private_input_required": True,
        "progression_order": helper.PROGRESSION,
        "sets": [
            {
                "set_name": name,
                "representative_sha256": "a" * 64,
                "raw_output_sha256": "b" * 64,
                "direct_midi_load": "rejected",
                "exit_code": 2,
                "diagnostic_could_not_load_song_file": True,
            }
            for name in helper.PROGRESSION
        ],
        "parity_status": "UNKNOWN",
    }
    donor_path = donor_dir / "donor-midi-corpus.json"
    candidate_path = candidate_dir / "candidate-midi-boundary.json"
    helper.write_json(donor_path, donor)
    helper.write_json(candidate_path, candidate)

    bad_candidate = dict(candidate)
    bad_candidate["sets"] = list(candidate["sets"])
    bad_candidate["sets"][0] = dict(bad_candidate["sets"][0])
    bad_candidate["sets"][0]["direct_midi_load"] = "inconclusive"
    helper.write_json(candidate_path, bad_candidate)
    try:
        helper.corpus_summary(
            donor_path,
            candidate_path,
            root / "summary.json",
        )
    except SystemExit as exc:
        assert "direct-load boundary" in str(exc)
    else:
        raise AssertionError("incomplete candidate MIDI boundary must be rejected")

print("phase6c-midi-corpus: PASS")
