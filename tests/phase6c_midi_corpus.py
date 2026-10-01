#!/usr/bin/env python3
"""Behavioral/static contracts for the Phase 6C real-world MIDI corpus lane."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import struct
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / "scripts/phase6c-midi-corpus.py"
MANIFEST = ROOT / "phase6c/reference-corpus/manifest.json"
PROBE = ROOT / "tests/phase6c_midi_corpus_probe.c"
WORKFLOW = ROOT / ".github/workflows/phase6c-midi-corpus-private.yml"
LEGACY_WORKFLOW = ROOT / ".github/workflows/phase6c-legacy-playback-import.yml"
PSYCONF = ROOT / "cpsycle/detail/psyconf.h"

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
assert {
    item["name"]: item["sha256"] for item in manifest["sets"]
} == {
    "Blue Glare": "b1297c44d138d71bbcfddaa4278e1b1fd4e5aac2c6a559276ab124f2b44db67f",
    "Celestial Mechanics": "01596654e86bb8fe3b36d2b8ea30c5760521133b18fb27742c9d2f8d7f95fafd",
    "Deterministic Pattern": "cfde1bcb71935718fc4a7e8e16b6aa77ceac44d8fb31d8dcadfee87a5b178b32",
    "Polyrhythmic Patterns": "5e7a3a17569661f6817ab664411e5ca6d44b22c50306d4a82df0a0b984f2f6ce",
    "NGC3603 Quantum Demoscene": "9fa0905ed0758dd23f61b144e22bf85150358c95f3a4268803a44351720af8c8",
    "FM Doom": "c78d8addbc8e6e95d2232c1505d5e668c5ab7c6a9f987d5f56005a52d5de5674",
}
assert {
    item["name"]: item["stems"] for item in manifest["sets"]
} == {
    "Blue Glare": 11,
    "Celestial Mechanics": 10,
    "Deterministic Pattern": 8,
    "Polyrhythmic Patterns": 9,
    "NGC3603 Quantum Demoscene": 12,
    "FM Doom": 12,
}
assert {
    item["name"]: item["analysis_expectations"]["tempo_events_per_stem"]
    for item in manifest["sets"]
} == {
    "Blue Glare": 539,
    "Celestial Mechanics": 449,
    "Deterministic Pattern": 319,
    "Polyrhythmic Patterns": 425,
    "NGC3603 Quantum Demoscene": 501,
    "FM Doom": 379,
}

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

def boundary_smf(reverse_tracks: bool) -> bytes:
    old_note = bytearray()
    old_note += helper.midi_varlen(0) + bytes((0x90, 60, 100))
    old_note += helper.midi_varlen(480) + bytes((0x80, 60, 0))
    old_note += helper.midi_varlen(0) + b"\xff\x2f\x00"

    new_note = bytearray()
    new_note += helper.midi_varlen(480) + bytes((0x90, 60, 100))
    new_note += helper.midi_varlen(480) + bytes((0x80, 60, 0))
    new_note += helper.midi_varlen(0) + b"\xff\x2f\x00"

    tracks = [bytes(old_note), bytes(new_note)]
    if reverse_tracks:
        tracks.reverse()
    payload = bytearray(b"MThd")
    payload += struct.pack(">IHHH", 6, 1, 2, 480)
    for track in tracks:
        payload += b"MTrk" + struct.pack(">I", len(track)) + track
    return bytes(payload)

boundary_forward = helper.parse_smf_bytes(
    boundary_smf(False), label="boundary-forward"
)
boundary_reversed = helper.parse_smf_bytes(
    boundary_smf(True), label="boundary-reversed"
)
for field in (
    "same_note_overlaps",
    "max_polyphony",
    "zero_duration_pairs",
    "balanced_note_pairs",
):
    assert boundary_forward[field] == boundary_reversed[field], field
assert boundary_forward["same_note_overlaps"] == 0
assert boundary_forward["max_polyphony"] == 1
assert boundary_forward["zero_duration_pairs"] == 0
assert boundary_forward["balanced_note_pairs"] is True

same_track = bytearray()
same_track += helper.midi_varlen(0) + bytes((0x90, 60, 100))
same_track += helper.midi_varlen(10) + bytes((0x90, 60, 100))
same_track += helper.midi_varlen(0) + bytes((0x80, 60, 0))
same_track += helper.midi_varlen(10) + bytes((0x80, 60, 0))
same_track += helper.midi_varlen(0) + b"\xff\x2f\x00"
same_track_smf = bytearray(b"MThd")
same_track_smf += struct.pack(">IHHH", 6, 1, 1, 480)
same_track_smf += b"MTrk" + struct.pack(">I", len(same_track)) + same_track
same_track_parsed = helper.parse_smf_bytes(
    bytes(same_track_smf), label="same-track-shared-boundary"
)
assert same_track_parsed["same_note_overlaps"] == 1
assert same_track_parsed["max_polyphony"] == 2
assert same_track_parsed["zero_duration_pairs"] == 0
assert same_track_parsed["balanced_note_pairs"] is True

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
    embedded_midi = public / "private.mid"
    helper.write_synthetic_fixture(embedded_midi)
    ordinary_zip = public / "ordinary.zip"
    with zipfile.ZipFile(ordinary_zip, "w", compression=zipfile.ZIP_STORED) as archive:
        archive.write(embedded_midi, arcname="private.mid")
    zip_bytes = ordinary_zip.read_bytes()
    embedded_midi.unlink()
    ordinary_zip.unlink()

    disguised_zip = public / "evidence.txt"
    disguised_zip.write_bytes(b"SFX-PREFIX" + zip_bytes)
    assert zipfile.is_zipfile(disguised_zip)
    try:
        helper.audit_public_tree(public)
    except SystemExit as exc:
        assert "raw corpus bytes" in str(exc)
    else:
        raise AssertionError("prefixed renamed ZIP payload must fail public-tree audit")

psyconf_source = PSYCONF.read_text(encoding="utf-8")
assert "#define PSYCLE_USE_MIDI_FILE" in psyconf_source
assert "/* #define PSYCLE_USE_MIDI_FILE */" not in psyconf_source

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

def aggregate_for(set_spec):
    expected = set_spec["analysis_expectations"]
    malformed = expected.get("malformed_key_signature")
    return {
        "stems": set_spec["stems"],
        "tracks": expected.get("tracks", 1),
        "notes": expected["notes"],
        "same_note_overlaps": expected["same_note_overlaps"],
        "max_polyphony": expected["max_polyphony"],
        "tempo_events_total": expected["tempo_events_per_stem"] * set_spec["stems"],
        "tempo_events_per_stem": [expected["tempo_events_per_stem"]],
        "tempo_min_bpm": expected["tempo_range_bpm"][0],
        "tempo_max_bpm": expected["tempo_range_bpm"][1],
        "duration_seconds_max": expected["duration_seconds_max"],
        "channels": expected.get("channels_used", []),
        "malformed_key_signatures": [] if malformed is None else [malformed],
        "zero_duration_pairs": 1 if expected["zero_duration_pairs_present"] else 0,
        "sysex_events": 0,
        "pitch_bend_events": 0,
        "aftertouch_events": 0,
        "time_signature_events": 0,
        "balanced_note_pairs": True,
        "formats": [1],
        "divisions": [480],
    }


def canonical_analysis(root: Path):
    sets = []
    manifest_by_name = {item["name"]: item for item in manifest["sets"]}
    for name in helper.PROGRESSION:
        set_spec = manifest_by_name[name]
        set_slug = helper.slug(name)
        stems = []
        for index in range(set_spec["stems"]):
            private_path = f"private/{set_slug}/{index:02d}.mid"
            path = root / private_path
            path.parent.mkdir(parents=True, exist_ok=True)
            payload = f"{name}:{index}".encode()
            path.write_bytes(payload)
            stem_sha = hashlib.sha256(payload).hexdigest()
            stems.append({
                "stem_id": f"{set_slug}-{index:02d}-{stem_sha[:12]}",
                "set_name": name,
                "set_slug": set_slug,
                "archive": set_spec["archive"],
                "archive_sha256": set_spec["sha256"],
                "relative_name": f"{index:02d}.mid",
                "private_path": private_path,
                "sha256": stem_sha,
                "size_bytes": len(payload),
                "analysis": {},
            })
        rep = stems[0]
        sets.append({
            "name": name,
            "slug": set_slug,
            "archive": set_spec["archive"],
            "archive_sha256": set_spec["sha256"],
            "role": set_spec["role"],
            "aggregate": aggregate_for(set_spec),
            "representative": {
                "stem_id": rep["stem_id"],
                "relative_name": rep["relative_name"],
                "private_path": rep["private_path"],
                "sha256": rep["sha256"],
                "size_bytes": rep["size_bytes"],
            },
            "stems": stems,
        })
    return {
        "schema_version": 1,
        "phase": "6C",
        "contract": helper.CONTRACT,
        "evidence_role": "private-input-analysis",
        "progression_order": helper.PROGRESSION,
        "sets": sets,
        "parity_status": "UNKNOWN",
    }


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    analysis = canonical_analysis(root)
    helper.validate_analysis_manifest_bindings(analysis)
    bad_analysis = json.loads(json.dumps(analysis))
    bad_analysis["sets"][0]["archive_sha256"] = "0" * 64
    try:
        helper.validate_analysis_manifest_bindings(bad_analysis)
    except SystemExit as exc:
        assert "manifest-pinned archive" in str(exc)
    else:
        raise AssertionError("bogus corpus archive identity must be rejected")

    analysis_path = root / "corpus-analysis.json"
    helper.write_json(analysis_path, analysis)
    helper.bind_analysis_root(analysis, analysis_path)
    outer_bundle = root / "fake-corpus-bundle.zip"
    with zipfile.ZipFile(outer_bundle, "w", compression=zipfile.ZIP_STORED) as outer:
        for set_info in analysis["sets"]:
            inner_bytes_path = root / f"{set_info['slug']}.zip"
            with zipfile.ZipFile(
                inner_bytes_path, "w", compression=zipfile.ZIP_STORED
            ) as inner:
                for stem in set_info["stems"]:
                    inner.write(
                        root / stem["private_path"],
                        arcname=stem["relative_name"],
                    )
            outer.write(
                inner_bytes_path,
                arcname=set_info["archive"],
            )
            inner_bytes_path.unlink()
    try:
        helper.validate_analysis_against_bundle(
            analysis, outer_bundle, representatives_only=True
        )
    except SystemExit as exc:
        assert "archive SHA-256 mismatch" in str(exc)
    else:
        raise AssertionError(
            "self-declared stems must not pass without the pinned archive bytes"
        )

with tempfile.TemporaryDirectory() as temporary:
    stub = Path(temporary) / "candidate-psycle-player"
    stub.write_text("#!/bin/sh\nexit 2\n", encoding="utf-8")
    stub.chmod(0o755)
    try:
        helper.validate_frozen_candidate_player(stub)
    except SystemExit as exc:
        assert "canonical Phase 6B build output" in str(exc)
    else:
        raise AssertionError("arbitrary candidate player stub must be rejected")

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
assert "donor-summary             phase6c-midi-private/corpus-analysis.json             \"$MIDI_CORPUS_BUNDLE\"" in workflow
assert "candidate-boundary             phase6c-midi-private/corpus-analysis.json             \"$MIDI_CORPUS_BUNDLE\"             \"$PLAYER\"" in workflow
assert "cp -- \"$PLAYER\" phase6c-midi-candidate-public/candidate-psycle-player" in workflow
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

# Public summary validation must require complete per-set donor evidence and
# manifest-bound candidate archive identities.
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
    candidate_player.write_bytes(b"\x7fELF" + b"candidate-player")

    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    manifest_by_name = {item["name"]: item for item in manifest["sets"]}
    donor_sets = []
    candidate_sets = []
    for name in helper.PROGRESSION:
        set_spec = manifest_by_name[name]
        stem_count = set_spec["stems"]
        stem_records = []
        for index in range(stem_count):
            source_sha = hashlib.sha256(
                f"{name}:{index}".encode()
            ).hexdigest()
            stem_records.append({
                "stem_id": (
                    f"{helper.slug(name)}-{index:02d}-{source_sha[:12]}"
                ),
                "source_relative_name": f"{index:02d}.mid",
                "source_sha256": source_sha,
                "source_size_bytes": 32 + index,
                "source_analysis_sha256": hashlib.sha256(
                    f"analysis:{name}:{index}".encode()
                ).hexdigest(),
                "import_event_digest_fnv64": (
                    f"{index + 1:016x}"[-16:]
                ),
                "render_sha256": hashlib.sha256(
                    f"render:{name}:{index}".encode()
                ).hexdigest(),
            })
        donor_sets.append({
            "name": name,
            "slug": helper.slug(name),
            "role": set_spec["role"],
            "archive": set_spec["archive"],
            "archive_sha256": set_spec["sha256"],
            "source_aggregate": aggregate_for(set_spec),
            "observed_stems": stem_count,
            "all_imported": True,
            "all_non_silent_projection": True,
            "stems": stem_records,
        })
        candidate_sets.append({
            "set_name": name,
            "archive": set_spec["archive"],
            "archive_sha256": set_spec["sha256"],
            "representative_stem_id": f"{helper.slug(name)}-00-deadbeef0000",
            "representative_relative_name": "00.mid",
            "representative_size_bytes": 32,
            "representative_sha256": "b" * 64,
            "raw_output_sha256": "c" * 64,
            "direct_midi_load": "rejected",
            "exit_code": 2,
            "diagnostic_could_not_load_song_file": True,
        })

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
        "sets": donor_sets,
        "all_sets_imported": True,
        "all_sets_non_silent_projection": True,
        "parity_status": "UNKNOWN",
    }
    candidate = {
        "schema_version": 1,
        "phase": "6C",
        "contract": helper.CANDIDATE_CONTRACT,
        "evidence_role": "candidate-observation",
        "candidate_baseline_sha256": helper.EXPECTED_CANDIDATE_BASELINE,
        "source_revision": helper.EXPECTED_CANDIDATE_SOURCE_REVISION,
        "repository_commit": "0" * 40,
        "build_target": "psycle-player",
        "player": candidate_player.name,
        "player_sha256": sha(candidate_player),
        "private_input_required": True,
        "progression_order": helper.PROGRESSION,
        "sets": candidate_sets,
        "parity_status": "UNKNOWN",
    }
    donor_path = donor_dir / "donor-midi-corpus.json"
    candidate_path = candidate_dir / "candidate-midi-boundary.json"
    helper.write_json(donor_path, donor)
    helper.write_json(candidate_path, candidate)
    helper.corpus_summary(donor_path, candidate_path, root / "summary.json")
    assert (root / "summary.json").exists()

    incomplete_donor = json.loads(json.dumps(donor))
    del incomplete_donor["sets"][0]["observed_stems"]
    helper.write_json(donor_path, incomplete_donor)
    try:
        helper.corpus_summary(donor_path, candidate_path, root / "bad-donor.json")
    except SystemExit as exc:
        assert "per-set evidence is incomplete" in str(exc)
    else:
        raise AssertionError("incomplete donor per-set evidence must be rejected")

    duplicate_donor = json.loads(json.dumps(donor))
    first_record = duplicate_donor["sets"][0]["stems"][0]
    duplicate_donor["sets"][0]["stems"] = [
        dict(first_record)
        for _ in range(duplicate_donor["sets"][0]["observed_stems"])
    ]
    helper.write_json(donor_path, duplicate_donor)
    try:
        helper.corpus_summary(
            donor_path, candidate_path, root / "duplicate-donor.json"
        )
    except SystemExit as exc:
        assert "stem evidence is duplicated" in str(exc)
    else:
        raise AssertionError(
            "one donor observation copied across N stems must be rejected"
        )

    helper.write_json(donor_path, donor)
    bad_candidate = json.loads(json.dumps(candidate))
    bad_candidate["sets"][0]["archive_sha256"] = "0" * 64
    helper.write_json(candidate_path, bad_candidate)
    try:
        helper.corpus_summary(
            donor_path,
            candidate_path,
            root / "bad-candidate.json",
        )
    except SystemExit as exc:
        assert "direct-load boundary is incomplete" in str(exc)
    else:
        raise AssertionError("wrong candidate archive identity must be rejected")

print("phase6c-midi-corpus: PASS")
