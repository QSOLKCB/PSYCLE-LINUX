#!/usr/bin/env python3
"""Private-input helpers for the Phase 6C real-world MIDI corpus lane."""
from __future__ import annotations

import argparse
import collections
import hashlib
import io
import json
import math
import pathlib
import shutil
import struct
import subprocess
import zipfile
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "phase6c/reference-corpus/manifest.json"
CONTRACT = "legacy-midi-real-world-corpus"
DONOR_CONTRACT = "legacy-midi-real-world-donor"
CANDIDATE_CONTRACT = "legacy-midi-real-world-candidate-boundary"
SUMMARY_CONTRACT = "legacy-midi-real-world-summary"
EXPECTED_CANDIDATE_BASELINE = (
    "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
)
EXPECTED_CANDIDATE_SOURCE_REVISION = "SourceForge SVN r12005"
CANDIDATE_SOURCE_ROOT = ROOT / "psycle-cpp-r12005-sanitized"
CANDIDATE_PLAYER = (
    CANDIDATE_SOURCE_ROOT / "psycle-player/++qmake/psycle-player"
)
CANDIDATE_BASELINE_RECEIPT = (
    ROOT / "phase6b-sanitized-manifest/baseline.sha256"
)

PROGRESSION = [
    "FM Doom",
    "Celestial Mechanics",
    "Deterministic Pattern",
    "Blue Glare",
    "Polyrhythmic Patterns",
    "NGC3603 Quantum Demoscene",
]


def die(message: str) -> "NoReturn":
    raise SystemExit(f"phase6c-midi-corpus: {message}")


def load_json(path: pathlib.Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        die(f"cannot read JSON {path}: {exc}")
    if not isinstance(data, dict):
        die(f"JSON root must be an object: {path}")
    return data


def write_json(path: pathlib.Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def slug(name: str) -> str:
    out = []
    dash = False
    for ch in name.lower():
        if ch.isalnum():
            out.append(ch)
            dash = False
        elif not dash:
            out.append("-")
            dash = True
    return "".join(out).strip("-")


def corpus_manifest() -> dict[str, Any]:
    manifest = load_json(MANIFEST)
    if manifest.get("schema_version") != 2:
        die("MIDI corpus manifest schema_version must be 2")
    if manifest.get("contract") != CONTRACT:
        die("MIDI corpus manifest contract changed")
    sets = manifest.get("sets")
    if not isinstance(sets, list) or len(sets) != 6:
        die("MIDI corpus manifest must contain exactly six sets")
    names = [item.get("name") for item in sets if isinstance(item, dict)]
    if sorted(names) != sorted(PROGRESSION):
        die("MIDI corpus manifest set identities changed")
    if manifest.get("progression_order") != PROGRESSION:
        die("MIDI corpus progression order changed")
    return manifest


def read_varlen(data: bytes, pos: int) -> tuple[int, int]:
    value = 0
    for _ in range(5):
        if pos >= len(data):
            die("truncated MIDI variable-length value")
        byte = data[pos]
        pos += 1
        value = (value << 7) | (byte & 0x7F)
        if not (byte & 0x80):
            return value, pos
    die("MIDI variable-length value exceeds five bytes")


def parse_track(track: bytes) -> dict[str, Any]:
    pos = 0
    tick = 0
    running_status: int | None = None
    note_ons = 0
    note_offs = 0
    unmatched_note_offs = 0
    same_note_overlaps = 0
    zero_duration_pairs = 0
    active_total = 0
    max_polyphony = 0
    active: dict[tuple[int, int], collections.deque[int]] = collections.defaultdict(collections.deque)
    tempos: list[tuple[int, int]] = []
    key_signatures: list[tuple[int, int]] = []
    channels: set[int] = set()
    sysex = 0
    pitch_bend = 0
    aftertouch = 0
    time_signatures = 0
    end_tick = 0
    note_transitions: list[tuple[int, int, int, int, bool]] = []
    event_serial = 0

    while pos < len(track):
        delta, pos = read_varlen(track, pos)
        tick += delta
        end_tick = max(end_tick, tick)
        if pos >= len(track):
            die("truncated MIDI event after delta time")

        first = track[pos]
        explicit_status = first >= 0x80
        if explicit_status:
            status = first
            pos += 1
            if 0x80 <= status <= 0xEF:
                running_status = status
            elif status in (0xF0, 0xF7):
                running_status = None
        else:
            if running_status is None:
                die("MIDI data byte encountered without running status")
            status = running_status

        if status == 0xFF:
            if pos >= len(track):
                die("truncated MIDI meta event")
            meta_type = track[pos]
            pos += 1
            length, pos = read_varlen(track, pos)
            end = pos + length
            if end > len(track):
                die("truncated MIDI meta payload")
            payload = track[pos:end]
            pos = end
            if meta_type == 0x2F:
                break
            if meta_type == 0x51 and len(payload) == 3:
                usec = int.from_bytes(payload, "big")
                if usec:
                    tempos.append((tick, usec))
            elif meta_type == 0x59 and len(payload) >= 2:
                sf = struct.unpack("b", payload[:1])[0]
                mi = payload[1]
                key_signatures.append((sf, mi))
            elif meta_type == 0x58:
                time_signatures += 1
            continue

        if status in (0xF0, 0xF7):
            length, pos = read_varlen(track, pos)
            end = pos + length
            if end > len(track):
                die("truncated MIDI SysEx payload")
            pos = end
            sysex += 1
            continue

        if not (0x80 <= status <= 0xEF):
            die(f"unsupported MIDI system status 0x{status:02x}")

        kind = status & 0xF0
        channel = status & 0x0F
        channels.add(channel)
        data_len = 1 if kind in (0xC0, 0xD0) else 2
        if explicit_status:
            if pos + data_len > len(track):
                die("truncated MIDI channel event")
            payload = track[pos:pos + data_len]
            pos += data_len
        else:
            if pos + data_len > len(track):
                die("truncated MIDI running-status event")
            payload = track[pos:pos + data_len]
            pos += data_len

        if kind in (0xA0, 0xD0):
            aftertouch += 1
        elif kind == 0xE0:
            pitch_bend += 1

        is_note_on = kind == 0x90 and len(payload) == 2 and payload[1] != 0
        is_note_off = kind == 0x80 or (kind == 0x90 and len(payload) == 2 and payload[1] == 0)
        if is_note_on:
            note = payload[0]
            note_transitions.append((tick, event_serial, channel, note, True))
            key = (channel, note)
            queue = active[key]
            if queue:
                same_note_overlaps += 1
            queue.append(tick)
            active_total += 1
            max_polyphony = max(max_polyphony, active_total)
            note_ons += 1
        elif is_note_off:
            note = payload[0]
            note_transitions.append((tick, event_serial, channel, note, False))
            key = (channel, note)
            queue = active[key]
            note_offs += 1
            if not queue:
                unmatched_note_offs += 1
            else:
                start_tick = queue.popleft()
                if start_tick == tick:
                    zero_duration_pairs += 1
                active_total -= 1
        event_serial += 1

    remaining = sum(len(queue) for queue in active.values())
    return {
        "note_ons": note_ons,
        "note_offs": note_offs,
        "same_note_overlaps": same_note_overlaps,
        "zero_duration_pairs": zero_duration_pairs,
        "max_polyphony": max_polyphony,
        "tempo_events": tempos,
        "key_signatures": key_signatures,
        "channels": sorted(channels),
        "sysex_events": sysex,
        "pitch_bend_events": pitch_bend,
        "aftertouch_events": aftertouch,
        "time_signature_events": time_signatures,
        "unmatched_note_offs": unmatched_note_offs,
        "remaining_active_notes": remaining,
        "end_tick": end_tick,
        "note_transitions": note_transitions,
    }


def duration_seconds(end_tick: int, division: int, tempos: list[tuple[int, int]]) -> float:
    by_tick: dict[int, int] = {}
    for tick, usec in tempos:
        by_tick[tick] = usec
    if 0 not in by_tick:
        by_tick[0] = 500000
    elapsed = 0.0
    current_tick = 0
    current_usec = by_tick[0]
    for tick in sorted(k for k in by_tick if 0 < k <= end_tick):
        elapsed += (tick - current_tick) * current_usec / (division * 1_000_000.0)
        current_tick = tick
        current_usec = by_tick[tick]
    elapsed += (end_tick - current_tick) * current_usec / (division * 1_000_000.0)
    return elapsed


def parse_smf_bytes(data: bytes, *, label: str = "<bytes>") -> dict[str, Any]:
    if len(data) < 14 or data[:4] != b"MThd":
        die(f"{label}: missing MThd header")
    header_len = int.from_bytes(data[4:8], "big")
    if header_len < 6 or 8 + header_len > len(data):
        die(f"{label}: invalid MThd length")
    fmt, ntracks, division = struct.unpack(">HHH", data[8:14])
    if division & 0x8000:
        die(f"{label}: SMPTE division is outside this corpus contract")

    pos = 8 + header_len
    tracks: list[dict[str, Any]] = []
    for index in range(ntracks):
        if pos + 8 > len(data) or data[pos:pos + 4] != b"MTrk":
            die(f"{label}: missing MTrk chunk {index}")
        length = int.from_bytes(data[pos + 4:pos + 8], "big")
        pos += 8
        end = pos + length
        if end > len(data):
            die(f"{label}: truncated MTrk chunk {index}")
        tracks.append(parse_track(data[pos:end]))
        pos = end

    tempos = [item for track in tracks for item in track["tempo_events"]]
    bpms = [60_000_000.0 / usec for _, usec in tempos if usec]

    ordered_transitions: list[
        tuple[int, int, int, int, int, int, bool]
    ] = []
    for track_index, track in enumerate(tracks):
        by_tick: dict[
            int, list[tuple[int, int, int, bool]]
        ] = collections.defaultdict(list)
        for tick, serial, channel, note, is_on in track["note_transitions"]:
            by_tick[tick].append((serial, channel, note, is_on))
        for tick, events in by_tick.items():
            events.sort()
            first_attack = next(
                (
                    index for index, (_serial, _channel, _note, is_on)
                    in enumerate(events)
                    if is_on
                ),
                len(events),
            )
            for index, (serial, channel, note, is_on) in enumerate(events):
                # Leading releases on a track occur before cross-track attacks
                # at the same tick. Once a track attacks, its remaining note
                # events keep strict same-track serial order.
                phase = index - first_attack
                kind_priority = 1 if is_on else 0
                ordered_transitions.append(
                    (
                        tick,
                        phase,
                        kind_priority,
                        channel,
                        note,
                        track_index,
                        is_on,
                    )
                )

    ordered_transitions.sort()
    active_notes: dict[
        tuple[int, int], collections.deque[int]
    ] = collections.defaultdict(collections.deque)
    active_total = 0
    global_max_polyphony = 0
    global_same_note_overlaps = 0
    global_zero_duration_pairs = 0
    global_unmatched_note_offs = 0
    for tick, _phase, _kind, channel, note, _track_index, is_on in ordered_transitions:
        key = (channel, note)
        queue = active_notes[key]
        if is_on:
            if queue:
                global_same_note_overlaps += 1
            queue.append(tick)
            active_total += 1
            global_max_polyphony = max(global_max_polyphony, active_total)
        elif not queue:
            global_unmatched_note_offs += 1
        else:
            start_tick = queue.popleft()
            if start_tick == tick:
                global_zero_duration_pairs += 1
            active_total -= 1

    global_remaining = sum(len(queue) for queue in active_notes.values())
    key_sigs = [pair for track in tracks for pair in track["key_signatures"]]
    channels = sorted({ch for track in tracks for ch in track["channels"]})
    end_tick = max((track["end_tick"] for track in tracks), default=0)
    return {
        "format": fmt,
        "tracks": ntracks,
        "division": division,
        "note_ons": sum(track["note_ons"] for track in tracks),
        "note_offs": sum(track["note_offs"] for track in tracks),
        "same_note_overlaps": global_same_note_overlaps,
        "zero_duration_pairs": global_zero_duration_pairs,
        "max_polyphony": global_max_polyphony,
        "tempo_events": len(tempos),
        "tempo_min_bpm": min(bpms) if bpms else None,
        "tempo_max_bpm": max(bpms) if bpms else None,
        "key_signatures": sorted(set(key_sigs)),
        "malformed_key_signatures": sorted(
            set((sf, mi) for sf, mi in key_sigs if sf < -7 or sf > 7 or mi not in (0, 1))
        ),
        "channels": channels,
        "sysex_events": sum(track["sysex_events"] for track in tracks),
        "pitch_bend_events": sum(track["pitch_bend_events"] for track in tracks),
        "aftertouch_events": sum(track["aftertouch_events"] for track in tracks),
        "time_signature_events": sum(track["time_signature_events"] for track in tracks),
        "unmatched_note_offs": global_unmatched_note_offs,
        "remaining_active_notes": global_remaining,
        "balanced_note_pairs": (
            global_unmatched_note_offs == 0 and global_remaining == 0
        ),
        "duration_seconds": duration_seconds(end_tick, division, tempos),
    }


def parse_smf(path: pathlib.Path) -> dict[str, Any]:
    return parse_smf_bytes(path.read_bytes(), label=path.name)


def safe_zip_member(name: str) -> pathlib.PurePosixPath:
    pure = pathlib.PurePosixPath(name.replace("\\", "/"))
    if pure.is_absolute() or ".." in pure.parts or not pure.name:
        die(f"unsafe ZIP member path: {name!r}")
    return pure


def find_outer_archive(bundle: zipfile.ZipFile, expected_name: str) -> zipfile.ZipInfo:
    matches = [
        info for info in bundle.infolist()
        if not info.is_dir() and pathlib.PurePosixPath(info.filename).name == expected_name
    ]
    if len(matches) != 1:
        die(f"outer corpus bundle must contain exactly one {expected_name!r}")
    return matches[0]


def validate_aggregate_against_manifest(
    set_spec: dict[str, Any],
    aggregate: dict[str, Any],
) -> None:
    expected = set_spec.get("analysis_expectations")
    if not isinstance(expected, dict):
        die(f"{set_spec['name']}: missing analysis_expectations")

    exact_fields = {
        "stems": "stems",
        "tracks": "tracks",
        "notes": "notes",
        "same_note_overlaps": "same_note_overlaps",
        "max_polyphony": "max_polyphony",
    }
    for actual_key, expected_key in exact_fields.items():
        expected_value = expected.get(expected_key)
        if expected_value is None:
            continue
        if aggregate.get(actual_key) != expected_value:
            die(
                f"{set_spec['name']}: {actual_key} mismatch: "
                f"expected={expected_value} actual={aggregate.get(actual_key)}"
            )

    expected_tempo_per_stem = expected.get("tempo_events_per_stem")
    if (
        expected_tempo_per_stem is None
        or aggregate.get("tempo_events_per_stem") != [expected_tempo_per_stem]
    ):
        die(
            f"{set_spec['name']}: per-stem tempo-map count changed: "
            f"expected={expected_tempo_per_stem} "
            f"actual={aggregate.get('tempo_events_per_stem')}"
        )

    if aggregate.get("formats") != [1] or aggregate.get("divisions") != [480]:
        die(f"{set_spec['name']}: corpus files are not uniformly SMF1/480 PPQN")
    if aggregate.get("balanced_note_pairs") is not True:
        die(f"{set_spec['name']}: unbalanced note pairs detected")
    for field in (
        "sysex_events",
        "pitch_bend_events",
        "aftertouch_events",
        "time_signature_events",
    ):
        if aggregate.get(field) != 0:
            die(f"{set_spec['name']}: unexpected {field}={aggregate.get(field)}")

    tempo_range = expected.get("tempo_range_bpm")
    if not (
        isinstance(tempo_range, list)
        and len(tempo_range) == 2
        and isinstance(aggregate.get("tempo_min_bpm"), (int, float))
        and isinstance(aggregate.get("tempo_max_bpm"), (int, float))
        and math.isclose(
            aggregate["tempo_min_bpm"], float(tempo_range[0]), abs_tol=0.01
        )
        and math.isclose(
            aggregate["tempo_max_bpm"], float(tempo_range[1]), abs_tol=0.01
        )
    ):
        die(
            f"{set_spec['name']}: tempo range changed: "
            f"expected={tempo_range} actual="
            f"{[aggregate.get('tempo_min_bpm'), aggregate.get('tempo_max_bpm')]}"
        )

    duration = aggregate.get("duration_seconds_max")
    if not isinstance(duration, (int, float)) or not math.isclose(
        duration,
        float(expected.get("duration_seconds_max")),
        abs_tol=0.02,
    ):
        die(
            f"{set_spec['name']}: duration changed: "
            f"expected={expected.get('duration_seconds_max')} actual={duration}"
        )

    expected_key = expected.get("malformed_key_signature")
    expected_keys = [] if expected_key is None else [tuple(expected_key)]
    actual_keys = [
        tuple(pair) for pair in aggregate.get("malformed_key_signatures", [])
    ]
    if actual_keys != expected_keys:
        die(
            f"{set_spec['name']}: malformed key-signature observation changed: "
            f"expected={expected_keys} actual={actual_keys}"
        )

    expected_zero = bool(expected.get("zero_duration_pairs_present"))
    zero_pairs = aggregate.get("zero_duration_pairs")
    if not isinstance(zero_pairs, int) or (zero_pairs > 0) != expected_zero:
        die(f"{set_spec['name']}: zero-duration-pair observation changed")

    expected_channels = expected.get("channels_used")
    if (
        expected_channels is not None
        and aggregate.get("channels") != expected_channels
    ):
        die(
            f"{set_spec['name']}: channel-use observation changed: "
            f"expected={expected_channels} actual={aggregate.get('channels')}"
        )


def validate_set_aggregate(
    set_spec: dict[str, Any],
    stems: list[dict[str, Any]],
) -> dict[str, Any]:
    aggregate = {
        "stems": len(stems),
        "tracks": sum(stem["analysis"]["tracks"] for stem in stems),
        "notes": sum(stem["analysis"]["note_ons"] for stem in stems),
        "same_note_overlaps": sum(stem["analysis"]["same_note_overlaps"] for stem in stems),
        "max_polyphony": max((stem["analysis"]["max_polyphony"] for stem in stems), default=0),
        "tempo_events_total": sum(stem["analysis"]["tempo_events"] for stem in stems),
        "tempo_events_per_stem": sorted(
            {stem["analysis"]["tempo_events"] for stem in stems}
        ),
        "tempo_min_bpm": min(
            stem["analysis"]["tempo_min_bpm"]
            for stem in stems if stem["analysis"]["tempo_min_bpm"] is not None
        ),
        "tempo_max_bpm": max(
            stem["analysis"]["tempo_max_bpm"]
            for stem in stems if stem["analysis"]["tempo_max_bpm"] is not None
        ),
        "duration_seconds_max": max(stem["analysis"]["duration_seconds"] for stem in stems),
        "channels": sorted({ch for stem in stems for ch in stem["analysis"]["channels"]}),
        "malformed_key_signatures": sorted({
            tuple(pair)
            for stem in stems
            for pair in stem["analysis"]["malformed_key_signatures"]
        }),
        "zero_duration_pairs": sum(stem["analysis"]["zero_duration_pairs"] for stem in stems),
        "sysex_events": sum(stem["analysis"]["sysex_events"] for stem in stems),
        "pitch_bend_events": sum(stem["analysis"]["pitch_bend_events"] for stem in stems),
        "aftertouch_events": sum(stem["analysis"]["aftertouch_events"] for stem in stems),
        "time_signature_events": sum(stem["analysis"]["time_signature_events"] for stem in stems),
        "balanced_note_pairs": all(stem["analysis"]["balanced_note_pairs"] for stem in stems),
        "formats": sorted({stem["analysis"]["format"] for stem in stems}),
        "divisions": sorted({stem["analysis"]["division"] for stem in stems}),
    }

    validate_aggregate_against_manifest(set_spec, aggregate)
    return aggregate


def prepare_bundle(bundle_path: pathlib.Path, root: pathlib.Path) -> dict[str, Any]:
    manifest = corpus_manifest()
    if not bundle_path.is_file():
        die(f"private MIDI corpus bundle is missing: {bundle_path}")
    private_root = root / "private"
    private_root.mkdir(parents=True, exist_ok=True)
    public_sets: list[dict[str, Any]] = []

    with zipfile.ZipFile(bundle_path, "r") as outer:
        for set_spec in manifest["sets"]:
            archive_info = find_outer_archive(outer, set_spec["archive"])
            archive_bytes = outer.read(archive_info)
            archive_sha = sha256_bytes(archive_bytes)
            if archive_sha != set_spec["sha256"]:
                die(
                    f"{set_spec['name']}: archive SHA-256 mismatch: "
                    f"expected={set_spec['sha256']} actual={archive_sha}"
                )
            set_slug = slug(set_spec["name"])
            set_root = private_root / set_slug
            set_root.mkdir(parents=True, exist_ok=True)
            stems: list[dict[str, Any]] = []
            with zipfile.ZipFile(io.BytesIO(archive_bytes), "r") as inner:
                midi_infos = [
                    info for info in inner.infolist()
                    if not info.is_dir()
                    and pathlib.PurePosixPath(info.filename).suffix.lower() in {".mid", ".midi"}
                ]
                midi_infos.sort(key=lambda info: info.filename.lower())
                if len(midi_infos) != set_spec["stems"]:
                    die(
                        f"{set_spec['name']}: stem count mismatch: "
                        f"expected={set_spec['stems']} actual={len(midi_infos)}"
                    )
                for index, info in enumerate(midi_infos):
                    rel = safe_zip_member(info.filename)
                    data = inner.read(info)
                    stem_sha = sha256_bytes(data)
                    analysis = parse_smf_bytes(data, label=f"{set_spec['name']}/{rel}")
                    stem_id = f"{set_slug}-{index:02d}-{stem_sha[:12]}"
                    destination = set_root / f"{index:02d}-{rel.name}"
                    destination.write_bytes(data)
                    stems.append({
                        "stem_id": stem_id,
                        "set_name": set_spec["name"],
                        "set_slug": set_slug,
                        "archive": set_spec["archive"],
                        "archive_sha256": archive_sha,
                        "relative_name": rel.as_posix(),
                        "private_path": destination.relative_to(root).as_posix(),
                        "sha256": stem_sha,
                        "size_bytes": len(data),
                        "analysis": analysis,
                    })
            aggregate = validate_set_aggregate(set_spec, stems)
            representative = stems[0]
            public_sets.append({
                "name": set_spec["name"],
                "slug": set_slug,
                "archive": set_spec["archive"],
                "archive_sha256": archive_sha,
                "role": set_spec["role"],
                "aggregate": aggregate,
                "representative": {
                    "stem_id": representative["stem_id"],
                    "relative_name": representative["relative_name"],
                    "private_path": representative["private_path"],
                    "sha256": representative["sha256"],
                    "size_bytes": representative["size_bytes"],
                },
                "stems": stems,
            })

    by_name = {item["name"]: item for item in public_sets}
    ordered = [by_name[name] for name in PROGRESSION]
    analysis_doc = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "evidence_role": "private-input-analysis",
        "progression_order": PROGRESSION,
        "sets": ordered,
        "parity_status": "UNKNOWN",
    }
    write_json(root / "corpus-analysis.json", analysis_doc)
    return analysis_doc


def validate_analysis_manifest_bindings(
    analysis: dict[str, Any],
) -> dict[str, dict[str, Any]]:
    manifest = corpus_manifest()
    manifest_by_name = {item["name"]: item for item in manifest["sets"]}
    sets = analysis.get("sets")
    if (
        analysis.get("schema_version") != 1
        or analysis.get("phase") != "6C"
        or analysis.get("contract") != CONTRACT
        or analysis.get("evidence_role") != "private-input-analysis"
        or analysis.get("progression_order") != PROGRESSION
        or analysis.get("parity_status") != "UNKNOWN"
        or not isinstance(sets, list)
        or len(sets) != len(PROGRESSION)
        or [item.get("name") for item in sets] != PROGRESSION
    ):
        die("MIDI corpus analysis is not the canonical private-input analysis")

    for set_info in sets:
        name = set_info["name"]
        set_spec = manifest_by_name[name]
        stems = set_info.get("stems")
        representative = set_info.get("representative")
        if (
            set_info.get("slug") != slug(name)
            or set_info.get("archive") != set_spec["archive"]
            or set_info.get("archive_sha256") != set_spec["sha256"]
            or set_info.get("role") != set_spec["role"]
            or not isinstance(stems, list)
            or len(stems) != set_spec["stems"]
            or not isinstance(representative, dict)
        ):
            die(f"{name}: analysis is not bound to the manifest-pinned archive")

        for stem in stems:
            if (
                stem.get("set_name") != name
                or stem.get("set_slug") != slug(name)
                or stem.get("archive") != set_spec["archive"]
                or stem.get("archive_sha256") != set_spec["sha256"]
                or not isinstance(stem.get("sha256"), str)
                or len(stem["sha256"]) != 64
                or not isinstance(stem.get("size_bytes"), int)
                or stem["size_bytes"] <= 0
                or not isinstance(stem.get("analysis"), dict)
            ):
                die(f"{name}: stem analysis is not archive-bound")

        matches = [
            stem for stem in stems
            if (
                stem.get("stem_id") == representative.get("stem_id")
                and stem.get("relative_name") == representative.get("relative_name")
                and stem.get("private_path") == representative.get("private_path")
                and stem.get("sha256") == representative.get("sha256")
                and stem.get("size_bytes") == representative.get("size_bytes")
            )
        ]
        if len(matches) != 1:
            die(f"{name}: representative is not one canonical analysed stem")
        validate_aggregate_against_manifest(set_spec, set_info.get("aggregate", {}))
    return manifest_by_name


def audit_public_tree(root: pathlib.Path) -> None:
    leaks: list[str] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        if path.suffix.lower() in {".mid", ".midi", ".zip"}:
            leaks.append(relative)
            continue
        with path.open("rb") as handle:
            prefix = handle.read(4)
        if prefix == b"MThd":
            leaks.append(relative)
            continue
        if prefix == b"PK\x03\x04" or zipfile.is_zipfile(path):
            leaks.append(relative)
    if leaks:
        die("public MIDI evidence contains raw corpus bytes: " + ", ".join(sorted(leaks)))


def verified_archive_members(
    outer: zipfile.ZipFile,
    set_spec: dict[str, Any],
) -> dict[str, bytes]:
    archive_info = find_outer_archive(outer, set_spec["archive"])
    archive_bytes = outer.read(archive_info)
    archive_sha = sha256_bytes(archive_bytes)
    if archive_sha != set_spec["sha256"]:
        die(
            f"{set_spec['name']}: private archive SHA-256 mismatch during observation: "
            f"expected={set_spec['sha256']} actual={archive_sha}"
        )
    members: dict[str, bytes] = {}
    with zipfile.ZipFile(io.BytesIO(archive_bytes), "r") as inner:
        for info in inner.infolist():
            if info.is_dir():
                continue
            rel = safe_zip_member(info.filename).as_posix()
            if rel in members:
                die(f"{set_spec['name']}: duplicate archive member {rel!r}")
            members[rel] = inner.read(info)
    return members


def validate_analysis_against_bundle(
    analysis: dict[str, Any],
    bundle_path: pathlib.Path,
    *,
    representatives_only: bool,
) -> dict[str, dict[str, Any]]:
    manifest_by_name = validate_analysis_manifest_bindings(analysis)
    if not bundle_path.is_file() or not zipfile.is_zipfile(bundle_path):
        die("private MIDI corpus outer bundle is missing or invalid")
    with zipfile.ZipFile(bundle_path, "r") as outer:
        for set_info in analysis["sets"]:
            name = set_info["name"]
            set_spec = manifest_by_name[name]
            members = verified_archive_members(outer, set_spec)
            stems = (
                [set_info["representative"]]
                if representatives_only
                else set_info["stems"]
            )
            for stem in stems:
                relative_name = stem.get("relative_name")
                if relative_name not in members:
                    die(
                        f"{name}: analysed stem {relative_name!r} is not present "
                        "in the manifest-pinned archive"
                    )
                archive_bytes = members[relative_name]
                archive_sha = sha256_bytes(archive_bytes)
                if (
                    archive_sha != stem.get("sha256")
                    or len(archive_bytes) != stem.get("size_bytes")
                ):
                    die(
                        f"{name}: analysed stem identity differs from its "
                        "manifest-pinned archive member"
                    )
                private_path = analysis_path_for_private_stem(
                    analysis, stem
                )
                if (
                    not private_path.is_file()
                    or private_path.read_bytes() != archive_bytes
                ):
                    die(
                        f"{name}: extracted private stem differs from the "
                        "verified archive member"
                    )
    return manifest_by_name


def analysis_path_for_private_stem(
    analysis: dict[str, Any],
    stem: dict[str, Any],
) -> pathlib.Path:
    root_value = analysis.get("_analysis_root")
    if not isinstance(root_value, str):
        die("private corpus analysis root is not bound")
    root = pathlib.Path(root_value)
    relative = pathlib.PurePosixPath(stem.get("private_path", ""))
    if relative.is_absolute() or ".." in relative.parts:
        die("private stem path escapes the corpus analysis root")
    return root / pathlib.Path(*relative.parts)


def bind_analysis_root(
    analysis: dict[str, Any],
    analysis_path: pathlib.Path,
) -> None:
    analysis["_analysis_root"] = str(analysis_path.parent.resolve())


def validate_frozen_candidate_player(player: pathlib.Path) -> dict[str, str]:
    expected_player = CANDIDATE_PLAYER.resolve()
    if player.resolve() != expected_player:
        die(
            "candidate player must be the canonical Phase 6B build output: "
            f"{expected_player}"
        )
    if not CANDIDATE_BASELINE_RECEIPT.is_file():
        die("Phase 6B sanitized baseline receipt is missing")
    receipt = CANDIDATE_BASELINE_RECEIPT.read_text(
        encoding="utf-8"
    ).strip()
    if receipt != EXPECTED_CANDIDATE_BASELINE:
        die("Phase 6B sanitized baseline receipt identity changed")
    if not player.is_file() or not os.access(player, os.X_OK):
        die("canonical candidate player is missing or not executable")
    with player.open("rb") as handle:
        if handle.read(4) != b"\x7fELF":
            die("canonical candidate player is not an ELF executable")
    diff = subprocess.run(
        [
            "git", "-C", str(ROOT), "diff", "--quiet", "HEAD", "--",
            str(CANDIDATE_SOURCE_ROOT.relative_to(ROOT)),
        ],
        check=False,
    )
    if diff.returncode != 0:
        die("tracked frozen candidate source differs from HEAD")
    repository_commit = subprocess.check_output(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True
    ).strip()
    return {
        "candidate_baseline_sha256": EXPECTED_CANDIDATE_BASELINE,
        "source_revision": EXPECTED_CANDIDATE_SOURCE_REVISION,
        "repository_commit": repository_commit,
        "build_target": "psycle-player",
    }


def candidate_boundary(
    analysis_path: pathlib.Path,
    bundle_path: pathlib.Path,
    player: pathlib.Path,
    output: pathlib.Path,
) -> None:
    analysis = load_json(analysis_path)
    bind_analysis_root(analysis, analysis_path)
    manifest_by_name = validate_analysis_against_bundle(
        analysis, bundle_path, representatives_only=True
    )
    candidate_identity = validate_frozen_candidate_player(player)
    player_sha = sha256_file(player)
    results = []
    for set_info in analysis["sets"]:
        rep = set_info["representative"]
        private_path = analysis_path.parent / rep["private_path"]
        if sha256_file(private_path) != rep["sha256"]:
            die(f"{set_info['name']}: representative private MIDI changed")
        replay_root = analysis_path.parent / "candidate-replay" / set_info["slug"]
        if replay_root.exists():
            shutil.rmtree(replay_root)
        replay_root.mkdir(parents=True)
        staged_player = replay_root / "candidate-psycle-player"
        staged_midi = replay_root / "representative.mid"
        shutil.copyfile(player, staged_player)
        shutil.copyfile(private_path, staged_midi)
        staged_player.chmod(0o755)
        proc = subprocess.run(
            [
                "./candidate-psycle-player",
                "--output-driver",
                "dummy",
                "--input-file",
                "representative.mid",
            ],
            cwd=replay_root,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        raw = proc.stdout
        diagnostic = b"could not load song file:" in raw
        direct = "rejected" if proc.returncode == 2 and diagnostic else "inconclusive"
        set_spec = manifest_by_name[set_info["name"]]
        results.append({
            "set_name": set_info["name"],
            "archive": set_spec["archive"],
            "archive_sha256": set_spec["sha256"],
            "representative_stem_id": rep["stem_id"],
            "representative_relative_name": rep["relative_name"],
            "representative_size_bytes": rep["size_bytes"],
            "representative_sha256": rep["sha256"],
            "exit_code": proc.returncode,
            "diagnostic_could_not_load_song_file": diagnostic,
            "raw_output_sha256": sha256_bytes(raw),
            "direct_midi_load": direct,
        })
        shutil.rmtree(replay_root)

    if any(item["direct_midi_load"] != "rejected" for item in results):
        die("frozen candidate real-world MIDI boundary was not a scoped direct-load rejection")
    receipt = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CANDIDATE_CONTRACT,
        "evidence_role": "candidate-observation",
        **candidate_identity,
        "player": "candidate-psycle-player",
        "player_sha256": player_sha,
        "working_directory": "artifact-root-with-private-input",
        "procedure": (
            "for each corpus set, place the exact representative SMF beside the "
            "artifact candidate player under the private replay root, chmod +x the "
            "player, invoke --output-driver dummy --input-file representative.mid "
            "with stdin=/dev/null, retain only the sanitized result/hash, then delete "
            "the private replay root"
        ),
        "private_input_required": True,
        "progression_order": PROGRESSION,
        "sets": results,
        "parity_status": "UNKNOWN",
        "claim_scope": (
            "Frozen C++ candidate representative SMF direct-load boundary only; "
            "absence of a direct loader is not a claim about internal MIDI event support."
        ),
    }
    write_json(output, receipt)


def donor_summary(
    analysis_path: pathlib.Path,
    bundle_path: pathlib.Path,
    observations_root: pathlib.Path,
    probe_path: pathlib.Path,
    source_path: pathlib.Path,
    output: pathlib.Path,
) -> None:
    analysis = load_json(analysis_path)
    bind_analysis_root(analysis, analysis_path)
    manifest_by_name = validate_analysis_against_bundle(
        analysis, bundle_path, representatives_only=False
    )
    set_summaries = []
    for set_info in analysis["sets"]:
        observations = []
        for stem in set_info["stems"]:
            path = observations_root / f"{stem['stem_id']}.json"
            obs = load_json(path)
            if obs.get("schema_version") != 1 or obs.get("phase") != "6C":
                die(f"{stem['stem_id']}: donor observation schema/phase changed")
            if obs.get("contract") != DONOR_CONTRACT:
                die(f"{stem['stem_id']}: donor observation contract changed")
            if obs.get("evidence_role") != "cpsycle-donor-observation":
                die(f"{stem['stem_id']}: donor evidence_role changed")
            if obs.get("source_sha256") != stem["sha256"]:
                die(f"{stem['stem_id']}: donor source identity changed")
            if obs.get("source_size_bytes") != stem["size_bytes"]:
                die(f"{stem['stem_id']}: donor source size changed")
            expected_analysis_sha = sha256_bytes(
                json.dumps(
                    stem["analysis"], sort_keys=True, separators=(",", ":")
                ).encode()
            )
            if obs.get("source_analysis_sha256") != expected_analysis_sha:
                die(f"{stem['stem_id']}: donor raw-SMF analysis binding changed")
            if obs.get("load_result") != "accepted":
                die(f"{stem['stem_id']}: donor load_result is not accepted")
            if obs.get("machines_before_projection") != 0:
                die(f"{stem['stem_id']}: MIDI importer unexpectedly created machines")
            if not isinstance(obs.get("projection_notes"), int) or obs["projection_notes"] <= 0:
                die(f"{stem['stem_id']}: donor projection contains no notes")
            if not isinstance(obs.get("render_frames"), int) or obs["render_frames"] <= 0:
                die(f"{stem['stem_id']}: donor projection rendered no frames")
            if not isinstance(obs.get("render_peak"), int) or obs["render_peak"] <= 0:
                die(f"{stem['stem_id']}: donor projection peak is invalid")
            if obs.get("private_input_required") is not True:
                die(f"{stem['stem_id']}: donor private-input flag changed")
            if obs.get("fixture_redistributed") is not False:
                die(f"{stem['stem_id']}: donor redistribution flag changed")
            digest = obs.get("import_event_digest_fnv64")
            if not isinstance(digest, str) or len(digest) != 16:
                die(f"{stem['stem_id']}: donor event digest is invalid")
            try:
                int(digest, 16)
            except ValueError:
                die(f"{stem['stem_id']}: donor event digest is not hexadecimal")
            render_sha = obs.get("render_sha256")
            if not isinstance(render_sha, str) or len(render_sha) != 64:
                die(f"{stem['stem_id']}: donor render SHA-256 is invalid")
            if obs.get("imported_notes") != stem["analysis"]["note_ons"]:
                die(
                    f"{stem['stem_id']}: imported note count differs from raw SMF: "
                    f"source={stem['analysis']['note_ons']} imported={obs.get('imported_notes')}"
                )
            if obs.get("non_silent_projection") is not True:
                die(f"{stem['stem_id']}: execution projection is silent")
            if obs.get("projection_kind") != "deterministic-sampulse":
                die(f"{stem['stem_id']}: execution projection kind changed")
            if obs.get("parity_status") != "UNKNOWN":
                die(f"{stem['stem_id']}: donor observation attempted parity promotion")
            observations.append(obs)
        set_spec = manifest_by_name[set_info["name"]]
        stem_records = [
            {
                "stem_id": item["stem_id"],
                "source_relative_name": item["source_relative_name"],
                "source_sha256": item["source_sha256"],
                "source_size_bytes": item["source_size_bytes"],
                "source_analysis_sha256": item["source_analysis_sha256"],
                "import_event_digest_fnv64": item["import_event_digest_fnv64"],
                "render_sha256": item["render_sha256"],
            }
            for item in observations
        ]
        set_summaries.append({
            "name": set_info["name"],
            "slug": set_info["slug"],
            "role": set_info["role"],
            "archive": set_spec["archive"],
            "archive_sha256": set_spec["sha256"],
            "source_aggregate": set_info["aggregate"],
            "observed_stems": len(observations),
            "all_imported": all(
                item.get("load_result") == "accepted"
                for item in observations
            ),
            "all_non_silent_projection": all(
                item["non_silent_projection"] is True
                for item in observations
            ),
            "stems": stem_records,
        })
    if not probe_path.is_file() or not source_path.is_file():
        die("donor MIDI corpus support files are missing")
    source_revision = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], text=True
    ).strip()

    receipt = {
        "schema_version": 1,
        "phase": "6C",
        "contract": DONOR_CONTRACT,
        "evidence_role": "cpsycle-donor-corpus-summary",
        "source_revision": source_revision,
        "source_file": source_path.name,
        "source_file_sha256": sha256_file(source_path),
        "probe_executable": probe_path.name,
        "probe_executable_sha256": sha256_file(probe_path),
        "progression_order": PROGRESSION,
        "sets": set_summaries,
        "all_sets_imported": all(item["all_imported"] for item in set_summaries),
        "all_sets_non_silent_projection": all(
            item["all_non_silent_projection"] for item in set_summaries
        ),
        "parity_status": "UNKNOWN",
        "claim_scope": (
            "C-Psycle real-world SMF import plus deterministic execution projection. "
            "The projection attaches a project-owned Sampulse substrate after freezing "
            "the untouched imported event digest; it is not native instrument-selection semantics."
        ),
    }
    write_json(output, receipt)


def corpus_summary(
    donor_path: pathlib.Path,
    candidate_path: pathlib.Path,
    output: pathlib.Path,
) -> None:
    donor = load_json(donor_path)
    candidate = load_json(candidate_path)
    def require_support(
        receipt_path: pathlib.Path,
        relative_name: Any,
        expected_sha: Any,
        label: str,
    ) -> None:
        if not isinstance(relative_name, str) or not relative_name:
            die(f"{label} support path is missing")
        relative = pathlib.PurePosixPath(relative_name)
        if relative.is_absolute() or ".." in relative.parts:
            die(f"{label} support path escapes its artifact root")
        path = receipt_path.parent / pathlib.Path(*relative.parts)
        if not path.is_file():
            die(f"{label} support file is missing: {relative_name}")
        if not isinstance(expected_sha, str) or len(expected_sha) != 64:
            die(f"{label} support SHA-256 is invalid")
        if sha256_file(path) != expected_sha.lower():
            die(f"{label} support file hash mismatch: {relative_name}")

    if (
        donor.get("schema_version") != 1
        or donor.get("phase") != "6C"
        or donor.get("contract") != DONOR_CONTRACT
        or donor.get("evidence_role") != "cpsycle-donor-corpus-summary"
        or donor.get("parity_status") != "UNKNOWN"
        or donor.get("all_sets_imported") is not True
        or donor.get("all_sets_non_silent_projection") is not True
        or donor.get("progression_order") != PROGRESSION
        or not isinstance(donor.get("sets"), list)
        or len(donor["sets"]) != 6
        or [item.get("name") for item in donor["sets"]] != PROGRESSION
    ):
        die("donor MIDI corpus summary is incomplete or invalid")
    manifest = corpus_manifest()
    manifest_by_name = {item["name"]: item for item in manifest["sets"]}
    for item in donor["sets"]:
        name = item["name"]
        set_spec = manifest_by_name[name]
        observed_stems = item.get("observed_stems")
        stem_records = item.get("stems")
        if (
            item.get("slug") != slug(name)
            or item.get("role") != set_spec["role"]
            or item.get("archive") != set_spec["archive"]
            or item.get("archive_sha256") != set_spec["sha256"]
            or observed_stems != set_spec["stems"]
            or item.get("all_imported") is not True
            or item.get("all_non_silent_projection") is not True
            or not isinstance(item.get("source_aggregate"), dict)
            or not isinstance(stem_records, list)
            or len(stem_records) != observed_stems
        ):
            die(f"donor {name} per-set evidence is incomplete")
        validate_aggregate_against_manifest(
            set_spec, item["source_aggregate"]
        )
        seen_ids: set[str] = set()
        seen_names: set[str] = set()
        expected_indexes = set(range(observed_stems))
        actual_indexes: set[int] = set()
        for record in stem_records:
            stem_id = record.get("stem_id")
            relative_name = record.get("source_relative_name")
            source_sha = record.get("source_sha256")
            source_size = record.get("source_size_bytes")
            analysis_sha = record.get("source_analysis_sha256")
            digest = record.get("import_event_digest_fnv64")
            render_sha = record.get("render_sha256")
            if (
                not isinstance(stem_id, str)
                or not isinstance(relative_name, str)
                or not isinstance(source_sha, str)
                or len(source_sha) != 64
                or not isinstance(source_size, int)
                or source_size <= 0
                or not isinstance(analysis_sha, str)
                or len(analysis_sha) != 64
                or not isinstance(digest, str)
                or len(digest) != 16
                or not isinstance(render_sha, str)
                or len(render_sha) != 64
            ):
                die(f"donor {name} stem evidence is malformed")
            prefix = f"{slug(name)}-"
            if not stem_id.startswith(prefix):
                die(f"donor {name} stem_id is outside the set namespace")
            remainder = stem_id[len(prefix):]
            parts = remainder.split("-", 1)
            if len(parts) != 2 or not parts[0].isdigit():
                die(f"donor {name} stem_id index is invalid")
            stem_index = int(parts[0])
            if parts[1] != source_sha[:12]:
                die(f"donor {name} stem_id is not bound to source SHA-256")
            if stem_id in seen_ids or relative_name in seen_names:
                die(f"donor {name} stem evidence is duplicated")
            seen_ids.add(stem_id)
            seen_names.add(relative_name)
            actual_indexes.add(stem_index)
            for value, label, length in (
                (source_sha, "source SHA-256", 64),
                (analysis_sha, "analysis SHA-256", 64),
                (render_sha, "render SHA-256", 64),
                (digest, "event digest", 16),
            ):
                if len(value) != length:
                    die(f"donor {name} {label} length changed")
                try:
                    int(value, 16)
                except ValueError:
                    die(f"donor {name} {label} is not hexadecimal")
        if actual_indexes != expected_indexes:
            die(f"donor {name} stem indexes are incomplete or duplicated")

    require_support(
        donor_path,
        donor.get("source_file"),
        donor.get("source_file_sha256"),
        "donor source",
    )
    require_support(
        donor_path,
        donor.get("probe_executable"),
        donor.get("probe_executable_sha256"),
        "donor probe",
    )

    candidate_sets = candidate.get("sets")
    if (
        candidate.get("schema_version") != 1
        or candidate.get("phase") != "6C"
        or candidate.get("contract") != CANDIDATE_CONTRACT
        or candidate.get("evidence_role") != "candidate-observation"
        or candidate.get("parity_status") != "UNKNOWN"
        or candidate.get("candidate_baseline_sha256") != EXPECTED_CANDIDATE_BASELINE
        or candidate.get("source_revision") != EXPECTED_CANDIDATE_SOURCE_REVISION
        or candidate.get("build_target") != "psycle-player"
        or candidate.get("private_input_required") is not True
        or candidate.get("progression_order") != PROGRESSION
        or not isinstance(candidate_sets, list)
        or len(candidate_sets) != 6
    ):
        die("candidate MIDI corpus boundary is incomplete or invalid")
    if [item.get("set_name") for item in candidate_sets] != PROGRESSION:
        die("candidate MIDI set order differs from the corpus progression")
    for item in candidate_sets:
        set_name = item.get("set_name")
        set_spec = manifest_by_name[set_name]
        if (
            item.get("archive") != set_spec["archive"]
            or item.get("archive_sha256") != set_spec["sha256"]
            or item.get("direct_midi_load") != "rejected"
            or item.get("exit_code") != 2
            or item.get("diagnostic_could_not_load_song_file") is not True
        ):
            die(f"candidate {item.get('set_name')} direct-load boundary is incomplete")
        rep_sha = item.get("representative_sha256")
        raw_sha = item.get("raw_output_sha256")
        if not isinstance(rep_sha, str) or len(rep_sha) != 64:
            die(f"candidate {item.get('set_name')} representative SHA-256 is invalid")
        if not isinstance(raw_sha, str) or len(raw_sha) != 64:
            die(f"candidate {item.get('set_name')} raw-output SHA-256 is invalid")

    require_support(
        candidate_path,
        candidate.get("player"),
        candidate.get("player_sha256"),
        "candidate player",
    )
    candidate_artifact = candidate_path.parent / candidate["player"]
    with candidate_artifact.open("rb") as handle:
        if handle.read(4) != b"\x7fELF":
            die("candidate player artifact is not an ELF executable")

    summary = {
        "schema_version": 1,
        "phase": "6C",
        "contract": SUMMARY_CONTRACT,
        "progression_order": PROGRESSION,
        "observations": {
            "cpsycle_donor": {
                "receipt": "donor/donor-midi-corpus.json",
                "receipt_sha256": sha256_file(donor_path),
                "all_sets_imported": True,
                "all_sets_non_silent_projection": True,
            },
            "frozen_cpp_candidate": {
                "receipt": "candidate/candidate-midi-boundary.json",
                "receipt_sha256": sha256_file(candidate_path),
                "representative_direct_load": "rejected",
            },
            "original_psycle_1_12_0_x86": {
                "status": "NOT_OBSERVED_IN_THIS_PHASE",
                "reason": (
                    "This real-world corpus phase is donor-first. Original-reference "
                    "MIDI import/playback remains a separate version-pinned observation "
                    "if parity classification is later required."
                ),
            },
        },
        "parity_status": "UNKNOWN",
        "classification_note": (
            "Real-world robustness evidence only. No compatibility-matrix row is "
            "created or promoted from this corpus summary."
        ),
    }
    write_json(output, summary)


def midi_varlen(value: int) -> bytes:
    if value < 0:
        raise ValueError("MIDI variable length value must be non-negative")
    buffer = value & 0x7F
    out = bytearray([buffer])
    while value >> 7:
        value >>= 7
        buffer = (value & 0x7F) | 0x80
        out.insert(0, buffer)
    return bytes(out)


def write_synthetic_fixture(path: pathlib.Path) -> None:
    track_zero = bytearray()
    track_zero += midi_varlen(0) + b"\xff\x51\x03\x07\xa1\x20"
    track_zero += midi_varlen(0) + b"\xff\x59\x02\x09\x01"
    track_zero += midi_varlen(0) + bytes((0x90, 60, 100))
    track_zero += midi_varlen(480) + bytes((0x80, 60, 0))
    track_zero += midi_varlen(0) + b"\xff\x2f\x00"

    track_one = bytearray()
    track_one += midi_varlen(240) + bytes((0x91, 64, 96))
    track_one += midi_varlen(240) + bytes((0x81, 64, 0))
    track_one += midi_varlen(0) + b"\xff\x2f\x00"

    payload = bytearray(b"MThd")
    payload += struct.pack(">IHHH", 6, 1, 2, 480)
    for track in (track_zero, track_one):
        payload += b"MTrk" + struct.pack(">I", len(track)) + track
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    synthetic_fixture = sub.add_parser("write-synthetic")
    synthetic_fixture.add_argument("output", type=pathlib.Path)
    synthetic_fixture.set_defaults(
        func=lambda args: write_synthetic_fixture(args.output.resolve())
    )

    prepare = sub.add_parser("prepare-bundle")
    prepare.add_argument("bundle", type=pathlib.Path)
    prepare.add_argument("root", type=pathlib.Path)
    prepare.set_defaults(func=lambda args: prepare_bundle(args.bundle.resolve(), args.root.resolve()))

    audit = sub.add_parser("audit-public")
    audit.add_argument("root", type=pathlib.Path)
    audit.set_defaults(func=lambda args: audit_public_tree(args.root.resolve()))

    candidate = sub.add_parser("candidate-boundary")
    candidate.add_argument("analysis", type=pathlib.Path)
    candidate.add_argument("bundle", type=pathlib.Path)
    candidate.add_argument("player", type=pathlib.Path)
    candidate.add_argument("output", type=pathlib.Path)
    candidate.set_defaults(
        func=lambda args: candidate_boundary(
            args.analysis.resolve(),
            args.bundle.resolve(),
            args.player.resolve(),
            args.output.resolve(),
        )
    )

    donor = sub.add_parser("donor-summary")
    donor.add_argument("analysis", type=pathlib.Path)
    donor.add_argument("bundle", type=pathlib.Path)
    donor.add_argument("observations", type=pathlib.Path)
    donor.add_argument("--probe", type=pathlib.Path, required=True)
    donor.add_argument("--source", type=pathlib.Path, required=True)
    donor.add_argument("--output", type=pathlib.Path, required=True)
    donor.set_defaults(
        func=lambda args: donor_summary(
            args.analysis.resolve(),
            args.bundle.resolve(),
            args.observations.resolve(),
            args.probe.resolve(),
            args.source.resolve(),
            args.output.resolve(),
        )
    )

    summary = sub.add_parser("summary")
    summary.add_argument("--donor", type=pathlib.Path, required=True)
    summary.add_argument("--candidate", type=pathlib.Path, required=True)
    summary.add_argument("--output", type=pathlib.Path, required=True)
    summary.set_defaults(
        func=lambda args: corpus_summary(
            args.donor.resolve(), args.candidate.resolve(), args.output.resolve()
        )
    )
    return parser


def main() -> int:
    args = build_parser().parse_args()
    result = args.func(args)
    if isinstance(result, dict):
        print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
