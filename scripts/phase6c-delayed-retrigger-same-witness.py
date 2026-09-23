#!/usr/bin/env python3
"""Collect and compare the Phase 6C same-witness Sampulse runtime renders."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
BASE_ANALYZER = ROOT / "scripts" / "phase6c-delayed-retrigger-render-evidence.py"
ORIGINAL_GATE = ROOT / "scripts" / "phase6c-delayed-retrigger-original.py"
REVIEWED_RENDERER_SOURCE = (
    ROOT / "tests" / "phase6c_delayed_retrigger_sampulse_render.cpp"
)
REVIEWED_RENDERER_PROJECT = (
    ROOT / "tests" / "phase6c_delayed_retrigger_sampulse_render.pro"
)
REVIEWED_FIXTURE_GENERATOR = (
    ROOT / "tests" / "phase6c_delayed_retrigger_sampulse_execution_fixture.c"
)
REVIEWED_EINS_CONVERTER = ROOT / "scripts" / "phase6c-sampulse-eins-compat.py"
EXPECTED_EINS_PAYLOAD_SHA256 = (
    "2f35cd43ae394ed987a149a2bdfc226e5be710d36b1516c1a3fc22c36037feb5"
)
EXPECTED_SMPD_SHA256 = (
    "8792a2a49e949c5648e6222e513f22fff644f20fca7d2f6801af766f230e0627"
)
EXPECTED_COMPRESSED_SAMPLE_SHA256 = (
    "6e0dc7e70768adbb91ab84a325d734b9a4eb19576dcc4547999447eb210f3bf0"
)
REQUIRED_RENDERER_DEFINED_SYMBOLS = (
    "psycle::core::Psy3Filter::LoadEINSv1",
    "psycle::core::Sequencer::Work(unsigned int)",
    "psycle::core::XMSampler::Channel::DelayedNote",
    "psycle::core::XMSampler::Voice::Retrig()",
    "psycle::core::Player::startRecording",
    "psycle::core::CoreSong::load",
)
REQUIRED_RENDERER_MAIN_CALLS = (
    "psycle::core::CoreSong::load",
    "psycle::core::Player::startRecording",
    "psycle::core::Sequencer::Work(unsigned int)",
)
REVIEWED_ENGINE_ANCHORS = {
    "sequencer": (
        ROOT
        / "psycle-cpp-r12005-sanitized"
        / "psycle-core"
        / "src"
        / "psycle"
        / "core"
        / "sequencer.cpp"
    ),
    "psy3_loader": (
        ROOT
        / "psycle-cpp-r12005-sanitized"
        / "psycle-core"
        / "src"
        / "psycle"
        / "core"
        / "psy3filter.cpp"
    ),
    "xmsampler": (
        ROOT
        / "psycle-cpp-r12005-sanitized"
        / "psycle-core"
        / "src"
        / "psycle"
        / "core"
        / "xmsampler.cpp"
    ),
}

CONTRACT = "sequencer-delayed-retrigger-same-witness-render"
NAME = "delayed-retrigger-sampulse-runtime"
FIXTURE = "delayed-retrigger/phase6c-delayed-retrigger-sampulse-execution.psy"
TITLE = "PSYCLE-LINUX Phase 6C delayed/retrigger Sampulse execution witness"
CANDIDATE_RECEIPT = "candidate-delayed-retrigger-sampulse-runtime.json"
ORIGINAL_RECEIPT = "original-delayed-retrigger-sampulse-runtime.json"
ORIGINAL_ANALYSIS = "original-delayed-retrigger-sampulse-runtime-analysis.json"
COMPARISON = "delayed-retrigger-sampulse-runtime-comparison.json"
REFERENCE_BUILD = "Psycle 1.12.0 x86"
ORIGINAL_RENDER_SETTINGS = {
    "sample_rate": 44100,
    "bits_per_sample": 16,
    "channels": "mono-mix",
    "dither": False,
    "range": "entire-song",
}
CANDIDATE_TARGET_BEATS = 4.25
CANDIDATE_BEAT_FRAMES = 44100.0 * 60.0 / 137.0
CANDIDATE_TARGET_FRAMES = math.ceil(
    CANDIDATE_TARGET_BEATS * CANDIDATE_BEAT_FRAMES
)

CANDIDATE_RENDER_PROCEDURE = {
    "engine": "frozen SourceForge SVN r12005 C++ candidate",
    "sample_rate": 44100,
    "bits_per_sample": 16,
    "channels": "mono-mix",
    "dither": False,
    "fixed_frame_render": True,
    "target_beats": CANDIDATE_TARGET_BEATS,
    "threads": 1,
    "repeat_count": 2,
    "sequencer_single_work": True,
    "player_work_direct": False,
    "harness_owned_master_stereo_buffer": True,
    "master_buffer_float_count": "2 * target_frames",
    "loader_preallocation": (
        "project-owned harness preallocates empty XM instrument/sample slot 0 "
        "because retained r12005 LoadEINSv1 loads into existing vectors; "
        "post-load validation requires the EINS bytes to overwrite both slots"
    ),
}


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ValueError(f"could not load module: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


base = load_module(BASE_ANALYZER, "phase6c_render_base")
original_gate_module = load_module(ORIGINAL_GATE, "phase6c_original_gate")
eins_converter_module = load_module(
    REVIEWED_EINS_CONVERTER, "phase6c_sampulse_eins_converter"
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def write_new(path: Path, value: dict) -> None:
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def child(root: Path, relative: str) -> Path:
    if not isinstance(relative, str):
        raise ValueError("unsafe artifact path")
    pure = PurePosixPath(relative)
    if pure.is_absolute() or ".." in pure.parts or "\\" in relative:
        raise ValueError("unsafe artifact path")
    result = (root / relative).resolve()
    result.relative_to(root.resolve())
    return result



def read_cstring(data: bytes, offset: int, label: str) -> tuple[str, int]:
    try:
        end = data.index(b"\0", offset)
    except ValueError as exc:
        raise ValueError(f"same-witness fixture lacks terminated {label}") from exc
    try:
        value = data[offset:end].decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError(f"same-witness fixture has invalid {label}") from exc
    return value, end + 1


def parse_psy3_chunks(data: bytes) -> list[tuple[bytes, int, bytes]]:
    if len(data) < 20 or data[:8] != b"PSY3SONG":
        raise ValueError("same-witness Sampulse fixture is not PSY3")
    song_size = struct.unpack_from("<I", data, 12)[0]
    chunk_count = struct.unpack_from("<I", data, 16)[0]
    chunk_start = 16 + song_size
    if song_size < 4 or chunk_start > len(data):
        raise ValueError("same-witness Sampulse fixture has invalid SONG header")
    chunks: list[tuple[bytes, int, bytes]] = []
    offset = chunk_start
    for _ in range(chunk_count):
        if offset + 12 > len(data):
            raise ValueError("same-witness Sampulse fixture has truncated chunk header")
        fourcc = data[offset : offset + 4]
        version, size = struct.unpack_from("<II", data, offset + 4)
        end = offset + 12 + size
        if end > len(data):
            raise ValueError("same-witness Sampulse fixture has truncated chunk payload")
        chunks.append((fourcc, version, data[offset + 12 : end]))
        offset = end
    if offset != len(data):
        raise ValueError("same-witness Sampulse fixture has trailing bytes")
    return chunks




def validate_sequence_playback(chunks: list[tuple[bytes, int, bytes]]) -> dict:
    seqd = [
        (version, payload)
        for fourcc, version, payload in chunks
        if fourcc == b"SEQD"
    ]
    if len(seqd) != 1 or seqd[0][0] != 2:
        raise ValueError("same-witness fixture must contain exactly one SEQD v2 chunk")
    payload = seqd[0][1]
    if len(payload) < 8:
        raise ValueError("same-witness fixture SEQD payload is truncated")
    sequence_index, play_length = struct.unpack_from("<ii", payload, 0)
    position = 8
    sequence_name, position = read_cstring(payload, position, "sequence name")
    if play_length < 0 or position + (4 * play_length) > len(payload):
        raise ValueError("same-witness fixture SEQD play order is invalid")
    pattern_slots = list(
        struct.unpack_from("<" + ("i" * play_length), payload, position)
    ) if play_length else []
    position += 4 * play_length
    if position + (4 * play_length) > len(payload):
        raise ValueError("same-witness fixture SEQD reposition data is truncated")
    reposition_offsets = list(
        struct.unpack_from("<" + ("f" * play_length), payload, position)
    ) if play_length else []
    position += 4 * play_length
    if position + 8 > len(payload):
        raise ValueError("same-witness fixture SEQD extension is truncated")
    marker_count = struct.unpack_from("<I", payload, position)[0]
    position += 4
    if marker_count != 0:
        raise ValueError("same-witness fixture SEQD unexpectedly schedules markers")
    sample_count = struct.unpack_from("<I", payload, position)[0]
    position += 4
    if sample_count != 0:
        raise ValueError("same-witness fixture SEQD unexpectedly schedules samples")
    if position + 4 != len(payload):
        raise ValueError("same-witness fixture SEQD trailing data mismatch")
    track_height = struct.unpack_from("<f", payload, position)[0]
    if (
        sequence_index != 0
        or play_length != 1
        or sequence_name != "seq"
        or pattern_slots != [0]
        or reposition_offsets != [0.0]
        or not math.isfinite(track_height)
    ):
        raise ValueError(
            "same-witness fixture does not schedule command pattern 0 for playback"
        )
    return {
        "sequence_index": sequence_index,
        "sequence_name": sequence_name,
        "play_length": play_length,
        "pattern_slots": pattern_slots,
        "reposition_offsets": reposition_offsets,
        "sha256": digest(payload),
    }


def parse_machine_routing(payload: bytes) -> dict:
    if len(payload) < 8:
        raise ValueError("same-witness fixture MACD payload is truncated")
    slot, machine_type = struct.unpack_from("<ii", payload, 0)
    position = 8
    module_name, position = read_cstring(
        payload, position, f"MACD slot {slot} module name"
    )
    if position + 22 > len(payload):
        raise ValueError("same-witness fixture MACD routing header is truncated")
    bypassed, muted = struct.unpack_from("<BB", payload, position)
    position += 2
    panning, x, y, input_count, output_count = struct.unpack_from(
        "<iiiii", payload, position
    )
    position += 20
    connections = []
    for connection_index in range(12):
        if position + 18 > len(payload):
            raise ValueError("same-witness fixture MACD wire table is truncated")
        input_slot, output_slot, input_volume, wire_multiplier = struct.unpack_from(
            "<iiff", payload, position
        )
        position += 16
        output_connected, input_connected = struct.unpack_from(
            "<BB", payload, position
        )
        position += 2
        connections.append(
            {
                "index": connection_index,
                "input_slot": input_slot,
                "output_slot": output_slot,
                "input_volume": input_volume,
                "wire_multiplier": wire_multiplier,
                "output_connected": output_connected,
                "input_connected": input_connected,
            }
        )
    edit_name, _ = read_cstring(
        payload, position, f"MACD slot {slot} edit name"
    )
    return {
        "slot": slot,
        "machine_type": machine_type,
        "module_name": module_name,
        "bypassed": bypassed,
        "muted": muted,
        "panning": panning,
        "position": [x, y],
        "input_count": input_count,
        "output_count": output_count,
        "connections": connections,
        "edit_name": edit_name,
        "sha256": digest(payload),
    }


def validate_playback_graph(
    chunks: list[tuple[bytes, int, bytes]]
) -> dict:
    sequence = validate_sequence_playback(chunks)
    machine_chunks = [
        (version, payload)
        for fourcc, version, payload in chunks
        if fourcc == b"MACD"
    ]
    if len(machine_chunks) != 2 or any(
        version != 3 for version, _payload in machine_chunks
    ):
        raise ValueError(
            "same-witness fixture must contain exactly sampler and Master MACD v3 chunks"
        )
    machines = {
        parsed["slot"]: parsed
        for parsed in (
            parse_machine_routing(payload)
            for _version, payload in machine_chunks
        )
    }
    if set(machines) != {0, 128}:
        raise ValueError("same-witness fixture playback machine set mismatch")
    sampler = machines[0]
    master = machines[128]
    sampler_outputs = [
        item
        for item in sampler["connections"]
        if item["output_connected"] == 1
    ]
    master_inputs = [
        item
        for item in master["connections"]
        if item["input_connected"] == 1
    ]
    if (
        sampler["machine_type"] != 12
        or sampler["bypassed"] != 0
        or sampler["muted"] != 0
        or sampler["input_count"] != 0
        or sampler["output_count"] != 1
        or len(sampler_outputs) != 1
        or sampler_outputs[0]["output_slot"] != 128
        or sampler_outputs[0]["input_slot"] != -1
        or master["machine_type"] != 0
        or master["input_count"] != 1
        or master["output_count"] != 0
        or len(master_inputs) != 1
        or master_inputs[0]["input_slot"] != 0
        or master_inputs[0]["output_slot"] != -1
    ):
        raise ValueError(
            "same-witness fixture does not route sampler slot 0 directly to Master slot 128"
        )

    active_gains = (
        sampler_outputs[0]["wire_multiplier"],
        master_inputs[0]["input_volume"],
    )
    if any(
        not math.isfinite(float(value))
        or not math.isclose(float(value), 1.0, rel_tol=0.0, abs_tol=1e-7)
        for value in active_gains
    ):
        raise ValueError(
            "same-witness fixture lacks canonical audible sampler-to-Master gain"
        )
    return {
        "sequence": sequence,
        "sampler_slot": 0,
        "sampler_type": sampler["machine_type"],
        "sampler_macd_sha256": sampler["sha256"],
        "master_slot": 128,
        "master_type": master["machine_type"],
        "master_macd_sha256": master["sha256"],
        "route": [0, 128],
    }


EXPECTED_FIXTURE_EVENTS = [
    (0, 0, 60, 0, 0, 255, 0xFD, 0x7F),
    (0, 480, 60, 0, 0, 255, 0xFB, 0x3F),
    (0, 960, 60, 0, 0, 255, 0xFA, 0x42),
    (0, 1440, 255, 0, 0, 255, 0xFE, 0x04),
    (0, 1500, 60, 0, 0, 255, 0x00, 0x00),
]



def validate_eins_payload(payload: bytes) -> dict:
    if digest(payload) != EXPECTED_EINS_PAYLOAD_SHA256:
        raise ValueError(
            "same-witness EINS payload digest differs from canonical converter output"
        )
    position = 0

    def take(fmt: str, label: str):
        nonlocal position
        size = struct.calcsize(fmt)
        if position + size > len(payload):
            raise ValueError(f"same-witness EINS payload truncates {label}")
        values = struct.unpack_from(fmt, payload, position)
        position += size
        return values[0] if len(values) == 1 else values

    if take("<I", "instrument count") != 1 or take("<i", "instrument index") != 0:
        raise ValueError("same-witness EINS payload instrument table mismatch")

    expected_instrument = eins_converter_module.historical_instrument()
    if payload[position : position + len(expected_instrument)] != expected_instrument:
        raise ValueError("same-witness EINS payload instrument state mismatch")
    position += len(expected_instrument)

    if take("<I", "sample count") != 1 or take("<i", "sample index") != 0:
        raise ValueError("same-witness EINS payload sample table mismatch")
    sample_start = position
    if position + 12 > len(payload) or payload[position : position + 4] != b"SMPD":
        raise ValueError("same-witness EINS payload lacks historical SMPD sample")
    sample_size = struct.unpack_from("<I", payload, position + 4)[0]
    sample_version = struct.unpack_from("<I", payload, position + 8)[0]
    if (
        sample_version != 1
        or sample_size < 12
        or sample_start + sample_size != len(payload)
    ):
        raise ValueError("same-witness EINS payload sample chunk identity mismatch")

    smpd = payload[sample_start : sample_start + sample_size]
    if digest(smpd) != EXPECTED_SMPD_SHA256:
        raise ValueError("same-witness EINS SMPD payload digest mismatch")
    sample_body = payload[position + 12 : sample_start + sample_size]
    sample_name, sample_position = read_cstring(sample_body, 0, "EINS sample name")

    def sample_take(fmt: str, label: str):
        nonlocal sample_position
        size = struct.calcsize(fmt)
        if sample_position + size > len(sample_body):
            raise ValueError(f"same-witness EINS sample truncates {label}")
        values = struct.unpack_from(fmt, sample_body, sample_position)
        sample_position += size
        return values[0] if len(values) == 1 else values

    wave_length = sample_take("<I", "wave length")
    global_volume = sample_take("<f", "global volume")
    default_volume = sample_take("<H", "default volume")
    loop_start, loop_end, loop_type = sample_take("<III", "loop state")
    sustain_start, sustain_end, sustain_type = sample_take(
        "<III", "sustain loop state"
    )
    sample_rate = sample_take("<I", "sample rate")
    tune, fine_tune = sample_take("<hh", "sample tuning")
    stereo = sample_take("<?", "stereo flag")
    pan_enabled = sample_take("<?", "pan-enabled flag")
    pan_factor = sample_take("<f", "pan factor")
    surround = sample_take("<?", "surround flag")
    vibrato = sample_take("<BBBB", "vibrato state")
    compressed_size = sample_take("<I", "compressed mono size")
    if compressed_size <= 0 or sample_position + compressed_size != len(sample_body):
        raise ValueError("same-witness EINS sample compressed payload mismatch")
    compressed = sample_body[sample_position : sample_position + compressed_size]

    if (
        sample_name != "Phase 6C deterministic impulse"
        or wave_length != 512
        or sample_rate != 44100
        or tune != 0
        or fine_tune != 0
        or stereo is not False
        or not math.isfinite(float(global_volume))
        or not 0.0 < float(global_volume) <= 1.0
        or not isinstance(default_volume, int)
        or default_volume <= 0
        or not math.isfinite(float(pan_factor))
        or not 0.0 <= float(pan_factor) <= 1.0
        or loop_type not in (0, 1, 2)
        or sustain_type not in (0, 1, 2)
        or digest(compressed) != EXPECTED_COMPRESSED_SAMPLE_SHA256
    ):
        raise ValueError("same-witness EINS payload deterministic sample mismatch")

    return {
        "instrument_count": 1,
        "instrument_index": 0,
        "instrument_name": "Phase 6C click instrument",
        "note_60_sample": 0,
        "sample_count": 1,
        "sample_index": 0,
        "sample_name": sample_name,
        "sample_frames": wave_length,
        "sample_rate": sample_rate,
        "mono": True,
        "compressed_sample_sha256": digest(compressed),
        "loop": [loop_start, loop_end, loop_type],
        "sustain_loop": [sustain_start, sustain_end, sustain_type],
        "pan_enabled": pan_enabled,
        "surround": surround,
        "vibrato": list(vibrato),
    }


def decompress_beerz77_v2(data: bytes, expected_size: int) -> bytes:
    """Decode the exact legacy PATD stream consumed by r12005 LoadPATDv0."""
    if len(data) < 5 or data[0] != 0x04:
        raise ValueError("same-witness legacy PATD compression header is invalid")
    declared_size = int.from_bytes(data[1:5], "little")
    if declared_size != expected_size:
        raise ValueError("same-witness legacy PATD decompressed size mismatch")

    source = 5
    output = bytearray()
    while len(output) < declared_size:
        if source >= len(data):
            raise ValueError("same-witness legacy PATD compressed stream is truncated")
        length = data[source]
        source += 1
        if length:
            if (
                source + length > len(data)
                or len(output) + length > declared_size
            ):
                raise ValueError("same-witness legacy PATD literal run is invalid")
            output += data[source : source + length]
            source += length
            continue

        if source + 2 > len(data):
            raise ValueError("same-witness legacy PATD back-reference is truncated")
        length = data[source] + 3
        offset = data[source + 1]
        source += 2
        start = len(output) - offset - length
        if (
            start < 0
            or start + length > len(output)
            or len(output) + length > declared_size
        ):
            raise ValueError("same-witness legacy PATD back-reference is invalid")
        output += output[start : start + length]

    if source != len(data):
        raise ValueError("same-witness legacy PATD compressed stream has trailing bytes")
    return bytes(output)


def expected_legacy_pattern_bytes(pattern_lines: int, pattern_tracks: int) -> bytes:
    if (pattern_lines, pattern_tracks) != (32, 16):
        raise ValueError("same-witness fixture legacy pattern geometry mismatch")
    empty = bytes((255, 255, 255, 0, 0))
    grid = bytearray(empty * (pattern_lines * pattern_tracks))
    ticks_per_line = 480 // 8
    for (
        track,
        offset_ticks,
        note,
        inst,
        mach,
        _volume,
        command,
        parameter,
    ) in EXPECTED_FIXTURE_EVENTS:
        if offset_ticks % ticks_per_line != 0:
            raise ValueError("same-witness command is not representable in legacy PATD")
        row = offset_ticks // ticks_per_line
        if not 0 <= track < pattern_tracks or not 0 <= row < pattern_lines:
            raise ValueError("same-witness command lies outside legacy PATD")
        position = (row * pattern_tracks + track) * 5
        grid[position : position + 5] = bytes(
            (note & 0xFF, inst & 0xFF, mach & 0xFF, command & 0xFF, parameter & 0xFF)
        )
    return bytes(grid)


def validate_fixture_identity(data: bytes) -> dict:
    chunks = parse_psy3_chunks(data)
    playback_graph = validate_playback_graph(chunks)

    info = [payload for fourcc, _version, payload in chunks if fourcc == b"INFO"]
    if len(info) != 1:
        raise ValueError("same-witness fixture must contain exactly one INFO chunk")
    title, _ = read_cstring(info[0], 0, "song title")
    if title != TITLE:
        raise ValueError("same-witness fixture song title mismatch")

    sngi = [payload for fourcc, _version, payload in chunks if fourcc == b"SNGI"]
    if len(sngi) != 1 or len(sngi[0]) < 12:
        raise ValueError("same-witness fixture SNGI identity is invalid")
    song_tracks, bpm, lpb = struct.unpack_from("<iii", sngi[0], 0)
    if (song_tracks, bpm, lpb) != (16, 137, 8):
        raise ValueError("same-witness fixture tempo/track geometry mismatch")

    machine_matches = []
    for fourcc, _version, payload in chunks:
        if fourcc != b"MACD" or len(payload) < 8:
            continue
        slot, machine_type = struct.unpack_from("<ii", payload, 0)
        if slot == 0:
            machine_matches.append(machine_type)
    if machine_matches != [12]:
        raise ValueError("same-witness fixture lacks XMSampler at machine slot 0")

    eins = [
        (version, payload)
        for fourcc, version, payload in chunks
        if fourcc == b"EINS"
    ]
    if (
        len(eins) != 1
        or eins[0][0] != 0x00010000
        or any(
            fourcc in {b"SMID", b"SMSB"}
            for fourcc, _version, _payload in chunks
        )
    ):
        raise ValueError("same-witness fixture historical Sampulse state mismatch")
    eins_identity = validate_eins_payload(eins[0][1])

    patterns = [
        (version, payload)
        for fourcc, version, payload in chunks
        if fourcc == b"PATD"
        and len(payload) >= 4
        and struct.unpack_from("<i", payload, 0)[0] == 0
    ]
    if len(patterns) != 1 or patterns[0][0] != 2:
        raise ValueError("same-witness fixture pattern-0 identity mismatch")
    payload = patterns[0][1]
    if len(payload) < 16:
        raise ValueError("same-witness fixture pattern-0 payload is truncated")
    index, pattern_lines, pattern_tracks = struct.unpack_from("<iii", payload, 0)
    position = 12
    pattern_name, position = read_cstring(payload, position, "pattern name")
    if position + 4 > len(payload):
        raise ValueError("same-witness fixture pattern compression header is truncated")
    compressed_size = struct.unpack_from("<I", payload, position)[0]
    position += 4
    if compressed_size <= 0 or position + compressed_size > len(payload):
        raise ValueError("same-witness fixture pattern compression payload is truncated")
    compressed_pattern = payload[position : position + compressed_size]
    legacy_pattern = decompress_beerz77_v2(
        compressed_pattern, pattern_lines * pattern_tracks * 5
    )
    if legacy_pattern != expected_legacy_pattern_bytes(pattern_lines, pattern_tracks):
        raise ValueError(
            "same-witness fixture legacy PATD playback grid does not match command witness"
        )
    position += compressed_size
    if position + 20 > len(payload):
        raise ValueError("same-witness fixture extended pattern header is truncated")
    timesig_cmd, timesig_param, ppq, length_ticks, entry_count = struct.unpack_from(
        "<IIiii", payload, position
    )
    position += 20
    if (
        index != 0
        or pattern_lines != 32
        or pattern_tracks != 16
        or pattern_name != "Execution Witness"
        or timesig_cmd != 0
        or timesig_param != 0
        or ppq != 480
        or length_ticks != 1920
        or entry_count != len(EXPECTED_FIXTURE_EVENTS)
    ):
        raise ValueError("same-witness fixture pattern geometry mismatch")

    observed_events: list[tuple[int, int, int, int, int, int, int, int]] = []
    for _ in range(entry_count):
        if position + 12 > len(payload):
            raise ValueError("same-witness fixture pattern entry is truncated")
        track, offset_ticks, event_count = struct.unpack_from("<iii", payload, position)
        position += 12
        if event_count != 1 or position + 24 > len(payload):
            raise ValueError("same-witness fixture pattern event shape mismatch")
        note, inst, mach, volume, command, parameter = struct.unpack_from(
            "<iiiiii", payload, position
        )
        position += 24
        observed_events.append(
            (track, offset_ticks, note, inst, mach, volume, command, parameter)
        )
    if position != len(payload) or observed_events != EXPECTED_FIXTURE_EVENTS:
        raise ValueError("same-witness fixture command geometry mismatch")

    return {
        "song_title": title,
        "song_tracks": song_tracks,
        "bpm": bpm,
        "lpb": lpb,
        "machine_slot": 0,
        "machine_type": 12,
        "machine_substrate": "XMSampler/Sampulse",
        "eins_version": 0x00010000,
        "eins_identity": eins_identity,
        "playback_graph": playback_graph,
        "pattern_name": pattern_name,
        "pattern_lines": pattern_lines,
        "pattern_tracks": pattern_tracks,
        "pattern_ppq": ppq,
        "pattern_length_ticks": length_ticks,
        "command_events": [
            {
                "track": track,
                "offset_ticks": offset_ticks,
                "note": note,
                "instrument": inst,
                "machine": mach,
                "volume": volume,
                "command": command,
                "parameter": parameter,
            }
            for (
                track,
                offset_ticks,
                note,
                inst,
                mach,
                volume,
                command,
                parameter,
            ) in observed_events
        ],
    }


def validate_same_witness_analysis(
    analysis: dict,
    role: str,
    *,
    expected_frame_count: int | None = None,
    require_all_command_windows: bool = False,
) -> dict:
    if expected_frame_count is not None and analysis.get("frame_count") != expected_frame_count:
        raise ValueError(
            f"{role} render frame count does not match fixed-frame target"
        )
    if require_all_command_windows:
        counts = analysis.get("window_onset_counts")
        required = (
            "note_delay_beat_0",
            "retrigger_beat_1",
            "retr_cont_beat_2",
            "extended_marker_beat_3",
        )
        if (
            not isinstance(counts, dict)
            or any(
                not isinstance(counts.get(key), int)
                or isinstance(counts.get(key), bool)
                or counts[key] <= 0
                for key in required
            )
        ):
            raise ValueError(
                f"{role} render does not expose every command-bearing window"
            )
        onset_beats = analysis.get("onset_beats")
        bounded_beat_3 = False
        if isinstance(onset_beats, list):
            for value in onset_beats:
                if (
                    isinstance(value, (int, float))
                    and not isinstance(value, bool)
                    and math.isfinite(float(value))
                    and 3.0 <= float(value) < 4.0
                ):
                    bounded_beat_3 = True
                    break
        if not bounded_beat_3:
            raise ValueError(
                f"{role} render lacks an onset in the bounded beat-3 command window"
            )
    return analysis


def reviewed_repository_bytes(path: Path) -> bytes:
    """Read the committed blob, independent of checkout line-ending conversion."""
    try:
        relative = path.resolve().relative_to(ROOT.resolve()).as_posix()
    except ValueError as exc:
        raise ValueError("reviewed input is outside the repository") from exc
    try:
        return subprocess.check_output(
            ["git", "cat-file", "blob", f"HEAD:{relative}"],
            cwd=ROOT,
            stderr=subprocess.DEVNULL,
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValueError(f"could not read reviewed repository input: {relative}") from exc


def reviewed_digest(path: Path) -> str:
    return digest(reviewed_repository_bytes(path))


def validate_retained_reviewed_copy(
    root: Path, retained_relative: str, reviewed_path: Path, label: str
) -> dict:
    retained = child(root, retained_relative).read_bytes()
    reviewed = reviewed_repository_bytes(reviewed_path)
    if retained != reviewed:
        raise ValueError(f"same-witness retained {label} differs from reviewed repository input")
    return {"path": retained_relative, "sha256": digest(retained)}


def reviewed_engine_bindings() -> dict:
    return {
        name: {
            "path": path.resolve().relative_to(ROOT.resolve()).as_posix(),
            "sha256": reviewed_digest(path),
        }
        for name, path in REVIEWED_ENGINE_ANCHORS.items()
    }


def expected_build_header() -> bytes:
    engines = reviewed_engine_bindings()
    return (
        "#pragma once\n"
        f"#define PHASE6C_RENDER_SOURCE_SHA256 \"{reviewed_digest(REVIEWED_RENDERER_SOURCE)}\"\n"
        f"#define PHASE6C_RENDER_PROJECT_SHA256 \"{reviewed_digest(REVIEWED_RENDERER_PROJECT)}\"\n"
        f"#define PHASE6C_ENGINE_SEQUENCER_SHA256 \"{engines['sequencer']['sha256']}\"\n"
        f"#define PHASE6C_ENGINE_PSY3_LOADER_SHA256 \"{engines['psy3_loader']['sha256']}\"\n"
        f"#define PHASE6C_ENGINE_XMSAMPLER_SHA256 \"{engines['xmsampler']['sha256']}\"\n"
    ).encode("utf-8")


def expected_compiled_provenance() -> dict:
    engines = reviewed_engine_bindings()
    return {
        "schema_version": 1,
        "renderer_source_sha256": reviewed_digest(REVIEWED_RENDERER_SOURCE),
        "renderer_project_sha256": reviewed_digest(REVIEWED_RENDERER_PROJECT),
        "engine_sequencer_sha256": engines["sequencer"]["sha256"],
        "engine_psy3_loader_sha256": engines["psy3_loader"]["sha256"],
        "engine_xmsampler_sha256": engines["xmsampler"]["sha256"],
    }


def validate_compiled_provenance_output(data: bytes) -> dict:
    lines = [line.strip() for line in data.splitlines() if line.strip()]
    if len(lines) != 1:
        raise ValueError("same-witness renderer provenance challenge output is invalid")
    try:
        value = json.loads(lines[0].decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ValueError(
            "same-witness renderer provenance challenge output is invalid"
        ) from exc
    if value != expected_compiled_provenance():
        raise ValueError("same-witness renderer compiled provenance mismatch")
    return value



def validate_renderer_code_identity(root: Path) -> dict:
    binary_relative = f"{NAME}/phase6c-delayed-retrigger-sampulse-render"
    symbols_relative = "delayed-retrigger/sampulse-render-symbols.log"
    main_relative = "delayed-retrigger/sampulse-render-main-disassembly.log"
    binary_path = child(root, binary_relative)
    symbols_path = child(root, symbols_relative)
    main_path = child(root, main_relative)
    symbols = symbols_path.read_bytes()
    main_disassembly = main_path.read_bytes()
    try:
        symbols_text = symbols.decode("utf-8")
        main_text = main_disassembly.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError(
            "same-witness renderer code-identity evidence is not UTF-8"
        ) from exc

    missing_symbols = [
        marker
        for marker in REQUIRED_RENDERER_DEFINED_SYMBOLS
        if marker not in symbols_text
    ]
    if missing_symbols:
        raise ValueError(
            "same-witness renderer lacks required reviewed renderer symbol: "
            + missing_symbols[0]
        )
    missing_calls = [
        marker
        for marker in REQUIRED_RENDERER_MAIN_CALLS
        if marker not in main_text
    ]
    if missing_calls:
        raise ValueError(
            "same-witness renderer main does not call reviewed render path: "
            + missing_calls[0]
        )

    if sys.platform.startswith("linux"):
        try:
            actual_symbols = subprocess.check_output(
                ["nm", "-C", "--defined-only", str(binary_path)],
                cwd=root,
                stderr=subprocess.STDOUT,
            )
            actual_main = subprocess.check_output(
                [
                    "objdump",
                    "-d",
                    "-C",
                    "--disassemble=main",
                    str(binary_path),
                ],
                cwd=root,
                stderr=subprocess.STDOUT,
            )
        except (OSError, subprocess.CalledProcessError) as exc:
            raise ValueError(
                "same-witness renderer code-identity inspection failed"
            ) from exc
        if actual_symbols != symbols or actual_main != main_disassembly:
            raise ValueError(
                "same-witness renderer code-identity evidence does not match exact ELF"
            )

    return {
        "symbols_log": artifact_binding(root, symbols_relative),
        "main_disassembly_log": artifact_binding(root, main_relative),
    }


def elf_text_sha256(path: Path, work: Path, label: str) -> str:
    dumped = work / (label + ".text")
    try:
        subprocess.check_output(
            ["objcopy", "--dump-section", f".text={dumped}", str(path)],
            stderr=subprocess.STDOUT,
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValueError("same-witness renderer .text inspection failed") from exc
    if not dumped.is_file() or dumped.stat().st_size == 0:
        raise ValueError("same-witness renderer .text section is missing")
    return digest(dumped.read_bytes())


def validate_reproducible_renderer_build(root: Path) -> None:
    """Rebuild reviewed renderer code and bind the retained ELF to its .text."""
    if not sys.platform.startswith("linux"):
        return

    retained = child(
        root, f"{NAME}/phase6c-delayed-retrigger-sampulse-render"
    )
    player_root = ROOT / "psycle-cpp-r12005-sanitized" / "psycle-player"
    staging = Path(
        tempfile.mkdtemp(prefix="phase6c-sampulse-verify-", dir=player_root)
    )
    try:
        with tempfile.TemporaryDirectory(prefix="phase6c-sampulse-build-") as temporary:
            build = Path(temporary)
            (build / "phase6c-render-provenance.hpp").write_bytes(
                expected_build_header()
            )
            project = staging / "render.pro"
            project.write_bytes(reviewed_repository_bytes(REVIEWED_RENDERER_PROJECT))
            qmake = subprocess.run(
                [
                    "qmake",
                    "CONFIG-=shared",
                    "CONFIG+=release",
                    f"PROBE_BUILD_DIR={build}",
                    f"REPO_ROOT={ROOT}",
                    "-o",
                    str(build / "Makefile"),
                    str(project),
                ],
                cwd=staging,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            )
            if qmake.returncode != 0:
                raise ValueError(
                    "same-witness renderer reproducible build qmake failed"
                )
            make = subprocess.run(
                [
                    "make",
                    "-C",
                    str(build),
                    "-f",
                    str(build / "Makefile"),
                    "-j2",
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            )
            rebuilt = build / "phase6c-delayed-retrigger-sampulse-render"
            if make.returncode != 0 or not rebuilt.is_file():
                raise ValueError(
                    "same-witness renderer reproducible build failed"
                )
            retained_text = elf_text_sha256(retained, build, "retained")
            rebuilt_text = elf_text_sha256(rebuilt, build, "rebuilt")
            if retained_text != rebuilt_text:
                raise ValueError(
                    "same-witness renderer reproducible build code identity mismatch"
                )
    finally:
        shutil.rmtree(staging, ignore_errors=True)


def validate_renderer_build_provenance(
    root: Path, observations: list[dict]
) -> dict:
    source_relative = f"{NAME}/render-probe.cpp"
    project_relative = f"{NAME}/render-probe.pro"
    generator_relative = f"{NAME}/fixture-generator.c"
    converter_relative = f"{NAME}/eins-compat.py"
    binary_relative = f"{NAME}/phase6c-delayed-retrigger-sampulse-render"
    header_relative = f"{NAME}/renderer-build-provenance.hpp"
    attestation_relative = f"{NAME}/renderer-build-provenance.json"
    qmake_relative = "delayed-retrigger/sampulse-render-qmake.log"
    build_relative = "delayed-retrigger/sampulse-render-build.log"
    provenance_relative = "delayed-retrigger/sampulse-render-provenance.log"

    source_binding = validate_retained_reviewed_copy(
        root, source_relative, REVIEWED_RENDERER_SOURCE, "renderer source"
    )
    project_binding = validate_retained_reviewed_copy(
        root, project_relative, REVIEWED_RENDERER_PROJECT, "renderer project"
    )
    generator_binding = validate_retained_reviewed_copy(
        root, generator_relative, REVIEWED_FIXTURE_GENERATOR, "fixture generator"
    )
    converter_binding = validate_retained_reviewed_copy(
        root, converter_relative, REVIEWED_EINS_CONVERTER, "EINS converter"
    )

    binary = child(root, binary_relative).read_bytes()
    if not binary.startswith(b"\x7fELF"):
        raise ValueError("same-witness retained renderer is not an ELF executable")
    binary_binding = {"path": binary_relative, "sha256": digest(binary)}

    header = child(root, header_relative).read_bytes()
    if header != expected_build_header():
        raise ValueError("same-witness renderer build header identity mismatch")
    header_binding = {"path": header_relative, "sha256": digest(header)}

    attestation = read_json(child(root, attestation_relative))
    expected_inputs = {
        "renderer_source": {
            "path": "tests/phase6c_delayed_retrigger_sampulse_render.cpp",
            "sha256": source_binding["sha256"],
        },
        "renderer_project": {
            "path": "tests/phase6c_delayed_retrigger_sampulse_render.pro",
            "sha256": project_binding["sha256"],
        },
        "fixture_generator": {
            "path": "tests/phase6c_delayed_retrigger_sampulse_execution_fixture.c",
            "sha256": generator_binding["sha256"],
        },
        "eins_converter": {
            "path": "scripts/phase6c-sampulse-eins-compat.py",
            "sha256": converter_binding["sha256"],
        },
    }
    engine_bindings = reviewed_engine_bindings()
    qmake_binding = artifact_binding(root, qmake_relative)
    build_binding = artifact_binding(root, build_relative)
    provenance_binding = artifact_binding(root, provenance_relative)
    provenance_bytes = child(root, provenance_relative).read_bytes()
    validate_compiled_provenance_output(provenance_bytes)
    code_identity = validate_renderer_code_identity(root)
    validate_reproducible_renderer_build(root)
    if (
        attestation.get("schema_version") != 3
        or attestation.get("reviewed_inputs") != expected_inputs
        or attestation.get("engine_anchors") != engine_bindings
        or attestation.get("binary")
        != {"path": binary_relative, "sha256": binary_binding["sha256"]}
        or attestation.get("qmake_log") != qmake_binding
        or attestation.get("build_log") != build_binding
        or attestation.get("provenance_challenge_log") != provenance_binding
        or attestation.get("code_identity") != code_identity
    ):
        raise ValueError("same-witness renderer build attestation mismatch")
    build_text = child(root, build_relative).read_text(encoding="utf-8", errors="replace")
    if (
        "phase6c_delayed_retrigger_sampulse_render.cpp" not in build_text
        or "phase6c-delayed-retrigger-sampulse-render" not in build_text
    ):
        raise ValueError("same-witness renderer build log lacks reviewed build path")

    expected_compiled = expected_compiled_provenance()
    for observation in observations:
        if (
            observation.get("renderer_source_sha256") != source_binding["sha256"]
            or observation.get("renderer_project_sha256") != project_binding["sha256"]
            or observation.get("engine_sequencer_sha256")
            != expected_compiled["engine_sequencer_sha256"]
            or observation.get("engine_psy3_loader_sha256")
            != expected_compiled["engine_psy3_loader_sha256"]
            or observation.get("engine_xmsampler_sha256")
            != expected_compiled["engine_xmsampler_sha256"]
        ):
            raise ValueError(
                "same-witness renderer runtime identity does not match reviewed inputs"
            )

    if sys.platform.startswith("linux"):
        binary_path = child(root, binary_relative)
        try:
            completed = subprocess.run(
                [str(binary_path), "--phase6c-provenance"],
                cwd=root,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                timeout=10,
                check=False,
            )
        except (OSError, subprocess.SubprocessError) as exc:
            raise ValueError(
                "same-witness renderer executable provenance challenge failed"
            ) from exc
        if (
            completed.returncode != 0
            or completed.stderr
            or completed.stdout != provenance_bytes
        ):
            raise ValueError(
                "same-witness renderer executable provenance challenge failed"
            )
        validate_compiled_provenance_output(completed.stdout)

    return {
        "binary": binary_binding,
        "source": source_binding,
        "project": project_binding,
        "fixture_generator_source": generator_binding,
        "eins_converter_source": converter_binding,
        "build_header": header_binding,
        "build_attestation": artifact_binding(root, attestation_relative),
        "qmake_log": qmake_binding,
        "build_log": build_binding,
        "provenance_challenge_log": provenance_binding,
        "renderer_symbols": code_identity["symbols_log"],
        "renderer_main_disassembly": code_identity["main_disassembly_log"],
    }


def render_binding(root: Path, relative: str) -> tuple[dict, bytes]:
    path = child(root, relative)
    data = path.read_bytes()
    return {"path": relative, "sha256": digest(data)}, data


def artifact_binding(root: Path, relative: str) -> dict:
    binding, _ = render_binding(root, relative)
    return binding


def validate_artifact_binding(root: Path, value: object, expected_path: str) -> dict:
    if (
        not isinstance(value, dict)
        or set(value) != {"path", "sha256"}
        or value.get("path") != expected_path
        or not isinstance(value.get("sha256"), str)
        or len(value["sha256"]) != 64
    ):
        raise ValueError("invalid retained artifact binding: " + expected_path)
    actual = artifact_binding(root, expected_path)
    if actual != value:
        raise ValueError("retained artifact binding mismatch: " + expected_path)
    return actual


def validate_pair_of_waves(
    root: Path,
    prefix: str,
    role: str,
    *,
    expected_frame_count: int | None = None,
    require_all_command_windows: bool = False,
) -> tuple[list[dict], bytes, dict]:
    bindings: list[dict] = []
    waves: list[bytes] = []
    analyses: list[dict] = []
    for index in (1, 2):
        relative = f"{NAME}/{prefix}-{index}.wav"
        binding, data = render_binding(root, relative)
        analysis = validate_same_witness_analysis(
            base.analyze_wave(data),
            role,
            expected_frame_count=expected_frame_count,
            require_all_command_windows=require_all_command_windows,
        )
        bindings.append(binding)
        waves.append(data)
        analyses.append(analysis)
    if waves[0] != waves[1] or bindings[0]["sha256"] != bindings[1]["sha256"]:
        raise ValueError(f"{role} repeated renders are not byte-identical")
    if analyses[0] != analyses[1]:
        raise ValueError(f"{role} repeated onset analyses differ")
    return bindings, waves[0], analyses[0]


def validate_candidate_render_log(root: Path, index: int) -> dict:
    relative = f"delayed-retrigger/sampulse-candidate-render-{index}.log"
    path = child(root, relative)
    summaries: list[dict] = []
    for raw_line in path.read_bytes().splitlines():
        if not raw_line.strip():
            continue
        try:
            line = raw_line.decode("utf-8").strip()
        except UnicodeDecodeError:
            continue
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(value, dict) and value.get("schema_version") == 1:
            summaries.append(value)
    if len(summaries) != 1:
        raise ValueError(
            "same-witness candidate render log lacks one renderer JSON summary"
        )

    summary = summaries[0]
    final_play_beat = summary.get("final_play_beat")
    if (
        summary.get("fixed_frame_render") is not True
        or summary.get("sample_rate") != 44100
        or summary.get("channels") != 1
        or summary.get("bits_per_sample") != 16
        or summary.get("target_beats") != CANDIDATE_TARGET_BEATS
        or summary.get("target_frames") != CANDIDATE_TARGET_FRAMES
        or summary.get("threads") != 1
        or summary.get("sequencer_work_calls") != 1
        or summary.get("player_work_direct") is not False
        or not isinstance(summary.get("renderer_source_sha256"), str)
        or len(summary["renderer_source_sha256"]) != 64
        or not isinstance(summary.get("renderer_project_sha256"), str)
        or len(summary["renderer_project_sha256"]) != 64
        or not isinstance(summary.get("engine_sequencer_sha256"), str)
        or len(summary["engine_sequencer_sha256"]) != 64
        or not isinstance(summary.get("engine_psy3_loader_sha256"), str)
        or len(summary["engine_psy3_loader_sha256"]) != 64
        or not isinstance(summary.get("engine_xmsampler_sha256"), str)
        or len(summary["engine_xmsampler_sha256"]) != 64
        or summary.get("master_buffer_float_count")
        != 2 * CANDIDATE_TARGET_FRAMES
        or not isinstance(final_play_beat, (int, float))
        or isinstance(final_play_beat, bool)
        or not math.isfinite(float(final_play_beat))
        or abs(float(final_play_beat) - CANDIDATE_TARGET_BEATS) > 0.001
    ):
        raise ValueError(
            "same-witness candidate render log does not substantiate "
            "the fixed-frame single-thread procedure"
        )
    return summary


def validate_candidate_render_logs(root: Path) -> list[dict]:
    observations = [
        validate_candidate_render_log(root, index)
        for index in (1, 2)
    ]
    if observations[0] != observations[1]:
        raise ValueError(
            "same-witness candidate renderer procedure observations differ"
        )
    return observations


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    fixture = child(root, FIXTURE)
    raw = fixture.read_bytes()
    fixture_identity = validate_fixture_identity(raw)

    render_observations = validate_candidate_render_logs(root)
    renders, wave, analysis = validate_pair_of_waves(
        root,
        "candidate-delayed-retrigger-sampulse-runtime",
        "candidate",
        expected_frame_count=CANDIDATE_TARGET_FRAMES,
        require_all_command_windows=True,
    )
    if any(
        observation["target_frames"] != analysis["frame_count"]
        for observation in render_observations
    ):
        raise ValueError(
            "candidate renderer summary target does not match WAV frame count"
        )
    reviewed_provenance = validate_renderer_build_provenance(
        root, render_observations
    )
    receipt = {
        "schema_version": 1,
        "phase": "6C",
        "scope": "candidate-runtime-execution-observation",
        "contract": CONTRACT,
        "evidence_role": "candidate",
        "fixture": FIXTURE,
        "fixture_sha256": digest(raw),
        "fixture_identity": fixture_identity,
        "song_title": TITLE,
        "machine_substrate": "XMSampler/Sampulse",
        "command_layout": base.EXPECTED_LAYOUT["commands"],
        "render_procedure": CANDIDATE_RENDER_PROCEDURE,
        "render_observations": render_observations,
        "renderer_provenance": {
            **reviewed_provenance,
            "eins_converter_log": artifact_binding(
                root, "delayed-retrigger/sampulse-eins-compat.log"
            ),
            "render_logs": [
                artifact_binding(
                    root, "delayed-retrigger/sampulse-candidate-render-1.log"
                ),
                artifact_binding(
                    root, "delayed-retrigger/sampulse-candidate-render-2.log"
                ),
            ],
        },
        "renders": renders,
        "render_sha256": digest(wave),
        "analysis": analysis,
        "runtime_command_execution_observed": True,
        "timing_interpretation": "deferred",
        "parity_status": "UNKNOWN",
    }
    write_new(root / CANDIDATE_RECEIPT, receipt)
    return receipt


def validate_candidate(root: Path) -> dict:
    root = root.resolve()
    receipt = read_json(root / CANDIDATE_RECEIPT)
    fixture = child(root, FIXTURE)
    fixture_identity = validate_fixture_identity(fixture.read_bytes())
    if (
        receipt.get("schema_version") != 1
        or receipt.get("phase") != "6C"
        or receipt.get("contract") != CONTRACT
        or receipt.get("evidence_role") != "candidate"
        or receipt.get("fixture") != FIXTURE
        or receipt.get("fixture_sha256") != digest(fixture.read_bytes())
        or receipt.get("fixture_identity") != fixture_identity
        or receipt.get("song_title") != TITLE
        or receipt.get("machine_substrate") != "XMSampler/Sampulse"
        or receipt.get("command_layout") != base.EXPECTED_LAYOUT["commands"]
        or receipt.get("render_procedure") != CANDIDATE_RENDER_PROCEDURE
        or receipt.get("runtime_command_execution_observed") is not True
        or receipt.get("timing_interpretation") != "deferred"
        or receipt.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("same-witness candidate receipt identity mismatch")
    provenance = receipt.get("renderer_provenance")
    if not isinstance(provenance, dict):
        raise ValueError("same-witness candidate renderer provenance is missing")
    expected_single = {
        "binary": f"{NAME}/phase6c-delayed-retrigger-sampulse-render",
        "source": f"{NAME}/render-probe.cpp",
        "project": f"{NAME}/render-probe.pro",
        "fixture_generator_source": f"{NAME}/fixture-generator.c",
        "eins_converter_source": f"{NAME}/eins-compat.py",
        "build_header": f"{NAME}/renderer-build-provenance.hpp",
        "build_attestation": f"{NAME}/renderer-build-provenance.json",
        "qmake_log": "delayed-retrigger/sampulse-render-qmake.log",
        "build_log": "delayed-retrigger/sampulse-render-build.log",
        "provenance_challenge_log": "delayed-retrigger/sampulse-render-provenance.log",
        "renderer_symbols": "delayed-retrigger/sampulse-render-symbols.log",
        "renderer_main_disassembly": "delayed-retrigger/sampulse-render-main-disassembly.log",
        "eins_converter_log": "delayed-retrigger/sampulse-eins-compat.log",
    }
    for key, expected_path in expected_single.items():
        validate_artifact_binding(root, provenance.get(key), expected_path)
    logs = provenance.get("render_logs")
    if not isinstance(logs, list) or len(logs) != 2:
        raise ValueError("same-witness candidate render-log provenance is invalid")
    for index, value in enumerate(logs, start=1):
        validate_artifact_binding(
            root,
            value,
            f"delayed-retrigger/sampulse-candidate-render-{index}.log",
        )
    render_observations = validate_candidate_render_logs(root)
    reviewed_provenance = validate_renderer_build_provenance(
        root, render_observations
    )
    for key, value in reviewed_provenance.items():
        if provenance.get(key) != value:
            raise ValueError(
                "same-witness candidate reviewed renderer provenance mismatch"
            )
    if receipt.get("render_observations") != render_observations:
        raise ValueError(
            "same-witness candidate renderer observation binding mismatch"
        )

    renders, wave, analysis = validate_pair_of_waves(
        root,
        "candidate-delayed-retrigger-sampulse-runtime",
        "candidate",
        expected_frame_count=CANDIDATE_TARGET_FRAMES,
        require_all_command_windows=True,
    )
    if any(
        observation["target_frames"] != analysis["frame_count"]
        for observation in render_observations
    ):
        raise ValueError(
            "candidate renderer summary target does not match WAV frame count"
        )
    if (
        receipt.get("renders") != renders
        or receipt.get("render_sha256") != digest(wave)
        or receipt.get("analysis") != analysis
    ):
        raise ValueError("same-witness candidate render binding mismatch")
    return receipt


def validate_original_event_binding(attempt: dict) -> None:
    runtime_id = attempt.get("selected_render_dialog_runtime_id")
    preexisting_count = attempt.get("preexisting_render_dialog_count")
    post_dispatch_count = attempt.get("render_dialog_post_dispatch_event_count")
    observed_window_count = attempt.get(
        "render_dialog_post_dispatch_observed_window_event_count"
    )
    unresolved_event_count = attempt.get(
        "render_dialog_unresolved_post_dispatch_event_count"
    )
    boundary_tick = attempt.get("render_dialog_dispatch_boundary_tick")
    selected_handle = attempt.get("selected_render_dialog_native_handle")
    if (
        attempt.get("render_dialog_native_event_hook_armed") is not True
        or attempt.get("render_dialog_event_message_pump_started") is not True
        or attempt.get("render_dialog_dispatch_boundary_set") is not True
        or not isinstance(boundary_tick, int)
        or isinstance(boundary_tick, bool)
        or boundary_tick < 0
        or boundary_tick > 0xFFFFFFFF
        or attempt.get("dialog_discovery")
        != "pumped-win-event-object-show-strictly-after-dispatch-tick"
        or not isinstance(preexisting_count, int)
        or isinstance(preexisting_count, bool)
        or preexisting_count < 0
        or not isinstance(observed_window_count, int)
        or isinstance(observed_window_count, bool)
        or observed_window_count != 1
        or not isinstance(unresolved_event_count, int)
        or isinstance(unresolved_event_count, bool)
        or unresolved_event_count != 0
        or not isinstance(post_dispatch_count, int)
        or isinstance(post_dispatch_count, bool)
        or post_dispatch_count != 1
        or not isinstance(selected_handle, int)
        or isinstance(selected_handle, bool)
        or selected_handle <= 0
        or not isinstance(runtime_id, list)
        or not runtime_id
        or any(
            not isinstance(value, int) or isinstance(value, bool)
            for value in runtime_id
        )
    ):
        raise ValueError(
            "same-witness original render lacks bound post-dispatch dialog evidence"
        )

AMBIGUITY_DIAGNOSTICS = (
    "unresolved post-dispatch Psycle window-show event observed",
    "multiple post-dispatch Psycle window-show events observed",
    "multiple post-dispatch Render as Wav File windows observed",
)


def validate_original_ambiguous_event_binding(attempt: dict) -> None:
    observed_window_count = attempt.get(
        "render_dialog_post_dispatch_observed_window_event_count"
    )
    unresolved_event_count = attempt.get(
        "render_dialog_unresolved_post_dispatch_event_count"
    )
    post_dispatch_count = attempt.get("render_dialog_post_dispatch_event_count")
    boundary_tick = attempt.get("render_dialog_dispatch_boundary_tick")
    preexisting_count = attempt.get("preexisting_render_dialog_count")
    diagnostics = attempt.get("diagnostics")
    if (
        attempt.get("render_dialog_native_event_hook_armed") is not True
        or attempt.get("render_dialog_event_message_pump_started") is not True
        or attempt.get("render_dialog_dispatch_boundary_set") is not True
        or not isinstance(boundary_tick, int)
        or isinstance(boundary_tick, bool)
        or boundary_tick < 0
        or boundary_tick > 0xFFFFFFFF
        or not isinstance(preexisting_count, int)
        or isinstance(preexisting_count, bool)
        or preexisting_count < 0
        or not isinstance(observed_window_count, int)
        or isinstance(observed_window_count, bool)
        or observed_window_count < 0
        or not isinstance(unresolved_event_count, int)
        or isinstance(unresolved_event_count, bool)
        or unresolved_event_count < 0
        or not isinstance(post_dispatch_count, int)
        or isinstance(post_dispatch_count, bool)
        or post_dispatch_count < 0
        or not isinstance(diagnostics, list)
        or not diagnostics
        or any(not isinstance(value, str) for value in diagnostics)
    ):
        raise ValueError(
            "same-witness inconclusive render lacks ambiguity evidence"
        )

    ambiguous = (
        observed_window_count > 1
        or unresolved_event_count > 0
        or post_dispatch_count > 1
    )
    diagnostic_matches = any(
        marker in diagnostic
        for diagnostic in diagnostics
        for marker in AMBIGUITY_DIAGNOSTICS
    )
    if not ambiguous or not diagnostic_matches:
        raise ValueError(
            "same-witness inconclusive render does not prove "
            "ambiguous post-dispatch event binding"
        )


def validate_original_observed_output(
    original_root: Path, attempt: dict, index: int
) -> dict | None:
    expected_name = f"original-delayed-retrigger-sampulse-runtime-{index}.wav"
    expected_relative = f"{NAME}/{expected_name}"
    expected_path = child(original_root, expected_relative)
    observed = attempt.get("observed_output")
    if observed is None:
        if expected_path.exists():
            raise ValueError(
                "same-witness inconclusive render created an unbound output file"
            )
        return None
    if (
        not isinstance(observed, dict)
        or set(observed) != {"path", "size_bytes", "sha256"}
        or observed.get("path") != expected_name
        or not isinstance(observed.get("size_bytes"), int)
        or isinstance(observed.get("size_bytes"), bool)
        or observed["size_bytes"] < 0
        or not isinstance(observed.get("sha256"), str)
        or len(observed["sha256"]) != 64
    ):
        raise ValueError(
            "same-witness inconclusive observed output binding is invalid"
        )
    data = expected_path.read_bytes()
    if (
        len(data) != observed["size_bytes"]
        or digest(data) != observed["sha256"]
    ):
        raise ValueError(
            "same-witness inconclusive observed output binding mismatch"
        )
    return {
        "path": expected_relative,
        "size_bytes": observed["size_bytes"],
        "sha256": observed["sha256"],
    }


def validate_original_inconclusive_runtime(
    original_root: Path, runtime: object
) -> dict:
    if not isinstance(runtime, dict):
        raise ValueError("same-witness original runtime receipt is missing")
    attempts = runtime.get("attempts")
    renders = runtime.get("renders")
    pre = runtime.get("pre_render_load")
    diagnostics = runtime.get("diagnostics")
    pre_render_failure_diagnostic = (
        "clean accepted load and dismissed Load Warning are required before "
        "same-witness Sampulse rendering"
    )
    if (
        runtime.get("schema_version") == 1
        and runtime.get("outcome") == "inconclusive"
        and runtime.get("deterministic") is False
        and runtime.get("settings") == ORIGINAL_RENDER_SETTINGS
        and attempts == []
        and renders == []
        and diagnostics == [pre_render_failure_diagnostic]
        and isinstance(pre, dict)
        and pre.get("schema_version") == 1
        and isinstance(pre.get("clean_accepted_load"), bool)
        and isinstance(pre.get("load_warning_dismissed"), bool)
        and isinstance(pre.get("process_running_before_render"), bool)
        and isinstance(pre.get("stable_marker_polls"), int)
        and not isinstance(pre.get("stable_marker_polls"), bool)
        and pre["stable_marker_polls"] >= 0
        and (
            pre.get("matched_marker") is None
            or isinstance(pre.get("matched_marker"), str)
        )
        and not (
            pre.get("clean_accepted_load") is True
            and pre.get("load_warning_dismissed") is True
        )
    ):
        for index in (1, 2):
            unexpected = child(
                original_root,
                f"{NAME}/original-delayed-retrigger-sampulse-runtime-{index}.wav",
            )
            if unexpected.exists():
                raise ValueError(
                    "same-witness failed pre-render load created unexpected output"
                )
        return {
            "inconclusive_reason": "pre-render-load-not-accepted",
            "binding_error": None,
            "diagnostics": diagnostics,
            "pre_render_load": pre,
            "observed_output": None,
            "retained_renders": [],
            "retained_render_analyses": [],
        }
    if (
        runtime.get("schema_version") != 1
        or runtime.get("outcome") != "inconclusive"
        or runtime.get("deterministic") is not False
        or runtime.get("settings") != ORIGINAL_RENDER_SETTINGS
        or not isinstance(attempts, list)
        or not isinstance(renders, list)
        or len(attempts) not in (1, 2)
        or len(renders) > 2
        or len(renders) > len(attempts)
        or not isinstance(pre, dict)
        or pre.get("schema_version") != 1
        or pre.get("clean_accepted_load") is not True
        or pre.get("load_warning_dismissed") is not True
        or pre.get("process_running_before_render") is not True
        or pre.get("matched_marker") != Path(FIXTURE).name
        or not isinstance(pre.get("stable_marker_polls"), int)
        or isinstance(pre.get("stable_marker_polls"), bool)
        or pre["stable_marker_polls"] < 4
    ):
        raise ValueError("same-witness inconclusive original runtime mismatch")

    retained_renders: list[dict] = []
    retained_analyses: list[dict] = []

    # Both renders may complete yet disagree byte-for-byte. That is valid
    # nondeterminism evidence, not a malformed receipt and not a runtime pair.
    if len(renders) == 2:
        if len(attempts) != 2:
            raise ValueError(
                "same-witness nondeterministic render-pair shape mismatch"
            )
        for index in (0, 1):
            binding, data = validate_original_attempt(
                original_root, attempts[index], index + 1
            )
            if renders[index] != binding:
                raise ValueError(
                    "same-witness nondeterministic render binding mismatch"
                )
            retained_renders.append(binding)
            retained_analyses.append(base.analyze_wave(data))
        if retained_renders[0]["sha256"] == retained_renders[1]["sha256"]:
            raise ValueError(
                "same-witness inconclusive pair is actually byte-identical"
            )
        return {
            "inconclusive_reason": "nondeterministic-completed-render-pair",
            "binding_error": None,
            "diagnostics": [],
            "observed_output": None,
            "retained_renders": retained_renders,
            "retained_render_analyses": retained_analyses,
        }

    # A finalized first render with failed dialog teardown must not trigger a
    # second modal render. Retain the completed render and stop at UNKNOWN.
    if len(renders) == 1 and len(attempts) == 1:
        binding, data = validate_original_attempt(
            original_root, attempts[0], 1
        )
        if renders[0] != binding:
            raise ValueError(
                "same-witness teardown-failure render binding mismatch"
            )
        teardown_diagnostic = (
            "render output finalized and Close control was verified, "
            "but dialog teardown did not complete"
        )
        if (
            attempts[0].get("dialog_closed") is not False
            or attempts[0].get("diagnostics") != [teardown_diagnostic]
        ):
            raise ValueError(
                "same-witness one-render inconclusive state lacks teardown failure"
            )
        retained_renders.append(binding)
        retained_analyses.append(base.analyze_wave(data))
        observed_binding = None
        if attempts[0].get("observed_output") is not None:
            observed_binding = validate_original_observed_output(
                original_root, attempts[0], 1
            )
        return {
            "inconclusive_reason": "first-render-dialog-teardown-failure",
            "binding_error": None,
            "diagnostics": attempts[0].get("diagnostics"),
            "observed_output": observed_binding,
            "retained_renders": retained_renders,
            "retained_render_analyses": retained_analyses,
        }

    # Otherwise the last attempt is a non-evidentiary interruption after zero
    # or one completed render and must prove ambiguous event binding.
    if len(renders) > 1 or len(attempts) != len(renders) + 1:
        raise ValueError("same-witness interrupted original runtime mismatch")
    for index in range(len(renders)):
        binding, data = validate_original_attempt(
            original_root, attempts[index], index + 1
        )
        if renders[index] != binding:
            raise ValueError(
                "same-witness inconclusive retained render binding mismatch"
            )
        retained_renders.append(binding)
        retained_analyses.append(base.analyze_wave(data))

    attempt = attempts[-1]
    if not isinstance(attempt, dict):
        raise ValueError("same-witness inconclusive render attempt is not an object")
    for key in ("command_verified", "command_dispatched"):
        if attempt.get(key) is not True:
            raise ValueError(
                f"same-witness inconclusive render did not verify {key}"
            )
    for key in ("dialog_verified", "controls_configured", "save_invoked"):
        if not isinstance(attempt.get(key), bool):
            raise ValueError(
                f"same-witness inconclusive render has invalid {key}"
            )
    diagnostics = attempt.get("diagnostics")
    if (
        attempt.get("outcome") != "inconclusive"
        or attempt.get("process_exited") is not False
        or attempt.get("process_exit_code") is not None
        or attempt.get("output") is not None
        or not isinstance(diagnostics, list)
        or not diagnostics
    ):
        raise ValueError("same-witness inconclusive render shape mismatch")

    try:
        validate_original_event_binding(attempt)
    except ValueError as exc:
        binding_error = str(exc)
    else:
        raise ValueError(
            "same-witness inconclusive render has valid post-dispatch dialog binding"
        )

    validate_original_ambiguous_event_binding(attempt)
    observed_binding = validate_original_observed_output(
        original_root, attempt, len(attempts)
    )

    return {
        "inconclusive_reason": "ambiguous-final-render-attempt",
        "binding_error": binding_error,
        "diagnostics": diagnostics,
        "observed_output": observed_binding,
        "retained_renders": retained_renders,
        "retained_render_analyses": retained_analyses,
    }

def inconclusive_render_binding_status(quarantine: dict) -> str:
    if quarantine.get("inconclusive_reason") == "pre-render-load-not-accepted":
        return "not-attempted"
    return "accepted" if quarantine.get("binding_error") is None else "rejected"


def validate_original_process_exit_runtime(
    original_root: Path, runtime: object, receipt: dict
) -> dict:
    if not isinstance(runtime, dict):
        raise ValueError("same-witness original runtime receipt is missing")
    attempts = runtime.get("attempts")
    renders = runtime.get("renders")
    pre = runtime.get("pre_render_load")
    if (
        runtime.get("schema_version") != 1
        or runtime.get("outcome")
        != "reference-process-exited-during-render"
        or runtime.get("deterministic") is not False
        or runtime.get("settings") != ORIGINAL_RENDER_SETTINGS
        or not isinstance(attempts, list)
        or not isinstance(renders, list)
        or len(renders) > 1
        or len(attempts) != len(renders) + 1
        or len(attempts) not in (1, 2)
        or not isinstance(pre, dict)
        or pre.get("schema_version") != 1
        or pre.get("clean_accepted_load") is not True
        or pre.get("load_warning_dismissed") is not True
        or pre.get("process_running_before_render") is not True
        or pre.get("matched_marker") != Path(FIXTURE).name
        or not isinstance(pre.get("stable_marker_polls"), int)
        or isinstance(pre.get("stable_marker_polls"), bool)
        or pre["stable_marker_polls"] < 4
    ):
        raise ValueError("same-witness process-exit runtime mismatch")

    retained_renders: list[dict] = []
    retained_analyses: list[dict] = []
    for index in range(len(renders)):
        binding, data = validate_original_attempt(
            original_root, attempts[index], index + 1
        )
        if renders[index] != binding:
            raise ValueError(
                "same-witness process-exit retained render binding mismatch"
            )
        retained_renders.append(binding)
        retained_analyses.append(base.analyze_wave(data))

    attempt = attempts[-1]
    if not isinstance(attempt, dict):
        raise ValueError("same-witness process-exit attempt is not an object")
    for key in (
        "command_verified",
        "command_dispatched",
        "dialog_verified",
        "controls_configured",
        "save_invoked",
    ):
        if attempt.get(key) is not True:
            raise ValueError(
                f"same-witness process-exit render did not verify {key}"
            )
    exit_code = attempt.get("process_exit_code")
    diagnostics = attempt.get("diagnostics")
    if (
        attempt.get("outcome") != "inconclusive"
        or attempt.get("process_exited") is not True
        or not isinstance(exit_code, int)
        or isinstance(exit_code, bool)
        or attempt.get("output") is not None
        or not isinstance(diagnostics, list)
        or "reference exited during offline render" not in diagnostics
        or receipt.get("exit_code_before_termination") != exit_code
    ):
        raise ValueError("same-witness process-exit evidence mismatch")

    try:
        validate_original_event_binding(attempt)
    except ValueError as exc:
        binding_error = str(exc)
        validate_original_ambiguous_event_binding(attempt)
    else:
        binding_error = None

    observed_binding = validate_original_observed_output(
        original_root, attempt, len(attempts)
    )
    return {
        "binding_error": binding_error,
        "diagnostics": diagnostics,
        "observed_output": observed_binding,
        "process_exit_code": exit_code,
        "retained_renders": retained_renders,
        "retained_render_analyses": retained_analyses,
    }


def validate_original_runtime_procedure(runtime: object) -> list[dict]:
    if not isinstance(runtime, dict):
        raise ValueError("same-witness original runtime receipt is missing")
    attempts = runtime.get("attempts")
    pre = runtime.get("pre_render_load")
    if (
        runtime.get("schema_version") != 1
        or runtime.get("outcome") != "rendered-twice"
        or runtime.get("deterministic") is not True
        or runtime.get("settings") != ORIGINAL_RENDER_SETTINGS
        or not isinstance(attempts, list)
        or len(attempts) != 2
        or not isinstance(pre, dict)
        or pre.get("schema_version") != 1
        or pre.get("clean_accepted_load") is not True
        or pre.get("load_warning_dismissed") is not True
        or pre.get("process_running_before_render") is not True
        or pre.get("matched_marker") != Path(FIXTURE).name
        or not isinstance(pre.get("stable_marker_polls"), int)
        or isinstance(pre.get("stable_marker_polls"), bool)
        or pre["stable_marker_polls"] < 4
    ):
        raise ValueError("same-witness original render procedure mismatch")
    return attempts


def validate_original_attempt(
    original_root: Path, attempt: object, index: int
) -> tuple[dict, bytes]:
    if not isinstance(attempt, dict):
        raise ValueError("same-witness original render attempt is not an object")
    for key in (
        "command_verified",
        "command_dispatched",
        "dialog_verified",
        "controls_configured",
        "save_invoked",
    ):
        if attempt.get(key) is not True:
            raise ValueError(f"same-witness original render did not verify {key}")

    validate_original_event_binding(attempt)

    diagnostics = attempt.get("diagnostics")
    teardown_diagnostic = (
        "render output finalized and Close control was verified, "
        "but dialog teardown did not complete"
    )
    completed_and_closed = (
        attempt.get("dialog_closed") is True and diagnostics == []
    )
    completed_with_teardown_failure = (
        attempt.get("dialog_closed") is False
        and attempt.get("close_control_seen") is True
        and attempt.get("close_uia_invoked") is True
        and diagnostics == [teardown_diagnostic]
    )
    if (
        attempt.get("outcome") != "rendered"
        or attempt.get("process_exited") is not False
        or attempt.get("process_exit_code") is not None
        or not isinstance(attempt.get("stable_output_polls"), int)
        or isinstance(attempt.get("stable_output_polls"), bool)
        or attempt["stable_output_polls"] < 4
        or not (completed_and_closed or completed_with_teardown_failure)
    ):
        raise ValueError("same-witness original render did not complete cleanly")
    expected_name = f"original-delayed-retrigger-sampulse-runtime-{index}.wav"
    output = attempt.get("output")
    if (
        not isinstance(output, dict)
        or output.get("path") != expected_name
        or not isinstance(output.get("sha256"), str)
    ):
        raise ValueError("same-witness original output binding is invalid")
    relative = f"{NAME}/{expected_name}"
    data = child(original_root, relative).read_bytes()
    if digest(data) != output["sha256"]:
        raise ValueError("same-witness original output hash mismatch")
    return {"path": relative, "sha256": output["sha256"]}, data


def validate_original(candidate_root: Path, original_root: Path) -> dict:
    candidate_root = candidate_root.resolve()
    original_root = original_root.resolve()
    candidate = validate_candidate(candidate_root)

    gate = original_gate_module.delayed_validator()
    generic_result = gate.validate_pair(
        NAME, CONTRACT, candidate_root, original_root
    )
    receipt = read_json(original_root / ORIGINAL_RECEIPT)
    runtime = receipt.get("runtime_execution")
    runtime_outcome = (
        runtime.get("outcome") if isinstance(runtime, dict) else None
    )
    if (
        receipt.get("schema_version") != 1
        or receipt.get("phase") != "6C"
        or receipt.get("contract") != CONTRACT
        or receipt.get("evidence_role") != "original"
        or receipt.get("reference_build") != REFERENCE_BUILD
        or receipt.get("fixture_sha256") != candidate["fixture_sha256"]
        or receipt.get("original_psycle_observed") is not True
        or receipt.get("parity_status") != "UNKNOWN"
    ):
        raise ValueError("same-witness original receipt identity mismatch")

    if runtime_outcome == "rendered-twice":
        if (
            generic_result != "accepted"
            or receipt.get("load_result") != "accepted"
        ):
            raise ValueError(
                "same-witness completed original load-result mismatch"
            )
    elif runtime_outcome == "reference-process-exited-during-render":
        if (
            generic_result != "inconclusive"
            or receipt.get("load_result") != "inconclusive"
            or receipt.get("observation")
            != "reference-process-exited-before-harness-termination"
        ):
            raise ValueError(
                "same-witness process-exit original load-result mismatch"
            )
    elif runtime_outcome == "inconclusive":
        if (
            generic_result != receipt.get("load_result")
            or generic_result not in {"accepted", "inconclusive"}
        ):
            raise ValueError(
                "same-witness inconclusive original load-result mismatch"
            )

    if runtime_outcome == "reference-process-exited-during-render":
        crash = validate_original_process_exit_runtime(
            original_root, runtime, receipt
        )
        original_analysis = {
            "schema_version": 1,
            "phase": "6C",
            "contract": CONTRACT,
            "evidence_role": "original-runtime",
            "reference_build": REFERENCE_BUILD,
            "fixture": candidate["fixture"],
            "fixture_sha256": candidate["fixture_sha256"],
            "machine_substrate": "XMSampler/Sampulse",
            "outcome": "reference-process-exited-during-render",
            "renders": crash["retained_renders"],
            "retained_render_analyses": crash["retained_render_analyses"],
            "render_sha256": None,
            "analysis": None,
            "runtime_command_execution_observed": False,
            "process_exit_code": crash["process_exit_code"],
            "fresh_render_event_binding": (
                "accepted" if crash["binding_error"] is None else "rejected"
            ),
            "fresh_render_event_binding_error": crash["binding_error"],
            "observed_output": crash["observed_output"],
            "diagnostics": crash["diagnostics"],
            "timing_interpretation": "deferred",
            "parity_status": "UNKNOWN",
        }
        write_new(original_root / ORIGINAL_ANALYSIS, original_analysis)

        comparison = {
            "schema_version": 1,
            "phase": "6C",
            "contract": CONTRACT,
            "fixture_sha256": candidate["fixture_sha256"],
            "same_fixture_bytes": True,
            "same_onset_analyzer": (
                "phase6c-delayed-retrigger-render-evidence.py::analyze_wave"
            ),
            "candidate": {
                "render_sha256": candidate["render_sha256"],
                "analysis": candidate["analysis"],
                "runtime_command_execution_observed": True,
            },
            "original": {
                "reference_build": REFERENCE_BUILD,
                "outcome": "reference-process-exited-during-render",
                "retained_renders": crash["retained_renders"],
                "retained_render_analyses": crash["retained_render_analyses"],
                "render_sha256": None,
                "analysis": None,
                "runtime_command_execution_observed": False,
                "process_exit_code": crash["process_exit_code"],
                "observed_output": crash["observed_output"],
            },
            "command_bearing_runtime_pair_observed": False,
            "exact_onset_timing_interpretation": "deferred",
            "classification_allowed": False,
            "parity_status": "UNKNOWN",
            "interpretation_boundary": (
                "The pinned original exited during the source-pinned Save Wave "
                "procedure. Exact exit and any partial-output evidence are "
                "retained diagnostically, but no deterministic original runtime "
                "render pair or delayed/retrigger parity is claimed."
            ),
        }
        write_new(original_root / COMPARISON, comparison)
        return comparison

    if isinstance(runtime, dict) and runtime.get("outcome") == "inconclusive":
        quarantine = validate_original_inconclusive_runtime(
            original_root, runtime
        )
        binding_status = inconclusive_render_binding_status(quarantine)
        original_analysis = {
            "schema_version": 1,
            "phase": "6C",
            "contract": CONTRACT,
            "evidence_role": "original-runtime",
            "reference_build": REFERENCE_BUILD,
            "fixture": candidate["fixture"],
            "fixture_sha256": candidate["fixture_sha256"],
            "machine_substrate": "XMSampler/Sampulse",
            "outcome": "inconclusive",
            "inconclusive_reason": quarantine["inconclusive_reason"],
            "renders": quarantine["retained_renders"],
            "retained_render_analyses": quarantine["retained_render_analyses"],
            "render_sha256": None,
            "analysis": None,
            "runtime_command_execution_observed": False,
            "fresh_render_event_binding": binding_status,
            "fresh_render_event_binding_error": quarantine["binding_error"],
            "observed_output": quarantine["observed_output"],
            "diagnostics": quarantine["diagnostics"],
            "timing_interpretation": "deferred",
            "parity_status": "UNKNOWN",
        }
        write_new(original_root / ORIGINAL_ANALYSIS, original_analysis)

        comparison = {
            "schema_version": 1,
            "phase": "6C",
            "contract": CONTRACT,
            "fixture_sha256": candidate["fixture_sha256"],
            "same_fixture_bytes": True,
            "same_onset_analyzer": (
                "phase6c-delayed-retrigger-render-evidence.py::analyze_wave"
            ),
            "candidate": {
                "render_sha256": candidate["render_sha256"],
                "analysis": candidate["analysis"],
                "runtime_command_execution_observed": True,
            },
            "original": {
                "reference_build": REFERENCE_BUILD,
                "outcome": "inconclusive",
                "inconclusive_reason": quarantine["inconclusive_reason"],
                "retained_renders": quarantine["retained_renders"],
                "retained_render_analyses": quarantine["retained_render_analyses"],
                "render_sha256": None,
                "analysis": None,
                "runtime_command_execution_observed": False,
                "fresh_render_event_binding": binding_status,
                "fresh_render_event_binding_error": quarantine["binding_error"],
                "observed_output": quarantine["observed_output"],
            },
            "command_bearing_runtime_pair_observed": False,
            "exact_onset_timing_interpretation": "deferred",
            "classification_allowed": False,
            "parity_status": "UNKNOWN",
            "interpretation_boundary": (
                "The candidate rendered the exact four-beat Sampulse/XMSampler "
                "witness, but the pinned original did not complete a deterministic "
                "repeated-render pair. Completed original renders and any terminal "
                "teardown or ambiguity evidence are retained diagnostically; no "
                "cross-side runtime pair is claimed and delayed/retrigger parity "
                "remains unclassified."
            ),
        }
        write_new(original_root / COMPARISON, comparison)
        return comparison

    attempts = validate_original_runtime_procedure(runtime)

    first_binding, first = validate_original_attempt(original_root, attempts[0], 1)
    second_binding, second = validate_original_attempt(original_root, attempts[1], 2)
    if first != second or first_binding["sha256"] != second_binding["sha256"]:
        raise ValueError("same-witness original renders are not byte-identical")
    expected_runtime_renders = [first_binding, second_binding]
    if runtime.get("renders") != expected_runtime_renders:
        raise ValueError("same-witness original runtime render bindings mismatch")
    analysis = validate_same_witness_analysis(
        base.analyze_wave(first),
        "original",
        require_all_command_windows=True,
    )
    second_analysis = validate_same_witness_analysis(
        base.analyze_wave(second),
        "original",
        require_all_command_windows=True,
    )
    if analysis != second_analysis:
        raise ValueError("same-witness original onset analyses differ")

    original_analysis = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "evidence_role": "original-runtime",
        "reference_build": REFERENCE_BUILD,
        "fixture": candidate["fixture"],
        "fixture_sha256": candidate["fixture_sha256"],
        "machine_substrate": "XMSampler/Sampulse",
        "renders": [first_binding, second_binding],
        "render_sha256": digest(first),
        "analysis": analysis,
        "runtime_command_execution_observed": True,
        "timing_interpretation": "deferred",
        "parity_status": "UNKNOWN",
    }
    write_new(original_root / ORIGINAL_ANALYSIS, original_analysis)

    comparison = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "fixture_sha256": candidate["fixture_sha256"],
        "same_fixture_bytes": True,
        "same_onset_analyzer": "phase6c-delayed-retrigger-render-evidence.py::analyze_wave",
        "candidate": {
            "render_sha256": candidate["render_sha256"],
            "analysis": candidate["analysis"],
            "runtime_command_execution_observed": True,
        },
        "original": {
            "reference_build": REFERENCE_BUILD,
            "render_sha256": original_analysis["render_sha256"],
            "analysis": original_analysis["analysis"],
            "runtime_command_execution_observed": True,
        },
        "command_bearing_runtime_pair_observed": True,
        "exact_onset_timing_interpretation": "deferred",
        "classification_allowed": False,
        "parity_status": "UNKNOWN",
        "interpretation_boundary": (
            "Both sides rendered the exact same four-beat Sampulse/XMSampler witness "
            "with the same command geometry and the same onset analyzer. This completes "
            "the command-bearing runtime evidence pair. Exact onset timing is retained "
            "for the next evidence rung and is not classified in this receipt."
        ),
    }
    write_new(original_root / COMPARISON, comparison)
    return comparison


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("candidate", "candidate-check", "original"))
    parser.add_argument("root", type=Path)
    parser.add_argument("other", type=Path, nargs="?")
    args = parser.parse_args()
    if args.mode == "candidate":
        if args.other is not None:
            parser.error("candidate accepts only ROOT")
        value = collect_candidate(args.root)
    elif args.mode == "candidate-check":
        if args.other is not None:
            parser.error("candidate-check accepts only ROOT")
        value = validate_candidate(args.root)
    else:
        if args.other is None:
            parser.error("original requires ORIGINAL_ROOT")
        value = validate_original(args.root, args.other)
    print(json.dumps(value, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
