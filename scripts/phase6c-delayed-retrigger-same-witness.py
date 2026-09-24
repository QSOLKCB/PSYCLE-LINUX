#!/usr/bin/env python3
"""Collect and compare the Phase 6C same-witness Sampulse runtime renders."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import os
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
EXPECTED_SNGI_PAYLOAD_SHA256 = (
    "66af0fbe63b11d37590af5bfaf90ff15d999fbb30d52409f529ba88f51ed19ad"
)
EXPECTED_XMSAMPLER_MACD_SHA256 = (
    "7bcba4bfa2f423e20c90f76c48eeca9c44962161b174ee1e0e344f065b2caaf9"
)
EXPECTED_XMSAMPLER_SPECIFIC_SHA256 = (
    "7690f43c027f3fd936cd2a50158b0b4664adfcf7bc1d414ab436821b8561fe03"
)
EXPECTED_XMSAMPLER_EXTENSION_SHA256 = (
    "5743fcdda132127999b7ac1571fb76e6829114fdf143ab4075e5f7d585cff677"
)
STRICT_ANALYZER_ID = "phase6c-delayed-retrigger-render-evidence.py::analyze_wave"
RELAXED_ANALYZER_ID = "phase6c-delayed-retrigger-same-witness.py::analyze_wave_observation"
RUNTIME_COMMAND_EXECUTION_SCOPE = {
    "established_effects": [
        "FB 3F retrigger",
        "FA 42 retrigger-continue",
    ],
    "criterion": (
        "multiple distinct onsets in both the FB beat-1 and FA beat-2 windows"
    ),
    "not_established": [
        "FD 7F note-delay effect",
        "FE 04 extended-command effect",
    ],
}
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
EXPECTED_PSY3_FILE_VERSION = 0x11
TITLE = "PSYCLE-LINUX Phase 6C delayed/retrigger Sampulse execution witness"
EXPECTED_INFO_PAYLOAD = (
    TITLE.encode("utf-8") + b"\0Unnamed\0No Comments\0"
)
EXPECTED_INFO_PAYLOAD_SHA256 = (
    "0f7eac3eff49e57435cbdb4deba7e62f098a2f886db1cf230a5f4390b139764e"
)
EXPECTED_TOP_LEVEL_LAYOUT = (
    (b"INFO", 0),
    (b"SNGI", 4),
    (b"SEQD", 2),
    (b"PATD", 2),
    (b"PATD", 0x00010002),
    (b"MACD", 3),
    (b"MACD", 3),
    (b"EINS", 0x00010000),
)
EXPECTED_MASTER_STATE_SHA256 = (
    "4703cfdaa33a2085dfc554c10e572903a2b78fade0de0aa29bac9385be31424c"
)
EXPECTED_MASTER_SPECIFIC_SHA256 = (
    "dcb7f4ba6c9ea763b23ecebc81f74f37a0804d237702338b6a6b26923f47fe0f"
)
EXPECTED_MASTER_EXTENSION_SHA256 = (
    "606dc4d051124dc4ee5c0125210f9271875ba081ce3915b354a0c49b7c7324bb"
)
CANDIDATE_RECEIPT = "candidate-delayed-retrigger-sampulse-runtime.json"
ORIGINAL_RECEIPT = "original-delayed-retrigger-sampulse-runtime.json"
ORIGINAL_ANALYSIS = "original-delayed-retrigger-sampulse-runtime-analysis.json"
COMPARISON = "delayed-retrigger-sampulse-runtime-comparison.json"
RENDER_EXECUTION_REPLAY = (
    "delayed-retrigger-sampulse-runtime/renderer-execution-replay.json"
)
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
    file_version = struct.unpack_from("<I", data, 8)[0]
    if file_version != EXPECTED_PSY3_FILE_VERSION:
        raise ValueError(
            "same-witness Sampulse fixture has noncanonical PSY3 file version"
        )
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


def expected_xmsampler_macd_state_bytes() -> bytes:
    body = bytearray()
    body += struct.pack("<III", 0x00010002, 64, 1)
    for _ in range(128):
        body += struct.pack("<ii", 0, 0)
    body += struct.pack("<BBii", 0, 1, 128, 0)
    channel = (
        b"CHAN"
        + struct.pack("<i", 20)
        + struct.pack("<iiiii", 200, 100, 127, 0, 4)
    )
    body += channel * 64
    body += struct.pack("<I", 1)
    if len(body) != 2842:
        raise AssertionError("canonical XMSampler specific body size changed")

    state = bytearray(struct.pack("<I", len(body)))
    state += body
    state += b"PMAP" + bytes((110,))
    for index in range(110):
        state += struct.pack("<BH", index, index)
    state += b"PBUS\0"
    return bytes(state)


def validate_xmsampler_macd_state(payload: bytes, position: int) -> dict:
    state = payload[position:]
    expected = expected_xmsampler_macd_state_bytes()
    if state != expected:
        raise ValueError(
            "same-witness XMSampler machine-specific MACD state differs from canonical payload"
        )
    if digest(payload) != EXPECTED_XMSAMPLER_MACD_SHA256:
        raise ValueError("same-witness XMSampler complete MACD digest mismatch")

    specific_size = struct.unpack_from("<I", state, 0)[0]
    specific_end = 4 + specific_size
    if (
        specific_size != 2842
        or specific_end > len(state)
        or digest(state[:specific_end]) != EXPECTED_XMSAMPLER_SPECIFIC_SHA256
    ):
        raise ValueError("same-witness XMSampler specific chunk identity mismatch")

    cursor = 4
    version, voices, quality = struct.unpack_from("<III", state, cursor)
    cursor += 12
    zxx = []
    for _ in range(128):
        zxx.append(struct.unpack_from("<ii", state, cursor))
        cursor += 8
    amiga_slides, use_filters = struct.unpack_from("<BB", state, cursor)
    cursor += 2
    global_volume, panning_mode = struct.unpack_from("<ii", state, cursor)
    cursor += 8

    channel_states = []
    for _ in range(64):
        if state[cursor : cursor + 4] != b"CHAN":
            raise ValueError("same-witness XMSampler channel state tag mismatch")
        channel_size = struct.unpack_from("<i", state, cursor + 4)[0]
        if channel_size != 20 or cursor + 8 + channel_size > specific_end:
            raise ValueError("same-witness XMSampler channel state size mismatch")
        values = struct.unpack_from("<iiiii", state, cursor + 8)
        channel_states.append(values)
        cursor += 8 + channel_size
    if cursor + 4 != specific_end:
        raise ValueError("same-witness XMSampler specific parser boundary mismatch")
    instrument_bank = struct.unpack_from("<I", state, cursor)[0]
    cursor += 4

    if (
        version != 0x00010002
        or voices != 64
        or quality != 1
        or any(item != (0, 0) for item in zxx)
        or amiga_slides != 0
        or use_filters != 1
        or global_volume != 128
        or panning_mode != 0
        or any(item != (200, 100, 127, 0, 4) for item in channel_states)
        or instrument_bank != 1
        or cursor != specific_end
    ):
        raise ValueError("same-witness XMSampler loader-consumed state mismatch")

    extension = state[specific_end:]
    if digest(extension) != EXPECTED_XMSAMPLER_EXTENSION_SHA256:
        raise ValueError("same-witness XMSampler modern MACD extension mismatch")
    cursor = 0
    if extension[cursor : cursor + 4] != b"PMAP":
        raise ValueError("same-witness XMSampler parameter map tag mismatch")
    cursor += 4
    if cursor >= len(extension) or extension[cursor] != 110:
        raise ValueError("same-witness XMSampler parameter map count mismatch")
    cursor += 1
    for expected_index in range(110):
        if cursor + 3 > len(extension):
            raise ValueError("same-witness XMSampler parameter map is truncated")
        source, target = struct.unpack_from("<BH", extension, cursor)
        cursor += 3
        if (source, target) != (expected_index, expected_index):
            raise ValueError("same-witness XMSampler parameter map mismatch")
    if extension[cursor : cursor + 4] != b"PBUS":
        raise ValueError("same-witness XMSampler bus-state tag mismatch")
    cursor += 4
    if cursor >= len(extension) or extension[cursor] != 0:
        raise ValueError("same-witness XMSampler bus-state value mismatch")
    cursor += 1
    if cursor != len(extension) or position + len(state) != len(payload):
        raise ValueError("same-witness XMSampler MACD parser did not end at chunk boundary")

    return {
        "specific_size": specific_size,
        "specific_sha256": digest(state[:specific_end]),
        "extension_sha256": digest(extension),
        "version": version,
        "voices": voices,
        "resampler_quality": quality,
        "global_volume": global_volume,
        "panning_mode": panning_mode,
        "instrument_bank": instrument_bank,
        "channel_count": len(channel_states),
        "macd_sha256": digest(payload),
    }


def expected_master_macd_state_bytes() -> bytes:
    state = bytearray(struct.pack("<IiB", 5, 256, 0))
    state += struct.pack("<iihhhh", 0, 2, 0, 0, 1, 1)
    state += b"PMAP" + bytes((18,))
    for index in range(18):
        state += struct.pack("<BH", index, index)
    state += b"PBUS\0"
    return bytes(state)


def validate_master_macd_state(payload: bytes, position: int) -> dict:
    state = payload[position:]
    expected = expected_master_macd_state_bytes()
    if state != expected or digest(state) != EXPECTED_MASTER_STATE_SHA256:
        raise ValueError(
            "same-witness Master machine-specific MACD state differs from canonical payload"
        )
    if len(state) < 9:
        raise ValueError("same-witness Master specific state is truncated")

    specific_size, out_dry, decrease_on_clip = struct.unpack_from("<IiB", state, 0)
    specific_end = 4 + specific_size
    if (
        specific_size != 5
        or specific_end != 9
        or digest(state[:specific_end]) != EXPECTED_MASTER_SPECIFIC_SHA256
        or out_dry != 256
        or decrease_on_clip != 0
    ):
        raise ValueError("same-witness Master loader-consumed output state mismatch")

    extension = state[specific_end:]
    if digest(extension) != EXPECTED_MASTER_EXTENSION_SHA256:
        raise ValueError("same-witness Master modern MACD extension mismatch")

    cursor = 0
    input_index, pair_count = struct.unpack_from("<ii", extension, cursor)
    cursor += 8
    pairs = []
    for _ in range(pair_count):
        if cursor + 4 > len(extension):
            raise ValueError("same-witness Master wire mapping is truncated")
        pairs.append(struct.unpack_from("<hh", extension, cursor))
        cursor += 4
    if input_index != 0 or pairs != [(0, 0), (1, 1)]:
        raise ValueError("same-witness Master stereo wire mapping mismatch")

    if extension[cursor : cursor + 4] != b"PMAP":
        raise ValueError("same-witness Master parameter map tag mismatch")
    cursor += 4
    if cursor >= len(extension) or extension[cursor] != 18:
        raise ValueError("same-witness Master parameter map count mismatch")
    cursor += 1
    for expected_index in range(18):
        if cursor + 3 > len(extension):
            raise ValueError("same-witness Master parameter map is truncated")
        source, target = struct.unpack_from("<BH", extension, cursor)
        cursor += 3
        if (source, target) != (expected_index, expected_index):
            raise ValueError("same-witness Master parameter map mismatch")

    if extension[cursor : cursor + 4] != b"PBUS":
        raise ValueError("same-witness Master bus-state tag mismatch")
    cursor += 4
    if cursor >= len(extension) or extension[cursor] != 0:
        raise ValueError("same-witness Master bus-state value mismatch")
    cursor += 1
    if cursor != len(extension) or position + len(state) != len(payload):
        raise ValueError("same-witness Master MACD parser did not end at chunk boundary")

    return {
        "specific_size": specific_size,
        "specific_sha256": digest(state[:specific_end]),
        "extension_sha256": digest(extension),
        "out_dry": out_dry,
        "decrease_on_clip": decrease_on_clip,
        "wire_mapping": [list(pair) for pair in pairs],
        "macd_state_sha256": digest(state),
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
    edit_name, state_offset = read_cstring(
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
        "state_offset": state_offset,
        "payload_size": len(payload),
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
    parsed_machines = [
        (parse_machine_routing(payload), payload)
        for _version, payload in machine_chunks
    ]
    machines = {parsed["slot"]: parsed for parsed, _payload in parsed_machines}
    machine_payloads = {parsed["slot"]: payload for parsed, payload in parsed_machines}
    if set(machines) != {0, 128}:
        raise ValueError("same-witness fixture playback machine set mismatch")
    sampler = machines[0]
    master = machines[128]

    def expected_connection(
        index: int,
        *,
        input_slot: int = -1,
        output_slot: int = -1,
        input_connected: int = 0,
        output_connected: int = 0,
    ) -> dict:
        return {
            "index": index,
            "input_slot": input_slot,
            "output_slot": output_slot,
            "input_volume": 1.0,
            "wire_multiplier": 1.0,
            "output_connected": output_connected,
            "input_connected": input_connected,
        }

    expected_sampler_connections = [
        expected_connection(0, output_slot=128, output_connected=1),
        *[expected_connection(index) for index in range(1, 12)],
    ]
    expected_master_connections = [
        expected_connection(0, input_slot=0, input_connected=1),
        *[expected_connection(index) for index in range(1, 12)],
    ]
    sampler_connection_identity = [
        dict(item) for item in sampler["connections"]
    ]
    master_connection_identity = [
        dict(item) for item in master["connections"]
    ]
    # The two active gain values have a dedicated audible-route check below.
    # Normalize only those fields here so the connection-table gate can focus
    # on exact slots, booleans, inactive gains, and all unused entries.
    sampler_connection_identity[0]["wire_multiplier"] = 1.0
    master_connection_identity[0]["input_volume"] = 1.0
    if (
        sampler_connection_identity != expected_sampler_connections
        or master_connection_identity != expected_master_connections
    ):
        raise ValueError(
            "same-witness fixture MACD connection table differs from canonical route"
        )

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
        or sampler["edit_name"] != "XMSampler"
        or sampler["bypassed"] != 0
        or sampler["muted"] != 0
        or sampler["input_count"] != 0
        or sampler["output_count"] != 1
        or len(sampler_outputs) != 1
        or sampler_outputs[0]["output_slot"] != 128
        or sampler_outputs[0]["input_slot"] != -1
        or master["machine_type"] != 0
        or master["edit_name"] != "Psycle Master and Minimixer"
        or master["bypassed"] != 0
        or master["muted"] != 0
        or master["input_count"] != 1
        or master["output_count"] != 0
        or len(master_inputs) != 1
        or master_inputs[0]["input_slot"] != 0
        or master_inputs[0]["output_slot"] != -1
    ):
        raise ValueError(
            "same-witness fixture does not route sampler slot 0 directly to Master slot 128"
        )

    xmsampler_state = validate_xmsampler_macd_state(
        machine_payloads[0], sampler["state_offset"]
    )
    master_state = validate_master_macd_state(
        machine_payloads[128], master["state_offset"]
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
        "sampler_state": xmsampler_state,
        "master_slot": 128,
        "master_type": master["machine_type"],
        "master_macd_sha256": master["sha256"],
        "master_state": master_state,
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
    observed_layout = [(fourcc, version) for fourcc, version, _payload in chunks]
    if tuple(observed_layout) != EXPECTED_TOP_LEVEL_LAYOUT:
        raise ValueError(
            "same-witness fixture top-level chunk layout differs from canonical witness"
        )

    info = [payload for fourcc, _version, payload in chunks if fourcc == b"INFO"]
    if (
        len(info) != 1
        or info[0] != EXPECTED_INFO_PAYLOAD
        or digest(info[0]) != EXPECTED_INFO_PAYLOAD_SHA256
    ):
        raise ValueError(
            "same-witness fixture INFO payload differs from canonical complete metadata"
        )
    title, position = read_cstring(info[0], 0, "song title")
    author, position = read_cstring(info[0], position, "song author")
    comment, position = read_cstring(info[0], position, "song comment")
    if (
        title != TITLE
        or author != "Unnamed"
        or comment != "No Comments"
        or position != len(info[0])
    ):
        raise ValueError("same-witness fixture INFO metadata mismatch")

    playback_graph = validate_playback_graph(chunks)

    sngi = [
        (version, payload)
        for fourcc, version, payload in chunks
        if fourcc == b"SNGI"
    ]
    if (
        len(sngi) != 1
        or sngi[0][0] != 4
        or len(sngi[0][1]) != 109
        or digest(sngi[0][1]) != EXPECTED_SNGI_PAYLOAD_SHA256
    ):
        raise ValueError(
            "same-witness fixture SNGI payload differs from canonical complete song state"
        )
    sngi_payload = sngi[0][1]
    song_tracks, bpm, lpb = struct.unpack_from("<iii", sngi_payload, 0)
    (
        octave,
        machine_soloed,
        track_soloed,
        sequence_bus,
        midi_selected,
        auxcol_selected,
        instrument_selected,
        sequence_width,
    ) = struct.unpack_from("<iiiiiiii", sngi_payload, 12)
    track_flags = list(sngi_payload[44:76])
    if (
        (song_tracks, bpm, lpb) != (16, 137, 8)
        or octave != 4
        or machine_soloed != -1
        or track_soloed != -1
        or sequence_bus != 128
        or midi_selected != 0
        or auxcol_selected != 0
        or instrument_selected != 0
        or sequence_width != 1
        or track_flags != [0] * 32
    ):
        raise ValueError(
            "same-witness fixture SNGI loader-consumed state mismatch"
        )

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
        "top_level_chunk_layout": [
            {"fourcc": fourcc.decode("ascii"), "version": version}
            for fourcc, version in observed_layout
        ],
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
    require_retrigger_effects: bool = False,
) -> dict:
    if expected_frame_count is not None and analysis.get("frame_count") != expected_frame_count:
        raise ValueError(
            f"{role} render frame count does not match fixed-frame target"
        )
    if require_retrigger_effects:
        counts = analysis.get("window_onset_counts")
        if (
            not isinstance(counts, dict)
            or not isinstance(counts.get("retrigger_beat_1"), int)
            or isinstance(counts.get("retrigger_beat_1"), bool)
            or counts["retrigger_beat_1"] < 2
            or not isinstance(counts.get("retr_cont_beat_2"), int)
            or isinstance(counts.get("retr_cont_beat_2"), bool)
            or counts["retr_cont_beat_2"] < 2
        ):
            raise ValueError(
                f"{role} render does not prove both FB and FA retrigger-family effects"
            )
    return analysis


def analyze_wave_observation(data: bytes) -> dict:
    """Describe a valid PCM render without requiring command-bearing onset counts."""
    parsed = base.parse_pcm16_wave(data)
    onsets = base.onset_frames(parsed["frames"])
    beat_frames = parsed["sample_rate"] * 60.0 / base.EXPECTED_LAYOUT["bpm"]
    counts = [0, 0, 0, 0]
    beat_positions = []
    for frame in onsets:
        beat = frame / beat_frames
        beat_positions.append(beat)
        bucket = min(3, max(0, int(math.floor(beat + 1e-9))))
        counts[bucket] += 1
    return {
        "sample_rate": parsed["sample_rate"],
        "channels": parsed["channels"],
        "bits_per_sample": parsed["bits_per_sample"],
        "frame_count": len(parsed["frames"]),
        "onset_frames": onsets,
        "onset_beats": [round(value, 9) for value in beat_positions],
        "window_onset_counts": {
            "note_delay_beat_0": counts[0],
            "retrigger_beat_1": counts[1],
            "retr_cont_beat_2": counts[2],
            "extended_marker_beat_3": counts[3],
        },
    }


def analyze_original_command_evidence(data: bytes) -> tuple[dict, bool, str | None]:
    """Keep valid original audio even when it does not prove command execution."""
    observation = analyze_wave_observation(data)
    try:
        strict = validate_same_witness_analysis(
            base.analyze_wave(data),
            "original",
            require_retrigger_effects=True,
        )
    except ValueError as exc:
        return observation, False, str(exc)
    return strict, True, None


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


def normalized_elf_sha256(path: Path, work: Path, label: str) -> str:
    """Hash complete behavior-affecting ELF/link state, excluding debug/tool notes."""
    normalized = work / (label + ".normalized-elf")
    try:
        shutil.copyfile(path, normalized)
        subprocess.check_output(
            [
                "objcopy",
                "--strip-debug",
                "--remove-section=.comment",
                "--remove-section=.note.gnu.build-id",
                str(normalized),
            ],
            stderr=subprocess.STDOUT,
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValueError(
            "same-witness renderer normalized ELF inspection failed"
        ) from exc
    data = normalized.read_bytes()
    if not data.startswith(b"\x7fELF"):
        raise ValueError("same-witness normalized renderer is not ELF")
    return digest(data)


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
            retained_elf = normalized_elf_sha256(retained, build, "retained")
            rebuilt_elf = normalized_elf_sha256(rebuilt, build, "rebuilt")
            if retained_elf != rebuilt_elf:
                raise ValueError(
                    "same-witness renderer reproducible normalized ELF identity mismatch"
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
    require_retrigger_effects: bool = False,
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
            require_retrigger_effects=require_retrigger_effects,
        )
        bindings.append(binding)
        waves.append(data)
        analyses.append(analysis)
    if waves[0] != waves[1] or bindings[0]["sha256"] != bindings[1]["sha256"]:
        raise ValueError(f"{role} repeated renders are not byte-identical")
    if analyses[0] != analyses[1]:
        raise ValueError(f"{role} repeated onset analyses differ")
    return bindings, waves[0], analyses[0]


def extract_renderer_summary(data: bytes, label: str) -> dict:
    summaries: list[dict] = []
    for raw_line in data.splitlines():
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
        raise ValueError(label + " lacks one renderer JSON summary")
    return summaries[0]


def expected_renderer_execution_replay_receipt(
    root: Path, observations: list[dict]
) -> dict:
    root = root.resolve()
    binary_relative = f"{NAME}/phase6c-delayed-retrigger-sampulse-render"
    binary_bytes = child(root, binary_relative).read_bytes()
    fixture_bytes = child(root, FIXTURE).read_bytes()
    renders = []
    for index, observation in enumerate(observations, start=1):
        relative = (
            f"{NAME}/candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
        )
        data = child(root, relative).read_bytes()
        renders.append(
            {
                "index": index,
                "path": relative,
                "sha256": digest(data),
                "size_bytes": len(data),
                "summary": observation,
            }
        )
    return {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "validation": "verified-renderer-execution-replay",
        "binary": {
            "path": binary_relative,
            "sha256": digest(binary_bytes),
        },
        "fixture": {
            "path": FIXTURE,
            "sha256": digest(fixture_bytes),
            "size_bytes": len(fixture_bytes),
        },
        "renders": renders,
    }


def validate_renderer_execution_replay(
    root: Path, observations: list[dict]
) -> dict:
    if not sys.platform.startswith("linux"):
        raise ValueError(
            "same-witness renderer execution replay requires the Linux candidate host"
        )
    root = root.resolve()
    binary = child(
        root, f"{NAME}/phase6c-delayed-retrigger-sampulse-render"
    )
    fixture = child(root, FIXTURE)
    fixture_bytes = fixture.read_bytes()
    expected = expected_renderer_execution_replay_receipt(root, observations)
    environment = dict(os.environ)
    environment["PSYCLE_THREADS"] = "1"

    with tempfile.TemporaryDirectory(
        prefix="phase6c-render-replay-", dir=root
    ) as temporary:
        replay_root = Path(temporary)
        for index, retained_observation in enumerate(observations, start=1):
            output = replay_root / f"render-{index}.wav"
            try:
                completed = subprocess.run(
                    [str(binary), str(fixture), str(output)],
                    cwd=root,
                    env=environment,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    timeout=60,
                    check=False,
                )
            except (OSError, subprocess.SubprocessError) as exc:
                raise ValueError(
                    "same-witness renderer execution replay failed"
                ) from exc
            if completed.returncode != 0 or not output.is_file():
                raise ValueError(
                    "same-witness renderer execution replay did not complete"
                )

            replay_summary = extract_renderer_summary(
                completed.stdout + b"\n" + completed.stderr,
                f"same-witness renderer replay {index}",
            )
            replay_bytes = output.read_bytes()
            retained_relative = (
                f"{NAME}/candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
            )
            retained_bytes = child(root, retained_relative).read_bytes()
            if replay_bytes != retained_bytes:
                raise ValueError(
                    "same-witness renderer replay WAV differs from retained WAV"
                )

            input_path = replay_summary.get("input_path")
            output_path = replay_summary.get("output_path")
            normalized_input = (
                input_path.replace("\\", "/")
                if isinstance(input_path, str)
                else None
            )
            normalized_output = (
                output_path.replace("\\", "/")
                if isinstance(output_path, str)
                else None
            )
            if (
                not isinstance(normalized_input, str)
                or not normalized_input.endswith("/" + FIXTURE)
                or replay_summary.get("input_size_bytes") != len(fixture_bytes)
                or replay_summary.get("input_sha256") != digest(fixture_bytes)
                or not isinstance(normalized_output, str)
                or not normalized_output.endswith("/" + output.name)
                or replay_summary.get("output_size_bytes") != len(replay_bytes)
                or replay_summary.get("output_sha256") != digest(replay_bytes)
            ):
                raise ValueError(
                    "same-witness renderer replay summary identity mismatch"
                )

            normalized = dict(replay_summary)
            normalized["input_path"] = FIXTURE
            normalized["output_path"] = retained_relative
            if normalized != retained_observation:
                raise ValueError(
                    "same-witness retained renderer summary differs from replay"
                )

    return expected


def validate_candidate_render_log(root: Path, index: int) -> dict:
    relative = f"delayed-retrigger/sampulse-candidate-render-{index}.log"
    path = child(root, relative)
    summary = extract_renderer_summary(
        path.read_bytes(),
        "same-witness candidate render log",
    )
    final_play_beat = summary.get("final_play_beat")
    expected_name = f"candidate-delayed-retrigger-sampulse-runtime-{index}.wav"
    expected_relative = f"{NAME}/{expected_name}"
    fixture_bytes = child(root, FIXTURE).read_bytes()
    input_path = summary.get("input_path")
    input_size = summary.get("input_size_bytes")
    input_sha256 = summary.get("input_sha256")
    normalized_input_path = (
        input_path.replace("\\", "/") if isinstance(input_path, str) else None
    )
    output_path = summary.get("output_path")
    output_size = summary.get("output_size_bytes")
    output_sha256 = summary.get("output_sha256")
    retained = child(root, expected_relative).read_bytes()
    normalized_output_path = (
        output_path.replace("\\", "/") if isinstance(output_path, str) else None
    )
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
    if (
        not isinstance(normalized_input_path, str)
        or not (
            normalized_input_path == FIXTURE
            or normalized_input_path.endswith("/" + FIXTURE)
        )
        or not isinstance(input_size, int)
        or isinstance(input_size, bool)
        or input_size != len(fixture_bytes)
        or not isinstance(input_sha256, str)
        or input_sha256 != digest(fixture_bytes)
    ):
        raise ValueError(
            "same-witness candidate renderer input identity does not match retained fixture"
        )

    if (
        not isinstance(normalized_output_path, str)
        or not normalized_output_path.endswith("/" + expected_relative)
        or not isinstance(output_size, int)
        or isinstance(output_size, bool)
        or output_size != len(retained)
        or not isinstance(output_sha256, str)
        or output_sha256 != digest(retained)
    ):
        raise ValueError(
            "same-witness candidate renderer output identity does not match retained WAV"
        )

    normalized = dict(summary)
    normalized["input_path"] = FIXTURE
    normalized["output_path"] = expected_relative
    return normalized


def validate_candidate_render_logs(root: Path) -> list[dict]:
    observations = [
        validate_candidate_render_log(root, index)
        for index in (1, 2)
    ]
    left = {key: value for key, value in observations[0].items() if key != "output_path"}
    right = {key: value for key, value in observations[1].items() if key != "output_path"}
    if left != right:
        raise ValueError(
            "same-witness candidate renderer procedure observations differ"
        )
    return observations


def collect_candidate(root: Path) -> dict:
    root = root.resolve()
    fixture = child(root, FIXTURE)
    raw = fixture.read_bytes()
    fixture_identity = validate_fixture_identity(raw)

    renders, wave, analysis = validate_pair_of_waves(
        root,
        "candidate-delayed-retrigger-sampulse-runtime",
        "candidate",
        expected_frame_count=CANDIDATE_TARGET_FRAMES,
        require_retrigger_effects=True,
    )
    render_observations = validate_candidate_render_logs(root)
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
    execution_replay = validate_renderer_execution_replay(
        root, render_observations
    )
    write_new(child(root, RENDER_EXECUTION_REPLAY), execution_replay)
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
            "execution_replay": artifact_binding(
                root, RENDER_EXECUTION_REPLAY
            ),
        },
        "renders": renders,
        "render_sha256": digest(wave),
        "analysis": analysis,
        "runtime_command_execution_observed": True,
        "runtime_command_execution_scope": RUNTIME_COMMAND_EXECUTION_SCOPE,
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
        or receipt.get("runtime_command_execution_scope")
        != RUNTIME_COMMAND_EXECUTION_SCOPE
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
        "execution_replay": RENDER_EXECUTION_REPLAY,
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

    stored_replay = read_json(child(root, RENDER_EXECUTION_REPLAY))
    expected_replay = expected_renderer_execution_replay_receipt(
        root, render_observations
    )
    if stored_replay != expected_replay:
        raise ValueError(
            "same-witness renderer execution replay receipt mismatch"
        )
    if sys.platform.startswith("linux"):
        replayed = validate_renderer_execution_replay(
            root, render_observations
        )
        if replayed != stored_replay:
            raise ValueError(
                "same-witness renderer execution replay validation mismatch"
            )

    renders, wave, analysis = validate_pair_of_waves(
        root,
        "candidate-delayed-retrigger-sampulse-runtime",
        "candidate",
        expected_frame_count=CANDIDATE_TARGET_FRAMES,
        require_retrigger_effects=True,
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


OBSERVER_INITIALIZATION_FAILURE_PREFIX = (
    "could not initialize render-dialog observer:"
)
OBSERVER_SEALING_FAILURE_PREFIX = "could not seal render-dialog observer:"


def validate_original_predispatch_observer_failure(attempt: object) -> list[str]:
    if not isinstance(attempt, dict):
        raise ValueError(
            "same-witness pre-dispatch observer failure attempt is not an object"
        )
    diagnostics = attempt.get("diagnostics")
    diagnostics_valid = (
        isinstance(diagnostics, list)
        and len(diagnostics) in (1, 2)
        and all(isinstance(value, str) for value in diagnostics)
        and diagnostics[0].startswith(
            OBSERVER_INITIALIZATION_FAILURE_PREFIX
        )
        and (
            len(diagnostics) == 1
            or diagnostics[1].startswith(
                base.PROCESS_INSPECTION_FAILURE_PREFIX
            )
        )
    )
    if (
        attempt.get("outcome") != "inconclusive"
        or attempt.get("command_verified") is not True
        or attempt.get("command_dispatched") is not False
        or attempt.get("dialog_verified") is not False
        or attempt.get("controls_configured") is not False
        or attempt.get("save_invoked") is not False
        or attempt.get("output") is not None
        or attempt.get("observed_output") is not None
        or attempt.get("process_exited") is not False
        or attempt.get("process_exit_code") is not None
        or not diagnostics_valid
    ):
        raise ValueError(
            "same-witness pre-dispatch observer failure shape is invalid"
        )
    return diagnostics


PRECOMMAND_EXIT_DIAGNOSTIC = "reference exited before offline render observation"
NO_EVENT_RENDER_DIALOG_DIAGNOSTIC = (
    "post-dispatch Render as Wav File EVENT_OBJECT_SHOW not observed"
)


def validate_original_precommand_exit(attempt: object) -> list[str]:
    if not isinstance(attempt, dict):
        raise ValueError("same-witness pre-command exit attempt is not an object")
    diagnostics = attempt.get("diagnostics")
    exit_code = attempt.get("process_exit_code")
    diagnostics_valid = (
        isinstance(diagnostics, list)
        and len(diagnostics) in (1, 2)
        and all(isinstance(value, str) for value in diagnostics)
        and diagnostics[0] == PRECOMMAND_EXIT_DIAGNOSTIC
        and (
            len(diagnostics) == 1
            or diagnostics[1].startswith(
                base.PROCESS_INSPECTION_FAILURE_PREFIX
            )
        )
    )
    if (
        attempt.get("outcome") != "inconclusive"
        or attempt.get("command_verified") is not False
        or attempt.get("command_dispatched") is not False
        or attempt.get("dialog_verified") is not False
        or attempt.get("controls_configured") is not False
        or attempt.get("save_invoked") is not False
        or attempt.get("render_dialog_native_event_hook_armed") is not False
        or attempt.get("render_dialog_event_message_pump_started") is not False
        or attempt.get("render_dialog_dispatch_boundary_set") is not False
        or attempt.get("render_dialog_dispatch_boundary_tick") is not None
        or attempt.get("preexisting_render_dialog_count") != 0
        or attempt.get("render_dialog_post_dispatch_observed_window_event_count") != 0
        or attempt.get("render_dialog_unresolved_post_dispatch_event_count") != 0
        or attempt.get("render_dialog_post_dispatch_event_count") != 0
        or attempt.get("selected_render_dialog_native_handle") is not None
        or attempt.get("selected_render_dialog_runtime_id") != []
        or attempt.get("dialog_discovery") is not None
        or attempt.get("output") is not None
        or attempt.get("observed_output") is not None
        or attempt.get("process_exited") is not True
        or not isinstance(exit_code, int)
        or isinstance(exit_code, bool)
        or not diagnostics_valid
    ):
        raise ValueError("same-witness pre-command process-exit shape is invalid")
    return diagnostics


def validate_original_no_event_render_dialog_failure(
    attempt: object,
) -> list[str]:
    if not isinstance(attempt, dict):
        raise ValueError("same-witness no-event render attempt is not an object")
    diagnostics = attempt.get("diagnostics")
    exit_code = attempt.get("process_exit_code")
    process_state_valid = (
        (attempt.get("process_exited") is False and exit_code is None)
        or (
            attempt.get("process_exited") is True
            and isinstance(exit_code, int)
            and not isinstance(exit_code, bool)
        )
    )
    diagnostics_valid = (
        isinstance(diagnostics, list)
        and len(diagnostics) in (1, 2)
        and all(isinstance(value, str) for value in diagnostics)
        and diagnostics[0] == NO_EVENT_RENDER_DIALOG_DIAGNOSTIC
        and (
            len(diagnostics) == 1
            or diagnostics[1].startswith(
                base.PROCESS_INSPECTION_FAILURE_PREFIX
            )
        )
    )
    boundary_tick = attempt.get("render_dialog_dispatch_boundary_tick")
    preexisting_count = attempt.get("preexisting_render_dialog_count")
    if (
        attempt.get("outcome") != "inconclusive"
        or attempt.get("command_verified") is not True
        or attempt.get("command_dispatched") is not True
        or attempt.get("dialog_verified") is not False
        or attempt.get("controls_configured") is not False
        or attempt.get("save_invoked") is not False
        or attempt.get("render_dialog_native_event_hook_armed") is not True
        or attempt.get("render_dialog_event_message_pump_started") is not True
        or attempt.get("render_dialog_dispatch_boundary_set") is not True
        or not isinstance(boundary_tick, int)
        or isinstance(boundary_tick, bool)
        or not 0 <= boundary_tick <= 0xFFFFFFFF
        or not isinstance(preexisting_count, int)
        or isinstance(preexisting_count, bool)
        or preexisting_count < 0
        or attempt.get("render_dialog_post_dispatch_observed_window_event_count") != 0
        or attempt.get("render_dialog_unresolved_post_dispatch_event_count") != 0
        or attempt.get("render_dialog_post_dispatch_event_count") != 0
        or attempt.get("selected_render_dialog_native_handle") is not None
        or attempt.get("selected_render_dialog_runtime_id") != []
        or attempt.get("dialog_discovery") is not None
        or attempt.get("output") is not None
        or attempt.get("observed_output") is not None
        or not process_state_valid
        or not diagnostics_valid
    ):
        raise ValueError(
            "same-witness no-event render-dialog failure shape is invalid"
        )
    return diagnostics


def validate_original_observer_sealing_failure(attempt: dict) -> None:
    diagnostics = attempt.get("diagnostics")
    boundary_tick = attempt.get("render_dialog_dispatch_boundary_tick")
    preexisting_count = attempt.get("preexisting_render_dialog_count")
    selected_handle = attempt.get("selected_render_dialog_native_handle")
    runtime_id = attempt.get("selected_render_dialog_runtime_id")
    if (
        attempt.get("render_dialog_native_event_hook_armed") is not True
        or attempt.get("render_dialog_event_message_pump_started") is not True
        or attempt.get("render_dialog_dispatch_boundary_set") is not True
        or not isinstance(boundary_tick, int)
        or isinstance(boundary_tick, bool)
        or not 0 <= boundary_tick <= 0xFFFFFFFF
        or not isinstance(preexisting_count, int)
        or isinstance(preexisting_count, bool)
        or preexisting_count < 0
        or not isinstance(selected_handle, int)
        or isinstance(selected_handle, bool)
        or selected_handle <= 0
        or not isinstance(runtime_id, list)
        or not runtime_id
        or any(type(value) is not int for value in runtime_id)
        or attempt.get("save_invoked") is not True
        or not isinstance(attempt.get("stable_output_polls"), int)
        or isinstance(attempt.get("stable_output_polls"), bool)
        or attempt["stable_output_polls"] < 4
        or attempt.get("close_control_seen") is not True
        or not isinstance(diagnostics, list)
        or not diagnostics
        or any(not isinstance(value, str) for value in diagnostics)
        or not any(
            value.startswith(OBSERVER_SEALING_FAILURE_PREFIX)
            for value in diagnostics
        )
    ):
        raise ValueError(
            "same-witness inconclusive render lacks observer-sealing failure evidence"
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

    # A render can be fully finalized while the helper fails only when
    # inspecting process state. That is infrastructure uncertainty: retain the
    # finalized output, but never promote it into deterministic evidence.
    last_attempt = attempts[-1] if attempts else None
    process_inspection_failure = (
        len(renders) == len(attempts)
        and len(renders) in (1, 2)
        and isinstance(last_attempt, dict)
        and last_attempt.get("outcome") == "rendered"
        and base.has_diagnostic_prefix(
            last_attempt, base.PROCESS_INSPECTION_FAILURE_PREFIX
        )
    )
    if process_inspection_failure:
        retained_diagnostics = []
        for index, value in enumerate(renders, start=1):
            attempt = attempts[index - 1]
            is_final = index == len(renders)
            binding, data = validate_original_attempt(
                original_root,
                attempt,
                index,
                allow_post_completion_exit=(
                    is_final and attempt.get("process_exited") is True
                ),
                allow_process_inspection_failure=is_final,
            )
            if index < len(attempts):
                require_clean_completed_attempt_before_later_attempt(
                    attempt, "same-witness later render"
                )
            if value != binding:
                raise ValueError(
                    "same-witness process-inspection render binding mismatch"
                )
            retained_renders.append(binding)
            retained_analyses.append(analyze_wave_observation(data))
            retained_diagnostics.extend(attempt.get("diagnostics", []))

        observed_binding = validate_original_observed_output(
            original_root, last_attempt, len(attempts)
        )
        if observed_binding is None:
            raise ValueError(
                "same-witness process-inspection failure lacks bound finalized output"
            )
        return {
            "inconclusive_reason": "post-render-process-inspection-failure",
            "binding_error": None,
            "diagnostics": retained_diagnostics,
            "process_exit_code": last_attempt.get("process_exit_code"),
            "observed_output": observed_binding,
            "retained_renders": retained_renders,
            "retained_render_analyses": retained_analyses,
        }

    # A render can be fully finalized and bound before the reference exits
    # during the helper's final process refresh. Retain that output and exact
    # exit diagnostically instead of forcing the shape through the
    # "failed extra attempt" process-exit path.
    post_completion_exit = (
        len(renders) == len(attempts)
        and len(renders) in (1, 2)
        and isinstance(last_attempt, dict)
        and last_attempt.get("outcome") == "rendered"
        and last_attempt.get("process_exited") is True
    )
    if post_completion_exit:
        retained_diagnostics = []
        for index, value in enumerate(renders, start=1):
            attempt = attempts[index - 1]
            binding, data = validate_original_attempt(
                original_root,
                attempt,
                index,
                allow_post_completion_exit=(index == len(renders)),
            )
            if index < len(attempts):
                require_clean_completed_attempt_before_later_attempt(
                    attempt, "same-witness later render"
                )
            if value != binding:
                raise ValueError(
                    "same-witness post-completion-exit render binding mismatch"
                )
            retained_renders.append(binding)
            retained_analyses.append(analyze_wave_observation(data))
            retained_diagnostics.extend(attempt.get("diagnostics", []))

        exit_code = last_attempt.get("process_exit_code")
        observed_binding = validate_original_observed_output(
            original_root, last_attempt, len(attempts)
        )
        if observed_binding is None:
            raise ValueError(
                "same-witness post-completion exit lacks bound finalized output"
            )
        return {
            "inconclusive_reason": "process-exit-after-completed-render",
            "binding_error": None,
            "diagnostics": retained_diagnostics,
            "process_exit_code": exit_code,
            "observed_output": observed_binding,
            "retained_renders": retained_renders,
            "retained_render_analyses": retained_analyses,
        }

    if len(attempts) == len(renders) + 1:
        final_attempt = attempts[-1]
        final_diagnostics = (
            final_attempt.get("diagnostics")
            if isinstance(final_attempt, dict)
            else None
        )
        if (
            isinstance(final_diagnostics, list)
            and len(final_diagnostics) in (1, 2)
            and all(isinstance(value, str) for value in final_diagnostics)
            and final_diagnostics[0].startswith(
                OBSERVER_INITIALIZATION_FAILURE_PREFIX
            )
            and (
                len(final_diagnostics) == 1
                or final_diagnostics[1].startswith(
                    base.PROCESS_INSPECTION_FAILURE_PREFIX
                )
            )
        ):
            retained_diagnostics = []
            for index, value in enumerate(renders, start=1):
                completed_attempt = attempts[index - 1]
                binding, data = validate_original_attempt(
                    original_root, completed_attempt, index
                )
                if value != binding:
                    raise ValueError(
                        "same-witness pre-dispatch failure retained render "
                        "binding mismatch"
                    )
                if (
                    completed_attempt.get("dialog_closed") is not True
                    or completed_attempt.get("diagnostics") != []
                ):
                    raise ValueError(
                        "same-witness second-attempt initialization failure "
                        "follows a non-clean completed render"
                    )
                retained_renders.append(binding)
                retained_analyses.append(analyze_wave_observation(data))
                retained_diagnostics.extend(
                    completed_attempt.get("diagnostics", [])
                )
            init_diagnostics = validate_original_predispatch_observer_failure(
                final_attempt
            )
            return {
                "inconclusive_reason": "render-observer-initialization-failure",
                "binding_error": None,
                "diagnostics": retained_diagnostics + init_diagnostics,
                "process_exit_code": None,
                "observed_output": None,
                "retained_renders": retained_renders,
                "retained_render_analyses": retained_analyses,
            }

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
            if index + 1 < len(attempts):
                require_clean_completed_attempt_before_later_attempt(
                    attempts[index], "same-witness later render"
                )
            if renders[index] != binding:
                raise ValueError(
                    "same-witness nondeterministic render binding mismatch"
                )
            retained_renders.append(binding)
            retained_analyses.append(analyze_wave_observation(data))
        if retained_renders[0]["sha256"] == retained_renders[1]["sha256"]:
            raise ValueError(
                "same-witness inconclusive pair is actually byte-identical"
            )
        retained_diagnostics = [
            diagnostic
            for attempt in attempts
            for diagnostic in attempt.get("diagnostics", [])
        ]
        return {
            "inconclusive_reason": "nondeterministic-completed-render-pair",
            "binding_error": None,
            "diagnostics": retained_diagnostics,
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
        retained_analyses.append(analyze_wave_observation(data))
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
        require_clean_completed_attempt_before_later_attempt(
            attempts[index], "same-witness interrupted later attempt"
        )
        if renders[index] != binding:
            raise ValueError(
                "same-witness inconclusive retained render binding mismatch"
            )
        retained_renders.append(binding)
        retained_analyses.append(analyze_wave_observation(data))

    attempt = attempts[-1]
    if not isinstance(attempt, dict):
        raise ValueError("same-witness inconclusive render attempt is not an object")

    attempt_diagnostics = attempt.get("diagnostics")
    if (
        isinstance(attempt_diagnostics, list)
        and len(attempt_diagnostics) in (1, 2)
        and attempt_diagnostics
        and attempt_diagnostics[0] == PRECOMMAND_EXIT_DIAGNOSTIC
    ):
        validate_original_precommand_exit(attempt)
        observed_binding = validate_original_observed_output(
            original_root, attempt, len(attempts)
        )
        return {
            "inconclusive_reason": "process-exit-before-render-command-verification",
            "binding_error": None,
            "diagnostics": attempt_diagnostics,
            "process_exit_code": attempt.get("process_exit_code"),
            "observed_output": observed_binding,
            "retained_renders": retained_renders,
            "retained_render_analyses": retained_analyses,
        }

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
    process_exited = attempt.get("process_exited")
    process_exit_code = attempt.get("process_exit_code")
    exited_validly = (
        process_exited is True
        and isinstance(process_exit_code, int)
        and not isinstance(process_exit_code, bool)
    )
    alive_validly = process_exited is False and process_exit_code is None
    if (
        attempt.get("outcome") != "inconclusive"
        or not (alive_validly or exited_validly)
        or attempt.get("output") is not None
        or not isinstance(diagnostics, list)
        or not diagnostics
    ):
        raise ValueError("same-witness inconclusive render shape mismatch")

    try:
        validate_original_event_binding(attempt)
    except ValueError as exc:
        binding_error = str(exc)
        if any(
            isinstance(value, str)
            and value.startswith(OBSERVER_SEALING_FAILURE_PREFIX)
            for value in diagnostics
        ):
            if attempt.get("save_invoked") is True:
                validate_original_observer_sealing_failure(attempt)
            else:
                base.validate_presave_render_quarantine(attempt)
            inconclusive_reason = "render-observer-sealing-failure"
        elif any(
            value == NO_EVENT_RENDER_DIALOG_DIAGNOSTIC
            for value in diagnostics
            if isinstance(value, str)
        ):
            validate_original_no_event_render_dialog_failure(attempt)
            base.validate_presave_render_quarantine(attempt)
            inconclusive_reason = "render-dialog-event-not-observed"
        else:
            validate_original_ambiguous_event_binding(attempt)
            inconclusive_reason = (
                "process-exit-before-save-wave"
                if exited_validly
                else "ambiguous-final-render-attempt"
            )
    else:
        # The dialog can be bound unambiguously and still fail later in UIA or
        # harness automation. A process exit before Save Wave is likewise
        # retained as non-evidentiary UNKNOWN.
        binding_error = None
        if any(
            isinstance(value, str)
            and value.startswith(OBSERVER_SEALING_FAILURE_PREFIX)
            for value in diagnostics
        ):
            if attempt.get("save_invoked") is True:
                validate_original_observer_sealing_failure(attempt)
            else:
                base.validate_presave_render_quarantine(attempt)
            inconclusive_reason = "render-observer-sealing-failure"
        else:
            inconclusive_reason = (
                "process-exit-before-save-wave"
                if exited_validly
                else "post-binding-render-automation-failure"
            )

    observed_binding = validate_original_observed_output(
        original_root, attempt, len(attempts)
    )

    return {
        "inconclusive_reason": inconclusive_reason,
        "binding_error": binding_error,
        "diagnostics": diagnostics,
        "process_exit_code": process_exit_code,
        "observed_output": observed_binding,
        "retained_renders": retained_renders,
        "retained_render_analyses": retained_analyses,
    }

def inconclusive_render_binding_status(quarantine: dict) -> str:
    if quarantine.get("inconclusive_reason") == "pre-render-load-not-accepted":
        return "not-attempted"
    if quarantine.get("inconclusive_reason") in {
        "render-observer-initialization-failure",
        "process-exit-before-render-command-verification",
    }:
        return "not-dispatched"
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
        require_clean_completed_attempt_before_later_attempt(
            attempts[index], "same-witness process-exit later attempt"
        )
        if renders[index] != binding:
            raise ValueError(
                "same-witness process-exit retained render binding mismatch"
            )
        retained_renders.append(binding)
        retained_analyses.append(analyze_wave_observation(data))

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
        if any(
            isinstance(value, str)
            and value.startswith(OBSERVER_SEALING_FAILURE_PREFIX)
            for value in diagnostics
        ):
            validate_original_observer_sealing_failure(attempt)
        else:
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
    original_root: Path,
    attempt: object,
    index: int,
    *,
    allow_post_completion_exit: bool = False,
    allow_process_inspection_failure: bool = False,
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
    if not isinstance(diagnostics, list) or any(
        not isinstance(value, str) for value in diagnostics
    ):
        raise ValueError("same-witness original render diagnostics are malformed")
    inspection_diagnostics = [
        value
        for value in diagnostics
        if value.startswith(base.PROCESS_INSPECTION_FAILURE_PREFIX)
    ]
    if allow_process_inspection_failure:
        if len(inspection_diagnostics) != 1:
            raise ValueError(
                "same-witness post-render process-inspection evidence is missing"
            )
    elif inspection_diagnostics:
        raise ValueError(
            "same-witness successful render contains process-inspection failure"
        )
    terminal_diagnostics = [
        value
        for value in diagnostics
        if not value.startswith(base.PROCESS_INSPECTION_FAILURE_PREFIX)
    ]
    teardown_diagnostic = (
        "render output finalized and Close control was verified, "
        "but dialog teardown did not complete"
    )
    completed_and_closed = (
        attempt.get("dialog_closed") is True
        and attempt.get("close_control_seen") is True
        and attempt.get("close_uia_invoked") is True
        and terminal_diagnostics == []
    )
    completed_with_teardown_failure = (
        attempt.get("dialog_closed") is False
        and attempt.get("close_control_seen") is True
        and attempt.get("close_uia_invoked") is True
        and terminal_diagnostics == [teardown_diagnostic]
    )
    exit_code = attempt.get("process_exit_code")
    process_state_valid = (
        (
            attempt.get("process_exited") is True
            and isinstance(exit_code, int)
            and not isinstance(exit_code, bool)
        )
        if allow_post_completion_exit
        else (
            attempt.get("process_exited") is False
            and exit_code is None
        )
    )
    if (
        attempt.get("outcome") != "rendered"
        or not process_state_valid
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


def require_clean_completed_attempt_before_later_attempt(
    attempt: object, context: str
) -> None:
    if (
        not isinstance(attempt, dict)
        or attempt.get("dialog_closed") is not True
        or attempt.get("diagnostics") != []
    ):
        raise ValueError(f"{context} follows a non-clean completed render")


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
            "same_onset_analyzer": False,
            "candidate_onset_analyzer": STRICT_ANALYZER_ID,
            "original_retained_render_analyzer": RELAXED_ANALYZER_ID,
            "candidate": {
                "render_sha256": candidate["render_sha256"],
                "analysis": candidate["analysis"],
                "runtime_command_execution_observed": True,
                "runtime_command_execution_scope": RUNTIME_COMMAND_EXECUTION_SCOPE,
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
        if (
            quarantine.get("process_exit_code") is not None
            and receipt.get("exit_code_before_termination")
            != quarantine["process_exit_code"]
        ):
            raise ValueError(
                "same-witness inconclusive process-exit code is inconsistent"
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
            "retained_render_analyzer": RELAXED_ANALYZER_ID,
            "render_sha256": None,
            "analysis": None,
            "runtime_command_execution_observed": False,
            "fresh_render_event_binding": binding_status,
            "fresh_render_event_binding_error": quarantine["binding_error"],
            "observed_output": quarantine["observed_output"],
            "diagnostics": quarantine["diagnostics"],
            "process_exit_code": quarantine.get("process_exit_code"),
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
            "same_onset_analyzer": False,
            "candidate_onset_analyzer": STRICT_ANALYZER_ID,
            "original_retained_render_analyzer": RELAXED_ANALYZER_ID,
            "candidate": {
                "render_sha256": candidate["render_sha256"],
                "analysis": candidate["analysis"],
                "runtime_command_execution_observed": True,
                "runtime_command_execution_scope": RUNTIME_COMMAND_EXECUTION_SCOPE,
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
                "process_exit_code": quarantine.get("process_exit_code"),
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
    require_clean_completed_attempt_before_later_attempt(
        attempts[0], "same-witness repeated render"
    )
    second_binding, second = validate_original_attempt(original_root, attempts[1], 2)
    if first != second or first_binding["sha256"] != second_binding["sha256"]:
        raise ValueError("same-witness original renders are not byte-identical")
    expected_runtime_renders = [first_binding, second_binding]
    if runtime.get("renders") != expected_runtime_renders:
        raise ValueError("same-witness original runtime render bindings mismatch")
    analysis, command_execution_observed, analysis_error = (
        analyze_original_command_evidence(first)
    )
    second_analysis, second_command_execution_observed, second_analysis_error = (
        analyze_original_command_evidence(second)
    )
    if (
        analysis != second_analysis
        or command_execution_observed != second_command_execution_observed
        or analysis_error != second_analysis_error
    ):
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
        "runtime_command_execution_observed": command_execution_observed,
        "runtime_command_execution_scope": (
            RUNTIME_COMMAND_EXECUTION_SCOPE
            if command_execution_observed
            else None
        ),
        "command_execution_analysis_error": analysis_error,
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
        "same_onset_analyzer": command_execution_observed,
        "candidate_onset_analyzer": STRICT_ANALYZER_ID,
        "original_onset_analyzer": (
            STRICT_ANALYZER_ID if command_execution_observed else RELAXED_ANALYZER_ID
        ),
        "candidate": {
            "render_sha256": candidate["render_sha256"],
            "analysis": candidate["analysis"],
            "runtime_command_execution_observed": True,
            "runtime_command_execution_scope": RUNTIME_COMMAND_EXECUTION_SCOPE,
        },
        "original": {
            "reference_build": REFERENCE_BUILD,
            "render_sha256": original_analysis["render_sha256"],
            "analysis": original_analysis["analysis"],
            "runtime_command_execution_observed": command_execution_observed,
            "runtime_command_execution_scope": (
                RUNTIME_COMMAND_EXECUTION_SCOPE
                if command_execution_observed
                else None
            ),
            "command_execution_analysis_error": analysis_error,
        },
        "command_bearing_runtime_pair_observed": command_execution_observed,
        "exact_onset_timing_interpretation": "deferred",
        "classification_allowed": False,
        "parity_status": "UNKNOWN",
        "interpretation_boundary": (
            (
                "Both sides rendered the exact same four-beat Sampulse/XMSampler "
                "witness with the same command geometry and the same onset analyzer. "
                "The multiple-onset evidence establishes only the FB retrigger and FA "
                "retrigger-continue effects; FD note-delay and FE extended-command "
                "effects are explicitly not established here. Exact onset timing is "
                "retained for the next evidence rung and is not classified in this receipt."
            )
            if command_execution_observed
            else (
                "The pinned original produced a deterministic repeated render pair, "
                "but that audio did not satisfy the conservative command-bearing onset "
                "criteria. The bound WAVs and relaxed onset observation are retained; "
                "runtime command execution is not claimed and parity remains UNKNOWN."
            )
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
