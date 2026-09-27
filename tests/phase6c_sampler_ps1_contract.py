#!/usr/bin/env python3
"""Negative controls for the Phase 6C Sampler PS1 source-semantic contract."""
from __future__ import annotations

import copy
import importlib.util
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "phase6c-sampler-ps1-contract.py"
spec = importlib.util.spec_from_file_location("phase6c_sampler_ps1_contract", SCRIPT)
assert spec is not None and spec.loader is not None
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def expect_value_error(fn, phrase: str) -> None:
    try:
        fn()
    except ValueError as exc:
        assert phrase in str(exc), exc
    else:
        raise AssertionError("expected ValueError")


if len(sys.argv) != 3:
    raise SystemExit(
        "usage: phase6c_sampler_ps1_contract.py ORIGINAL_SAMPLER_CPP ORIGINAL_SAMPLER_HPP"
    )

original_cpp = Path(sys.argv[1])
original_hpp = Path(sys.argv[2])

derived = m.derive(original_cpp, original_hpp)
committed = m.json.loads(m.OUTPUT.read_text(encoding="utf-8"))
assert m.validate(committed) == committed
assert derived == committed

assert committed["parity_status"] == "UNKNOWN"
assert committed["classification_allowed"] is False
assert committed["source_observations"] == {
    "command_ids_match_original_candidate": True,
    "polyphony_defaults_match_original_candidate": True,
    "sampler_machine_state_version_match_original_candidate": False,
    "pitch_sample_rate_basis_match_original_candidate": False,
    "extended_note_source_expression_names_match_original_candidate": False,
    "loaded_psy3_extended_note_timing_basis_match_original_candidate": True,
    "source_correspondence_is_runtime_parity": False,
}

assert committed["original"]["sampler_machine_state_version"] == 2
assert committed["candidate"]["sampler_machine_state_version"] == 1
assert committed["cpsycle"]["sampler_machine_state_version"] == 3
assert committed["original"]["pitch_sample_rate_basis"] == (
    "wave-sample-rate/output-sample-rate"
)
assert committed["candidate"]["pitch_sample_rate_basis"] == (
    "44100/output-sample-rate"
)
assert committed["original"]["extended_note_timing_basis"] == "samples-per-row/6"
assert committed["candidate"]["extended_note_timing_expression"] == "samplesPerTick/6"
assert committed["candidate"]["loaded_psy3_extended_note_timing_basis"] == (
    "row-interval/6"
)
assert committed["candidate"]["loaded_psy3_timing_evidence"] == {
    "observation": "phase6c/evidence/sequencer-bpm-lpb-tick/candidate-bpm-lpb-tick.json",
    "tick_speed": 8,
    "derived_lpb": 8.0,
    "samples_per_tick_equals_fixture_line": True,
}
assert committed["candidate"]["nonzero_extended_note_delay_assignment"] is True
assert committed["original"]["source_repository"] == "jpaquim/psycle"
assert committed["original"]["files"]["Sampler.cpp"] == {
    "path": "psycle/src/psycle/host/Sampler.cpp",
    "git_blob": m.ORIGINAL_CPP_BLOB,
}
assert committed["original"]["files"]["Sampler.hpp"] == {
    "path": "psycle/src/psycle/host/Sampler.hpp",
    "git_blob": m.ORIGINAL_HPP_BLOB,
}

assert committed["candidate"]["files"]["sampler.cpp"] == {
    "path": "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.cpp",
    "git_blob": m.CANDIDATE_CPP_BLOB,
}
assert committed["candidate"]["files"]["sampler.h"] == {
    "path": "psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.h",
    "git_blob": m.CANDIDATE_HPP_BLOB,
}

forged_binding = copy.deepcopy(committed)
forged_binding["original"]["source_repository"] = "example/other"
expect_value_error(
    lambda: m.validate(forged_binding),
    "source repository changed",
)

forged_path = copy.deepcopy(committed)
forged_path["original"]["files"]["Sampler.cpp"]["path"] = "Sampler.cpp"
expect_value_error(
    lambda: m.validate(forged_path),
    "source path/blob binding changed",
)

forged_edx = copy.deepcopy(committed)
forged_edx["candidate"]["nonzero_extended_note_delay_assignment"] = False
expect_value_error(
    lambda: m.validate(forged_edx),
    "nonzero E-Dx assignment changed",
)

promoted = copy.deepcopy(committed)
promoted["parity_status"] = "PASS"
expect_value_error(
    lambda: m.validate(promoted),
    "source receipt envelope is invalid",
)

forged_commands = copy.deepcopy(committed)
forged_commands["candidate"]["command_ids"]["OFFSET"] = 0x90
expect_value_error(
    lambda: m.validate(forged_commands),
    "candidate command table changed",
)

forged_boolean_command = copy.deepcopy(committed)
forged_boolean_command["candidate"]["command_ids"]["PORTAUP"] = True
expect_value_error(
    lambda: m.validate(forged_boolean_command),
    "candidate command identifier changed: PORTAUP",
)

forged_candidate_path = copy.deepcopy(committed)
forged_candidate_path["candidate"]["files"]["sampler.cpp"]["path"] = "sampler.cpp"
expect_value_error(
    lambda: m.validate(forged_candidate_path),
    "candidate Sampler source blob binding changed",
)

forged_loaded_timing = copy.deepcopy(committed)
forged_loaded_timing["candidate"]["loaded_psy3_extended_note_timing_basis"] = (
    "samples-per-tick/6"
)
expect_value_error(
    lambda: m.validate(forged_loaded_timing),
    "candidate Sampler loaded-PSY3 timing basis changed",
)

forged_version = copy.deepcopy(committed)
forged_version["candidate"]["sampler_machine_state_version"] = 2
expect_value_error(
    lambda: m.validate(forged_version),
    "candidate Sampler machine-state version changed",
)

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    mutated_cpp = root / "Sampler.cpp"
    mutated_hpp = root / "Sampler.hpp"
    mutated_cpp.write_bytes(original_cpp.read_bytes() + b"\n// mutation\n")
    mutated_hpp.write_bytes(original_hpp.read_bytes())
    expect_value_error(
        lambda: m.derive(mutated_cpp, mutated_hpp),
        "original Sampler.cpp blob mismatch",
    )

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    exact_cpp = root / "Sampler.cpp"
    mutated_hpp = root / "Sampler.hpp"
    exact_cpp.write_bytes(original_cpp.read_bytes())
    mutated_hpp.write_bytes(original_hpp.read_bytes() + b"\n// mutation\n")
    expect_value_error(
        lambda: m.derive(exact_cpp, mutated_hpp),
        "original Sampler.hpp blob mismatch",
    )

print("phase6c-sampler-ps1-contract: PASS")
