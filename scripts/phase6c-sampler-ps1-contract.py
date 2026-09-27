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
ORIGINAL_SOURCE_REPOSITORY = "jpaquim/psycle"
ORIGINAL_SOURCE_COMMIT = "7ac6d2c3553e2ee8dda55814d8e689919c345478"
ORIGINAL_CPP_PATH = "psycle/src/psycle/host/Sampler.cpp"
ORIGINAL_HPP_PATH = "psycle/src/psycle/host/Sampler.hpp"
CANDIDATE_BASELINE = "00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"

ORIGINAL_CPP_BLOB = "6cc0bd7328d01131c3d41b68f4e5d4189959e364"
ORIGINAL_HPP_BLOB = "46da9fa80757b11ed146a21a529c70dbc6364f12"
CANDIDATE_CPP_PATH = "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.cpp"
CANDIDATE_HPP_PATH = "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.h"
CANDIDATE_CPP = ROOT / CANDIDATE_CPP_PATH
CANDIDATE_HPP = ROOT / CANDIDATE_HPP_PATH
CANDIDATE_CPP_BLOB = "9edc00fb9013fbfde0bb0977398b934265b9c463"
CANDIDATE_HPP_BLOB = "f8df31889b2c0c4d89413db13fc5180fd5da4d53"
CPSYCLE_SOURCE_REPOSITORY = "QSOLKCB/PSYCLE-LINUX"
CPSYCLE_SNAPSHOT = "cpsycle-r12005-baseline"
CPSYCLE_C_PATH = "cpsycle/audio/src/sampler.c"
CPSYCLE_H_PATH = "cpsycle/audio/src/sampler.h"
CPSYCLE_DEFS_PATH = "cpsycle/audio/src/samplerdefs.h"
CPSYCLE_C = ROOT / CPSYCLE_C_PATH
CPSYCLE_H = ROOT / CPSYCLE_H_PATH
CPSYCLE_DEFS = ROOT / CPSYCLE_DEFS_PATH
CPSYCLE_C_BLOB = "475c96cc0742091b3b34aad634bd8d989caf83c8"
CPSYCLE_H_BLOB = "e74dd3270f5581e17104006efe874299e974e94b"
CPSYCLE_DEFS_BLOB = "88e4b0fa1d720e1dd567386270694052df0ecd61"

OUTPUT = ROOT / "phase6c/evidence/sampler-ps1/source-contract.json"
CANDIDATE_TIMING_RECEIPT = (
    ROOT / "phase6c/evidence/sequencer-bpm-lpb-tick/candidate-bpm-lpb-tick.json"
)

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
    "speeddouble = pow(2.0f, (pEntry->_note+wave.WaveTune()-baseC +finetune)/12.0f)*((float)wave.WaveSampleRate()/Global::player().SampleRate());",
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
    "speeddouble = pow(2.0f, (pEntry.note()+pIns->waveTune-baseC +finetune)/12.0f)*4294967296.0f*(44100.0f/timeInfo.sampleRate());",
    "pVoice->_envelope._step = (1.0f/pIns->ENV_AT)*(44100.0f/timeInfo.sampleRate());",
    "pVoice->_wave._rVolDest > 0.5f",
    "pVoice->_wave._lVolDest > 0.5f",
    "timeInfo.samplesPerTick()/6",
    "SAMPLER_CMD_EXT_NOTEDELAY",
    "(pEntry.parameter() & 0x0f) == 0",
    "pVoice->_triggerNoteDelay = static_cast<int>( (timeInfo.samplesPerTick()/6)*(pEntry.parameter() & 0x0f) );",
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


def require_exact_int(value: object, expected: int, context: str) -> int:
    if type(value) is not int or value != expected:
        raise ValueError(f"{context} must be integer {expected}")
    return value


def require_exact_float(value: object, expected: float, context: str) -> float:
    if type(value) is not float or value != expected:
        raise ValueError(f"{context} must be float {expected}")
    return value


def require_exact_command_table(value: object, label: str) -> dict[str, int]:
    if not isinstance(value, dict) or set(value) != set(COMMAND_IDS):
        raise ValueError(f"Sampler PS1 {label} command table changed")
    for name, expected in COMMAND_IDS.items():
        actual = value.get(name)
        if type(actual) is not int or actual != expected:
            raise ValueError(
                f"Sampler PS1 {label} command identifier changed: {name}"
            )
    return value


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def validate_loaded_psy3_timing_receipt() -> dict:
    receipt = read_json(CANDIDATE_TIMING_RECEIPT)
    if (
        receipt.get("contract") != "sequencer-bpm-lpb-tick"
        or receipt.get("snapshot") != CANDIDATE_BASELINE
        or receipt.get("observation") != "timing-model-observed"
        or receipt.get("is_ticks") is not True
        or type(receipt.get("tick_speed")) is not int
        or type(receipt.get("derived_lpb")) not in (int, float)
        or float(receipt["tick_speed"]) != float(receipt["derived_lpb"])
    ):
        raise ValueError("candidate loaded-PSY3 timing receipt identity changed")
    sample_rates = receipt.get("sample_rates")
    if not isinstance(sample_rates, list) or not sample_rates:
        raise ValueError("candidate loaded-PSY3 timing samples are missing")
    for row in sample_rates:
        if not isinstance(row, dict):
            raise ValueError("candidate loaded-PSY3 timing sample is invalid")
        per_tick = row.get("samples_per_tick")
        per_line = row.get("samples_per_fixture_line")
        if type(per_tick) not in (int, float) or type(per_line) not in (int, float):
            raise ValueError("candidate loaded-PSY3 timing interval is invalid")
        if float(per_tick) != float(per_line):
            raise ValueError(
                "candidate loaded-PSY3 samplesPerTick is not the fixture line interval"
            )
    return receipt


def polyphony(source: str, prefix: str = "SAMPLER_") -> tuple[int, int]:
    return (
        extract_define(source, "MAX_POLYPHONY", prefix),
        extract_define(source, "DEFAULT_POLYPHONY", prefix),
    )


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

    original_max_polyphony, original_default_polyphony = polyphony(
        original_hpp_text
    )
    candidate_max_polyphony, candidate_default_polyphony = polyphony(
        candidate_hpp_text
    )
    cpsycle_max_polyphony, cpsycle_default_polyphony = polyphony(
        cpsycle_h_text, "PS1_SAMPLER_"
    )
    original_version = extract_cpp_version(original_hpp_text)
    candidate_version = extract_cpp_version(candidate_hpp_text)
    cpsycle_version = extract_c_version(cpsycle_defs_text)
    loaded_psy3_timing = validate_loaded_psy3_timing_receipt()

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
            "source_repository": ORIGINAL_SOURCE_REPOSITORY,
            "source_commit": ORIGINAL_SOURCE_COMMIT,
            "files": {
                "Sampler.cpp": {
                    "path": ORIGINAL_CPP_PATH,
                    "git_blob": ORIGINAL_CPP_BLOB,
                },
                "Sampler.hpp": {
                    "path": ORIGINAL_HPP_PATH,
                    "git_blob": ORIGINAL_HPP_BLOB,
                },
            },
            "max_polyphony": original_max_polyphony,
            "default_polyphony": original_default_polyphony,
            "command_ids": original_commands,
            "sampler_machine_state_version": original_version,
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
                "sampler.cpp": {
                    "path": CANDIDATE_CPP_PATH,
                    "git_blob": CANDIDATE_CPP_BLOB,
                },
                "sampler.h": {
                    "path": CANDIDATE_HPP_PATH,
                    "git_blob": CANDIDATE_HPP_BLOB,
                },
            },
            "max_polyphony": candidate_max_polyphony,
            "default_polyphony": candidate_default_polyphony,
            "command_ids": candidate_commands,
            "sampler_machine_state_version": candidate_version,
            "pitch_sample_rate_basis": "44100/output-sample-rate",
            "envelope_sample_rate_basis": "44100/output-sample-rate",
            "normal_loop_wrap": "subtract-loop-length-at-loop-end",
            "panning_destination_cap": 0.5,
            "extended_note_timing_expression": "samplesPerTick/6",
            "loaded_psy3_extended_note_timing_basis": "row-interval/6",
            "loaded_psy3_timing_evidence": {
                "observation": "phase6c/evidence/sequencer-bpm-lpb-tick/candidate-bpm-lpb-tick.json",
                "tick_speed": loaded_psy3_timing["tick_speed"],
                "derived_lpb": loaded_psy3_timing["derived_lpb"],
                "samples_per_tick_equals_fixture_line": True,
            },
            "nonzero_extended_note_delay_assignment": (
                "pVoice->_triggerNoteDelay = static_cast<int>( "
                "(timeInfo.samplesPerTick()/6)*(pEntry.parameter() & 0x0f) );"
                in candidate_cpp_text
            ),
        },
        "cpsycle": {
            "role": "shared-contract supporting source only",
            "source_repository": CPSYCLE_SOURCE_REPOSITORY,
            "snapshot": CPSYCLE_SNAPSHOT,
            "files": {
                "sampler.c": {
                    "path": CPSYCLE_C_PATH,
                    "git_blob": CPSYCLE_C_BLOB,
                },
                "sampler.h": {
                    "path": CPSYCLE_H_PATH,
                    "git_blob": CPSYCLE_H_BLOB,
                },
                "samplerdefs.h": {
                    "path": CPSYCLE_DEFS_PATH,
                    "git_blob": CPSYCLE_DEFS_BLOB,
                },
            },
            "max_polyphony": cpsycle_max_polyphony,
            "default_polyphony": cpsycle_default_polyphony,
            "command_ids": cpsycle_commands,
            "sampler_machine_state_version": cpsycle_version,
            "pitch_sample_rate_basis": "sample-rate/output-sample-rate",
            "envelope_sample_rate_basis": "44100/output-sample-rate",
            "extended_note_timing_basis": "samples-per-row/6",
        },
        "source_observations": {
            "command_ids_match_original_candidate": original_commands == candidate_commands,
            "polyphony_defaults_match_original_candidate": (
                original_max_polyphony == candidate_max_polyphony
                and original_default_polyphony == candidate_default_polyphony
            ),
            "sampler_machine_state_version_match_original_candidate": (
                original_version == candidate_version
            ),
            "pitch_sample_rate_basis_match_original_candidate": False,
            "extended_note_source_expression_names_match_original_candidate": False,
            "loaded_psy3_extended_note_timing_basis_match_original_candidate": True,
            "source_correspondence_is_runtime_parity": False,
        },
        "next_evidence_boundary": {
            "priority_1": (
                "render the same project-authored PS1 note from at least one non-44100-Hz "
                "sample under pinned original Psycle and the frozen candidate; compare "
                "sample-rate-aware pitch/duration without classifying from source alone"
            ),
            "priority_2": (
                "observe PS1 E-Dx/E-Cx execution with a command-bearing runtime witness; "
                "the existing loaded-PSY3 timing receipt already establishes that the "
                "candidate samplesPerTick interval equals the fixture row interval"
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
    require_exact_int(
        value.get("schema_version"), 1, "Sampler PS1 schema_version"
    )
    if (
        value.get("phase") != "6C"
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
        "extended_note_source_expression_names_match_original_candidate": False,
        "loaded_psy3_extended_note_timing_basis_match_original_candidate": True,
        "source_correspondence_is_runtime_parity": False,
    }
    if observations != required:
        raise ValueError("Sampler PS1 source observations changed")
    for role in ("original", "candidate", "cpsycle"):
        section = value.get(role)
        if not isinstance(section, dict):
            raise ValueError(f"Sampler PS1 {role} section is missing")
        require_exact_int(
            section.get("max_polyphony"), 16, f"Sampler PS1 {role} max_polyphony"
        )
        require_exact_int(
            section.get("default_polyphony"), 8, f"Sampler PS1 {role} default_polyphony"
        )
        require_exact_command_table(section.get("command_ids"), role)
        require_exact_float(
            section.get("panning_destination_cap"),
            0.5,
            f"Sampler PS1 {role} panning_destination_cap",
        )
    original = value["original"]
    if original.get("reference_build") != REFERENCE_BUILD:
        raise ValueError("original Sampler reference build changed")
    if original.get("source_repository") != ORIGINAL_SOURCE_REPOSITORY:
        raise ValueError("original Sampler source repository changed")
    if original.get("source_commit") != ORIGINAL_SOURCE_COMMIT:
        raise ValueError("original Sampler source commit changed")
    files = original.get("files")
    if not isinstance(files, dict):
        raise ValueError("original Sampler source files are missing")
    expected_original_files = {
        "Sampler.cpp": {"path": ORIGINAL_CPP_PATH, "git_blob": ORIGINAL_CPP_BLOB},
        "Sampler.hpp": {"path": ORIGINAL_HPP_PATH, "git_blob": ORIGINAL_HPP_BLOB},
    }
    if files != expected_original_files:
        raise ValueError("original Sampler source path/blob binding changed")
    require_exact_int(
        original.get("sampler_machine_state_version"),
        2,
        "original Sampler machine_state_version",
    )
    candidate = value["candidate"]
    if candidate.get("snapshot") != CANDIDATE_BASELINE:
        raise ValueError("candidate Sampler baseline changed")
    expected_candidate_files = {
        "sampler.cpp": {
            "path": CANDIDATE_CPP_PATH,
            "git_blob": CANDIDATE_CPP_BLOB,
        },
        "sampler.h": {
            "path": CANDIDATE_HPP_PATH,
            "git_blob": CANDIDATE_HPP_BLOB,
        },
    }
    if candidate.get("files") != expected_candidate_files:
        raise ValueError("candidate Sampler source blob binding changed")
    require_exact_int(
        candidate.get("sampler_machine_state_version"),
        1,
        "candidate Sampler machine_state_version",
    )
    if candidate.get("extended_note_timing_expression") != "samplesPerTick/6":
        raise ValueError("candidate Sampler extended-note timing expression changed")
    if candidate.get("loaded_psy3_extended_note_timing_basis") != "row-interval/6":
        raise ValueError("candidate Sampler loaded-PSY3 timing basis changed")
    timing_evidence = candidate.get("loaded_psy3_timing_evidence")
    if not isinstance(timing_evidence, dict):
        raise ValueError("candidate Sampler loaded-PSY3 timing evidence changed")
    if timing_evidence.get("observation") != (
        "phase6c/evidence/sequencer-bpm-lpb-tick/candidate-bpm-lpb-tick.json"
    ):
        raise ValueError("candidate Sampler loaded-PSY3 timing evidence changed")
    require_exact_int(
        timing_evidence.get("tick_speed"),
        8,
        "candidate Sampler loaded-PSY3 tick_speed",
    )
    require_exact_float(
        timing_evidence.get("derived_lpb"),
        8.0,
        "candidate Sampler loaded-PSY3 derived_lpb",
    )
    if timing_evidence.get("samples_per_tick_equals_fixture_line") is not True:
        raise ValueError("candidate Sampler loaded-PSY3 timing evidence changed")
    if candidate.get("nonzero_extended_note_delay_assignment") is not True:
        raise ValueError("candidate Sampler nonzero E-Dx assignment changed")
    cpsycle = value["cpsycle"]
    if cpsycle.get("source_repository") != CPSYCLE_SOURCE_REPOSITORY:
        raise ValueError("C-Psycle Sampler source repository changed")
    if cpsycle.get("snapshot") != CPSYCLE_SNAPSHOT:
        raise ValueError("C-Psycle Sampler snapshot changed")
    expected_cpsycle_files = {
        "sampler.c": {"path": CPSYCLE_C_PATH, "git_blob": CPSYCLE_C_BLOB},
        "sampler.h": {"path": CPSYCLE_H_PATH, "git_blob": CPSYCLE_H_BLOB},
        "samplerdefs.h": {
            "path": CPSYCLE_DEFS_PATH,
            "git_blob": CPSYCLE_DEFS_BLOB,
        },
    }
    if cpsycle.get("files") != expected_cpsycle_files:
        raise ValueError("C-Psycle Sampler source path/blob binding changed")
    require_exact_int(
        cpsycle.get("sampler_machine_state_version"),
        3,
        "C-Psycle Sampler machine_state_version",
    )
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
