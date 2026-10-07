#!/usr/bin/env python3
"""Validate the legacy detour handoff and recheck sanitized observation summaries.

A valid implementation ledger is not proof that private observations ran. The
report keeps those gates pending unless their complete component artifacts pass
the existing lane validators. No compatibility row is written or classified.
"""
from __future__ import annotations

import argparse
import contextlib
import hashlib
import importlib.util
import io
import json
import pathlib
import tempfile
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
LEDGER = ROOT / "phase6c/legacy-closeout.json"
CONTRACT = "legacy-playback-import-closeout"
PINS = {
    "phase6c/compatibility-matrix.json": "70e9321d9b5cb5960d2bf15f7ee2c3b71824a7780cf9602395bc4f79c7f7f1c6",
    "phase6c/reference-corpus/manifest.json": "e5ade0893e9b1f00861120a3f6f216e03a4328a83ee0ce9224d52592c865d949",
    "phase6c/evidence/legacy-module-import/historical-sickmaate.json": "07099abd15c028d8be5a27d76a4990f2b51fb4cecd2b1e09b5727356891c318d",
}
DISPOSITIONS = {
    "it-sample-mode-route": "REPAIRED_DONOR_ONLY",
    "midi-import-execution": "REPAIRED_DONOR_ONLY",
    "midi-release-polyphony": "REPAIRED_DONOR_ONLY",
    "midi-tempo-event-semantics": "REPAIRED_DONOR_ONLY",
    "sampulse-ended-voice": "REPAIRED_DONOR_ONLY",
    "candidate-legacy-file-loaders": "HANDED_OFF",
    "it-note-cut-translation": "HANDED_OFF",
    "midi-native-instrument-selection": "HANDED_OFF",
    "original-runtime-sampler": "HANDED_OFF",
}
OBSERVATIONS = {
    "historical-sickmaate": ".github/workflows/phase6c-historical-it-private.yml",
    "midi-real-world-corpus": ".github/workflows/phase6c-midi-corpus-private.yml",
}


def fail(message: str) -> None:
    raise ValueError(message)


def reject_duplicate_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            fail(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def read_json(path: pathlib.Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=reject_duplicate_keys)
    if not isinstance(data, dict):
        fail(f"JSON root must be an object: {path.name}")
    return data


def digest(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def local_file(root: pathlib.Path, name: Any) -> pathlib.Path:
    if not isinstance(name, str) or not name or "\\" in name:
        fail("artifact/repository path must be a nonempty POSIX path")
    relative = pathlib.PurePosixPath(name)
    if relative.is_absolute() or ".." in relative.parts:
        fail(f"path escapes root: {name}")
    path = root / name
    if not path.resolve().is_relative_to(root.resolve()) or not path.is_file():
        fail(f"missing or escaping file: {name}")
    return path


def load_helper(filename: str):
    spec = importlib.util.spec_from_file_location(filename.replace("-", "_"), ROOT / "scripts" / filename)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def validate_ledger(ledger: dict[str, Any]) -> None:
    required = {
        "schema_version": 1,
        "phase": "6C",
        "contract": CONTRACT,
        "implementation_status": "COMPLETE",
        "parity_status": "UNKNOWN",
        "resume_contract": "sampler-ps1-extended-timing",
    }
    for key, expected in required.items():
        if type(ledger.get(key)) is not type(expected) or ledger[key] != expected:
            fail(f"closeout {key} changed")
    if ledger.get("preserved_inputs") != PINS:
        fail("closeout preserved input identities changed")
    for path, expected in PINS.items():
        if digest(local_file(ROOT, path)) != expected:
            fail(f"preserved input hash mismatch: {path}")
    items = ledger.get("dispositions")
    if not isinstance(items, list) or len(items) != len(DISPOSITIONS):
        fail("closeout disposition set is incomplete")
    seen = set()
    for item in items:
        if not isinstance(item, dict):
            fail("disposition must be an object")
        identifier = item.get("id")
        if not isinstance(identifier, str) or identifier in seen or identifier not in DISPOSITIONS:
            fail("unknown or duplicate disposition")
        seen.add(identifier)
        if item.get("status") != DISPOSITIONS[identifier]:
            fail(f"{identifier}: invalid disposition status")
        for field in ("scope", "next_action"):
            if not isinstance(item.get(field), str) or not item[field].strip():
                fail(f"{identifier}: {field} is required")
        references = item.get("references")
        if not isinstance(references, list) or not references:
            fail(f"{identifier}: references are required")
        for reference in references:
            local_file(ROOT, reference)
    observations = ledger.get("pending_observations")
    if not isinstance(observations, dict) or set(observations) != set(OBSERVATIONS):
        fail("closeout pending observation set changed")
    for identifier, workflow in OBSERVATIONS.items():
        item = observations[identifier]
        if not isinstance(item, dict) or item.get("status") != "PENDING_PRIVATE_DISPATCH" or item.get("workflow") != workflow:
            fail(f"{identifier}: pending observation boundary changed")
        local_file(ROOT, workflow)
    local_file(ROOT, "phase6c/SAMPLER_PS1.md")


def audit_artifact(root: pathlib.Path) -> None:
    if not root.is_dir():
        fail("observation artifact root is missing")
    # Reject links before the lane validators read any support file.
    if root.is_symlink() or any(path.is_symlink() for path in root.rglob("*")):
        fail("observation artifacts must not contain symlinks")
    load_helper("phase6c-midi-corpus.py").audit_public_tree(root)
    for path in root.rglob("*"):
        if path.is_file():
            prefix = path.read_bytes()[:12]
            wave = prefix[:4] in {b"RIFF", b"RIFX", b"RF64"} and prefix[8:12] == b"WAVE"
            if path.suffix.lower() in {".it", ".wav"} or prefix.startswith(b"IMPM") or wave:
                fail("observation artifact contains raw IT/audio payload")


def revalidate_summary(root: pathlib.Path, lane: str) -> dict[str, Any]:
    audit_artifact(root)
    # Recreate the complete summary from its component files. Reading a top-level
    # success flag or hash alone would admit an incomplete/edited observation.
    with tempfile.TemporaryDirectory() as temporary:
        output = pathlib.Path(temporary) / "summary.json"
        if lane == "historical-sickmaate":
            helper = load_helper("phase6c-historical-it.py")
            summary = local_file(root, "historical-sickmaate-three-way.json")
            with contextlib.redirect_stdout(io.StringIO()):
                helper.command_summary(argparse.Namespace(
                    donor=local_file(root, "donor/cpsycle-historical-it.json"),
                    candidate=local_file(root, "candidate/candidate-historical-it.json"),
                    original=local_file(root, "original/original-historical-legacy-it.json"),
                    output=output,
                ))
        elif lane == "midi-real-world-corpus":
            helper = load_helper("phase6c-midi-corpus.py")
            summary = local_file(root, "midi-real-world-corpus-summary.json")
            helper.corpus_summary(
                local_file(root, "donor/donor-midi-corpus.json"),
                local_file(root, "candidate/candidate-midi-boundary.json"), output,
            )
        else:
            fail("unknown observation lane")
        if read_json(summary) != read_json(output):
            fail(f"{lane}: summary differs from validated components")
        return {"status": "COMPONENTS_REVALIDATED", "summary_sha256": digest(summary)}


def closeout_report(ledger: dict[str, Any], artifacts: dict[str, pathlib.Path]) -> dict[str, Any]:
    validate_ledger(ledger)
    if set(artifacts) - set(OBSERVATIONS):
        fail("unknown observation artifact")
    observations = {
        lane: revalidate_summary(artifacts[lane], lane) if lane in artifacts
        else {"status": "PENDING_PRIVATE_DISPATCH"}
        for lane in OBSERVATIONS
    }
    return {
        "schema_version": 1, "phase": "6C", "contract": CONTRACT,
        "evidence_role": "implementation-closeout-and-observation-handoff",
        "implementation_status": "COMPLETE",
        "observations": observations,
        "observation_summaries_revalidated": all(item["status"] == "COMPONENTS_REVALIDATED" for item in observations.values()),
        # Summary integrity is narrower than independent execution/behavioural
        # verification. The maintainer must review the observed outcomes against
        # the priority-hold exit criteria; this tool does not authorize expansion.
        "priority_hold": "AWAITING_OBSERVATION_REVIEW",
        "resume_contract": ledger["resume_contract"],
        "parity_status": "UNKNOWN",
        "matrix_sha256": PINS["phase6c/compatibility-matrix.json"],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--historical-artifact", type=pathlib.Path)
    parser.add_argument("--midi-artifact", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()
    artifacts = {
        lane: path for lane, path in (
            ("historical-sickmaate", args.historical_artifact),
            ("midi-real-world-corpus", args.midi_artifact),
        ) if path is not None
    }
    try:
        report = closeout_report(read_json(LEDGER), artifacts)
    except (OSError, ValueError, TypeError, KeyError) as exc:
        parser.exit(2, f"phase6c-legacy-closeout: {exc}\n")
    rendered = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
