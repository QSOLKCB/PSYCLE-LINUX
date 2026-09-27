#!/usr/bin/env python3
"""Derive and validate the Phase 6C Sampler PS1 source-semantic baseline."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = "sampler-ps1-source-semantics"
REFERENCE_BUILD = "Psycle 1.12.0 x86"
ORIGINAL_SOURCE_COMMIT = "7ac6d2c3553e2ee8dda55814d8e689919c345478"
CANDIDATE_BASELINE = "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"

ORIGINAL_CPP_BLOB = "6cc0bd7328d01131c3d41b68f4e5d4189959e364"
ORIGINAL_HPP_BLOB = "46da9fa80757b11ed146a21a529c70dbc6364f12"
CANDIDATE_CPP = ROOT / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.cpp"
CANDIDATE_HPP = ROOT / "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.h"
CANDIDATE_CPP_BLOB = "9edc00fb9013fbfde0bb0977398b934265b9c463"
CANDIDATE_HPP_BLOB = "f8df31889b2c0c4d89413db13fc5180fd5da4d53"
CPSYCLE_C = ROOT / "cpsycle/audio/src/sampler.c"
CPSYCLE_H = ROOT / "cpsycle/audio/src/sampler.h"
CPSYCLE_DEFS = ROOT / "cpsycle/audio/src/samplerdefs.h"
CPSYCLE_C_BLOB = "475c96cc0742091b3b34aad634bd8d989caf83c8"
CPSYCLE_H_BLOB = "e74dd3270f5581e17104006efe874299e974e94b"
CPSYCLE_DEFS_BLOB = "88e4b0fa1d720e1dd567386270694052df0ecd61"

OUTPUT = ROOT / "phase6c/evidence/sampler-ps1/source-contract.json"

COMMAND_IDS = {
    "NONE": 0x00,
    "PORTAUP": 0x01,
    "PORTADOWN": 0x02,
    "PORTA2NOTE": 0x03,
    "PANNING": 0x08,
    "OFFSET": 0x09,
    "VOLUME": 0x0C,
    "RETRIG": 0x15,
    "EXTENDED": 0x0E,
    "EXT_NOTEOFF": 0xC0,
    "EXT_NOTEDELAY": 0xD0,
}

ORIGINAL_CPP_MARKERS = [
    "baseC = notecommands::middleC;",
    "_resampler.quality(helpers::dsp::resampler::quality::spline);",
    "wave.WaveSampleRate()/Global::player().SampleRate()",
    "_envelope._step = (1.0f/inst->ENV_AT)*_envelope.sratefactor;",
    "controller._rVolDest > 0.5f",
    "controller._lVolDest > 0.5f",
    "Global::player().SamplesPerRow()/6.f",
    "SAMPLER_CMD_EXT_NOTEDELAY",
    "effVal= (Global::player().SamplesPerRow()/(effretTicks+1));",
    "pFile->Write(&linearslide, sizeof(bool));",
]
ORIGINAL_HPP_MARKERS = [
    "sratefactor = 44100.0f/samplerate;",
    "WaveLoopType() == XMInstrument::WaveData<>::LoopType::NORMAL",
    "_pos.HighPart -= (wave->WaveLoopEnd() - wave->WaveLoopStart());",
    "static const uint32_t SAMPLERVERSION = 0x00000002;",
]
CANDIDATE_CPP_MARKERS = [
    "baseC = 60;",
    "resampler_.quality(dsp::resampler::quality::linear);",
    "*(44100.0f/timeInfo.sampleRate())",
    "pVoice->_envelope._step = (1.0f/pIns->ENV_AT)*(44100.0f/timeInfo.sampleRate());",
    "pVoice->_wave._rVolDest > 0.5f",
    "pVoice->_wave._lVolDest > 0.5f",
    "timeInfo.samplesPerTick()/6",
    "SAMPLER_CMD_EXT_NOTEDELAY",
    "(pEntry.parameter() & 0x0f) == 0",
    "pFile->Write(SAMPLERVERSION);",
]
CANDIDATE_HPP_MARKERS = [
    "static const uint32_t SAMPLERVERSION = 0x00000001;",
]
CPSYCLE_C_MARKERS = [
    "sample->samplerate / psy_audio_machine_samplerate",
    "psy_audio_machine_curr_samples_per_row(",
    "/ 6.f",
    "self->controller._rVolDest > 0.5f",
    "self->controller._lVolDest > 0.5f",
]
CPSYCLE_H_MARKERS = [
    "samplerenvelope_updatesrate",
    "self->sratefactor = 44100.0 / samplerate;",
]
CPSYCLE_DEFS_MARKERS = [
    "#define SAMPLERVERSION 0x00000003",
]


def git_blob(data: bytes) -> str:
    return hashlib.sha1(
        b"blob " + str(len(data)).encode("ascii") + b"\0" + data
    ).hexdigest()


def text(path: Path) -> str:
    return path.read_bytes().replace(b"\r\n", b"\n").decode("utf-8")


def require_blob(path: Path, expected: str, label: str) -> str:
    actual = git_blob(path.read_bytes())
    if actual != expected:
        raise ValueError(f"{label} blob mismatch: expected={expected} actual={actual}")
    return actual


def require_markers(source: str, markers: list[str], label: str) -> None:
    for marker in markers:
        if marker not in source:
            raise ValueError(f"{label} source marker missing: {marker}")


def extract_define(source: str, name: str, prefix: str = "") -> int:
    token = prefix + name
    match = re.search(
        rf"^\s*#define\s+{re.escape(token)}\s+(0x[0-9A-Fa-f]+|\d+)\b",
        source,
        re.MULTILINE,
    )
    if match is None:
        raise ValueError(f"missing command define: {token}")
    return int(match.group(1), 0)


def extract_cpp_version(source: str) -> int:
    match = re.search(
        r"SAMPLERVERSION\s*=\s*(0x[0-9A-Fa-f]+|\d+)",
        source,
    )
    if match is None:
        raise ValueError("missing C++ Sampler version")
    return int(match.group(1), 0)


def extract_c_version(source: str) -> int:
    match = re.search(
        r"^\s*#define\s+SAMPLERVERSION\s+(0x[0-9A-Fa-f]+|\d+)\b",
        source,
        re.MULTILINE,
    )
    if match is None:
        raise ValueError("missing C-Psycle Sampler version")
    return int(match.group(1), 0)


def command_table(source: str, prefix: str = "SAMPLER_CMD_") -> dict[str, int]:
    return {
        name: extract_define(source, name, prefix)
        for name in COMMAND_IDS
    }


def derive(original_cpp: Path, original_hpp: Path) -> dict:
    require_blob(original_cpp, ORIGINAL_CPP_BLOB, "original Sampler.cpp")
    require_blob(original_hpp, ORIGINAL_HPP_BLOB, "original Sampler.hpp")
    require_blob(CANDIDATE_CPP, CANDIDATE_CPP_BLOB, "candidate sampler.cpp")
    require_blob(CANDIDATE_HPP, CANDIDATE_HPP_BLOB, "candidate sampler.h")
    require_blob(CPSYCLE_C, CPSYCLE_C_BLOB, "C-Psycle sampler.c")
    require_blob(CPSYCLE_H, CPSYCLE_H_BLOB, "C-Psycle sampler.h")
    require_blob(CPSYCLE_DEFS, CPSYCLE_DEFS_BLOB, "C-Psycle samplerdefs.h")

    original_cpp_text = text(original_cpp)
    original_hpp_text = text(original_hpp)
    candidate_cpp_text = text(CANDIDATE_CPP)
    candidate_hpp_text = text(CANDIDATE_HPP)
    cpsycle_c_text = text(CPSYCLE_C)
    cpsycle_h_text = text(CPSYCLE_H)
    cpsycle_defs_text = text(CPSYCLE_DEFS)

    require_markers(original_cpp_text, ORIGINAL_CPP_MARKERS, "original Sampler.cpp")
    require_markers(original_hpp_text, ORIGINAL_HPP_MARKERS, "original Sampler.hpp")
    require_markers(candidate_cpp_text, CANDIDATE_CPP_MARKERS, "candidate sampler.cpp")
    require_markers(candidate_hpp_text, CANDIDATE_HPP_MARKERS, "candidate sampler.h")
    require_markers(cpsycle_c_text, CPSYCLE_C_MARKERS, "C-Psycle sampler.c")
    require_markers(cpsycle_h_text, CPSYCLE_H_MARKERS, "C-Psycle sampler.h")
    require_markers(cpsycle_defs_text, CPSYCLE_DEFS_MARKERS, "C-Psycle samplerdefs.h")

    original_commands = command_table(original_hpp_text)
    candidate_commands = command_table(candidate_hpp_text)
    cpsycle_commands = command_table(cpsycle_h_text, "PS1_SAMPLER_CMD_")
    if original_commands != COMMAND_IDS:
        raise ValueError("pinned original PS1 command table changed")
    if candidate_commands != COMMAND_IDS:
        raise ValueError("frozen candidate PS1 command table changed")
    if cpsycle_commands != COMMAND_IDS:
        raise ValueError("C-Psycle PS1 command table changed")

    result = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "parity_status": "UNKNOWN",
        "classification_allowed": False,
        "scope": {
            "kind": "source-semantic-baseline",
            "covers": [
                "PS1 command identifiers",
                "polyphony defaults",
                "pitch/sample-rate basis",
                "amplitude-envelope sample-rate scaling",
                "normal-loop wrap semantics",
                "panning destination clamp",
                "extended note-delay/note-off timing basis",
                "Sampler machine-state chunk version",
            ],
            "does_not_classify": [
                "runtime waveform parity",
                "audible pitch parity",
                "envelope timing parity",
                "loop-boundary parity",
                "tracker-command execution parity",
                "Sampler state round-trip compatibility",
                "XMSampler/Sampulse behaviour",
            ],
        },
        "original": {
            "reference_build": REFERENCE_BUILD,
            "source_commit": ORIGINAL_SOURCE_COMMIT,
            "files": {
                "Sampler.cpp": {"git_blob": ORIGINAL_CPP_BLOB},
                "Sampler.hpp": {"git_blob": ORIGINAL_HPP_BLOB},
            },
            "max_polyphony": 16,
            "default_polyphony": 8,
            "command_ids": original_commands,
            "sampler_machine_state_version": extract_cpp_version(original_hpp_text),
            "pitch_sample_rate_basis": "wave-sample-rate/output-sample-rate",
            "envelope_sample_rate_basis": "44100/output-sample-rate",
            "normal_loop_wrap": "subtract-loop-length-at-loop-end",
            "panning_destination_cap": 0.5,
            "extended_note_timing_basis": "samples-per-row/6",
            "nonzero_extended_note_delay_assignment": True,
        },
        "candidate": {
            "snapshot": CANDIDATE_BASELINE,
            "files": {
                "sampler.cpp": {"git_blob": CANDIDATE_CPP_BLOB},
                "sampler.h": {"git_blob": CANDIDATE_HPP_BLOB},
            },
            "max_polyphony": 16,
            "default_polyphony": 8,
            "command_ids": candidate_commands,
            "sampler_machine_state_version": extract_cpp_version(candidate_hpp_text),
            "pitch_sample_rate_basis": "44100/output-sample-rate",
            "envelope_sample_rate_basis": "44100/output-sample-rate",
            "normal_loop_wrap": "subtract-loop-length-at-loop-end",
            "panning_destination_cap": 0.5,
            "extended_noteoff_timing_basis": "samples-per-tick/6",
            "nonzero_extended_note_delay_assignment": False,
        },
        "cpsycle": {
            "role": "shared-contract supporting source only",
            "files": {
                "sampler.c": {"git_blob": CPSYCLE_C_BLOB},
                "sampler.h": {"git_blob": CPSYCLE_H_BLOB},
                "samplerdefs.h": {"git_blob": CPSYCLE_DEFS_BLOB},
            },
            "max_polyphony": 16,
            "default_polyphony": 8,
            "command_ids": cpsycle_commands,
            "sampler_machine_state_version": extract_c_version(cpsycle_defs_text),
            "pitch_sample_rate_basis": "sample-rate/output-sample-rate",
            "envelope_sample_rate_basis": "44100/output-sample-rate",
            "extended_note_timing_basis": "samples-per-row/6",
        },
        "source_observations": {
            "command_ids_match_original_candidate": original_commands == candidate_commands,
            "polyphony_defaults_match_original_candidate": True,
            "sampler_machine_state_version_match_original_candidate": False,
            "pitch_sample_rate_basis_match_original_candidate": False,
            "extended_note_timing_basis_match_original_candidate": False,
            "source_correspondence_is_runtime_parity": False,
        },
        "next_evidence_boundary": {
            "priority_1": (
                "render the same project-authored PS1 note from at least one non-44100-Hz "
                "sample under pinned original Psycle and the frozen candidate; compare "
                "sample-rate-aware pitch/duration without classifying from source alone"
            ),
            "priority_2": (
                "observe PS1 E-Dx/E-Cx timing with a command-bearing runtime witness to "
                "resolve the source-level samples-per-row versus samples-per-tick boundary"
            ),
            "then": (
                "add envelope, loop, panning, offset, volume, retrigger and state-roundtrip "
                "fixtures before considering the broad Sampler PS1 row classifiable"
            ),
        },
    }
    validate(result)
    return result


def validate(value: object) -> dict:
    if not isinstance(value, dict):
        raise ValueError("Sampler PS1 source receipt is not an object")
    if (
        value.get("schema_version") != 1
        or value.get("phase") != "6C"
        or value.get("contract") != CONTRACT
        or value.get("parity_status") != "UNKNOWN"
        or value.get("classification_allowed") is not False
    ):
        raise ValueError("Sampler PS1 source receipt envelope is invalid")
    scope = value.get("scope")
    if not isinstance(scope, dict) or scope.get("kind") != "source-semantic-baseline":
        raise ValueError("Sampler PS1 source scope is invalid")
    observations = value.get("source_observations")
    if not isinstance(observations, dict):
        raise ValueError("Sampler PS1 source observations are missing")
    required = {
        "command_ids_match_original_candidate": True,
        "polyphony_defaults_match_original_candidate": True,
        "sampler_machine_state_version_match_original_candidate": False,
        "pitch_sample_rate_basis_match_original_candidate": False,
        "extended_note_timing_basis_match_original_candidate": False,
        "source_correspondence_is_runtime_parity": False,
    }
    if observations != required:
        raise ValueError("Sampler PS1 source observations changed")
    for role in ("original", "candidate", "cpsycle"):
        section = value.get(role)
        if not isinstance(section, dict):
            raise ValueError(f"Sampler PS1 {role} section is missing")
        if section.get("max_polyphony") != 16 or section.get("default_polyphony") != 8:
            raise ValueError(f"Sampler PS1 {role} polyphony contract changed")
        if section.get("command_ids") != COMMAND_IDS:
            raise ValueError(f"Sampler PS1 {role} command table changed")
    if value["original"].get("sampler_machine_state_version") != 2:
        raise ValueError("original Sampler machine-state version changed")
    if value["candidate"].get("sampler_machine_state_version") != 1:
        raise ValueError("candidate Sampler machine-state version changed")
    if value["cpsycle"].get("sampler_machine_state_version") != 3:
        raise ValueError("C-Psycle Sampler machine-state version changed")
    if not isinstance(value.get("next_evidence_boundary"), dict):
        raise ValueError("Sampler PS1 next evidence boundary is missing")
    return value


def write_new(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8") as handle:
        handle.write(json.dumps(value, indent=2, sort_keys=True) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)
    derive_parser = sub.add_parser("derive")
    derive_parser.add_argument("original_cpp", type=Path)
    derive_parser.add_argument("original_hpp", type=Path)
    derive_parser.add_argument("output", type=Path, nargs="?", default=OUTPUT)
    check_parser = sub.add_parser("check")
    check_parser.add_argument("original_cpp", type=Path)
    check_parser.add_argument("original_hpp", type=Path)
    check_parser.add_argument("receipt", type=Path, nargs="?", default=OUTPUT)
    args = parser.parse_args()

    expected = derive(args.original_cpp, args.original_hpp)
    if args.mode == "derive":
        write_new(args.output, expected)
        value = expected
    else:
        value = json.loads(args.receipt.read_text(encoding="utf-8"))
        validate(value)
        if value != expected:
            raise ValueError("Sampler PS1 source receipt differs from pinned source derivation")
    print(json.dumps(value, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
