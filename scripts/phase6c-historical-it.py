#!/usr/bin/env python3
"""Private-input helpers for the Phase 6C historical SickMaate IT witness."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import pathlib
import shutil
import subprocess
import sys
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "phase6c/evidence/legacy-module-import/historical-sickmaate.json"
EXPECTED_CONTRACT = "legacy-impulse-tracker-import-reference"
SUMMARY_CONTRACT = "legacy-impulse-tracker-three-way-summary"
EXPECTED_REFERENCE_BUILD = "Psycle 1.12.0 x86"
EXPECTED_REFERENCE_FILE = "PsycleInstallerx86-1.12.0.exe"
EXPECTED_REFERENCE_INSTALLER_SHA256 = (
    "f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769"
)
EXPECTED_REFERENCE_INSTALLER_SIZE = 9322919
EXPECTED_REFERENCE_EXECUTABLE_SHA256 = (
    "fdb130d2465d5b4a4acfbfe0bfb2368926380fe07a383c4774f0951591b6d6b6"
)
BUILD_IDENTITY_FIELDS = (
    "candidate_baseline_sha256", "source_revision", "repository_commit",
    "build_target", "plugin_interface_git_blob", "diversalis_revision",
    "diversalis_file_count", "diversalis_manifest_sha256", "clean_rebuild_sha256",
)


def candidate_build_identity(player: pathlib.Path, attestation: pathlib.Path) -> dict[str, Any]:
    """Reuse the exact frozen-player attestation contract from the MIDI lane."""
    spec = importlib.util.spec_from_file_location(
        "historical_candidate_build", ROOT / "scripts/phase6c-midi-corpus.py"
    )
    helper = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(helper)
    with player.open("rb") as handle:
        if handle.read(4) != b"\x7fELF":
            die("historical candidate player is not an ELF executable")
    verified = helper.validate_candidate_attestation(
        attestation, player, helper.repository_commit()
    )
    return {field: verified[field] for field in BUILD_IDENTITY_FIELDS}


def die(message: str) -> "NoReturn":
    raise SystemExit(f"phase6c-historical-it: {message}")


def load_json(path: pathlib.Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        die(f"cannot read JSON {path}: {exc}")
    if not isinstance(data, dict):
        die(f"JSON root must be an object: {path}")
    return data


def manifest_source() -> dict[str, Any]:
    manifest = load_json(MANIFEST)
    if manifest.get("contract") != EXPECTED_CONTRACT:
        die("historical manifest contract changed")
    source = manifest.get("source")
    if not isinstance(source, dict):
        die("historical manifest source must be an object")
    return source


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_historical_file(path: pathlib.Path) -> dict[str, Any]:
    source = manifest_source()
    if not path.is_file():
        die(f"historical IT input is missing: {path}")
    size = path.stat().st_size
    expected_size = source.get("size_bytes")
    if size != expected_size:
        die(f"historical IT size mismatch: expected={expected_size} actual={size}")
    actual_sha = sha256_file(path)
    expected_sha = source.get("sha256")
    if actual_sha != expected_sha:
        die(f"historical IT SHA-256 mismatch: expected={expected_sha} actual={actual_sha}")
    with path.open("rb") as handle:
        header = handle.read(30)
    if len(header) < 30 or header[:4] != b"IMPM":
        die("historical IT input lacks the IMPM header")
    title = header[4:30].split(b"\0", 1)[0].decode("cp437", errors="replace")
    if title != source.get("title"):
        die(f"historical IT title mismatch: expected={source.get('title')!r} actual={title!r}")
    return {
        "filename": source["filename"],
        "sha256": actual_sha,
        "size_bytes": size,
        "title": title,
        "redistribution": "external-hash-bound",
    }


def write_json(path: pathlib.Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def command_verify(args: argparse.Namespace) -> None:
    identity = validate_historical_file(args.input.resolve())
    if args.output:
        write_json(args.output, {
            "schema_version": 1,
            "phase": "6C",
            "contract": EXPECTED_CONTRACT,
            "evidence_role": "external-input-identity",
            "source": identity,
            "parity_status": "UNKNOWN",
        })
    print(json.dumps(identity, sort_keys=True))


def command_prepare(args: argparse.Namespace) -> None:
    source_path = args.input.resolve()
    identity = validate_historical_file(source_path)
    root = args.root.resolve()
    private_dir = root / "private"
    private_dir.mkdir(parents=True, exist_ok=True)
    destination = private_dir / identity["filename"]
    shutil.copyfile(source_path, destination)
    if sha256_file(destination) != identity["sha256"]:
        die("private historical IT copy changed after preparation")
    receipt = {
        "schema_version": 1,
        "phase": "6C",
        "contract": EXPECTED_CONTRACT,
        "evidence_role": "candidate",
        "fixture": destination.relative_to(root).as_posix(),
        "fixture_sha256": identity["sha256"],
        "fixture_size_bytes": identity["size_bytes"],
        "fixture_distribution": "external-private-input",
        "procedure": (
            "verify the operator-supplied historical IT by manifest-bound size, "
            "SHA-256, IMPM magic and title; copy it only into the private working "
            "root for transient observation; never upload or commit the module bytes"
        ),
        "observation": "external-input-prepared",
        "parity_status": "UNKNOWN",
    }
    write_json(root / "candidate-historical-legacy-it.json", receipt)
    write_json(root / "historical-input-identity.json", {
        "schema_version": 1,
        "phase": "6C",
        "contract": EXPECTED_CONTRACT,
        "evidence_role": "external-input-identity",
        "source": identity,
        "parity_status": "UNKNOWN",
    })
    print(root / "candidate-historical-legacy-it.json")


def command_candidate(args: argparse.Namespace) -> None:
    input_path = args.input.resolve()
    player = args.player.resolve()
    identity = validate_historical_file(input_path)
    if not player.is_file():
        die(f"candidate player is missing: {player}")
    attestation = args.attestation.resolve()
    if attestation.parent != args.output.resolve().parent:
        die("historical candidate attestation must be retained beside its receipt")
    build_identity = candidate_build_identity(player, attestation)
    player_sha = sha256_file(player)
    if input_path.parent != player.parent:
        die("candidate replay requires the private IT beside the candidate player")
    command = [
        f"./{player.name}",
        "--output-driver",
        "dummy",
        "--input-file",
        input_path.name,
    ]
    proc = subprocess.run(
        command,
        cwd=player.parent,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    raw = proc.stdout
    diagnostic = b"could not load song file:" in raw
    result = {
        "schema_version": 1,
        "phase": "6C",
        "contract": EXPECTED_CONTRACT,
        "evidence_role": "candidate-observation",
        "source_sha256": identity["sha256"],
        "source_size_bytes": identity["size_bytes"],
        "player": "candidate-psycle-player",
        "player_sha256": player_sha,
        **build_identity,
        "build_attestation": attestation.name,
        "build_attestation_sha256": sha256_file(attestation),
        "command": [
            "./candidate-psycle-player",
            "--output-driver",
            "dummy",
            "--input-file",
            identity["filename"],
        ],
        "working_directory": "artifact-root-with-private-input",
        "replay_setup": ["chmod", "+x", "candidate-psycle-player"],
        "private_input_required": True,
        "private_input_filename": identity["filename"],
        "stdin": "/dev/null",
        "procedure": (
            "chmod +x candidate-psycle-player && ./candidate-psycle-player "
            "--output-driver dummy --input-file d-503_-_sickmaate.it "
            "</dev/null; historical IT bytes must be supplied privately and "
            "verified before replay"
        ),
        "exit_code": proc.returncode,
        "diagnostic_could_not_load_song_file": diagnostic,
        "raw_output_sha256": sha256_bytes(raw),
        "direct_it_load": "rejected" if proc.returncode == 2 and diagnostic else "inconclusive",
        "parity_status": "UNKNOWN",
        "note": (
            "Candidate-only direct-load capability observation. Raw output is hashed "
            "but not retained to keep the public historical summary minimal."
        ),
    }
    write_json(args.output, result)
    if result["direct_it_load"] == "inconclusive":
        die(
            "candidate historical IT observation was not the expected scoped "
            f"direct-load rejection: exit={proc.returncode}"
        )


def receipt_sha(path: pathlib.Path) -> str:
    return sha256_file(path)


def require_hash(value: Any, label: str) -> str:
    if not isinstance(value, str) or len(value) != 64:
        die(f"{label} must be a SHA-256 string")
    try:
        int(value, 16)
    except ValueError:
        die(f"{label} must be hexadecimal")
    return value.lower()


def require_common_receipt(
    receipt: dict[str, Any],
    *,
    label: str,
    evidence_role: str,
) -> None:
    if receipt.get("schema_version") != 1:
        die(f"{label} receipt has unexpected schema_version")
    if receipt.get("phase") != "6C":
        die(f"{label} receipt has unexpected phase")
    if receipt.get("contract") != EXPECTED_CONTRACT:
        die(f"{label} receipt has unexpected contract")
    if receipt.get("evidence_role") != evidence_role:
        die(f"{label} receipt has unexpected evidence_role")
    if receipt.get("parity_status") != "UNKNOWN":
        die(f"{label} receipt attempted to promote parity")


def require_public_support_file(
    receipt_path: pathlib.Path,
    relative_name: Any,
    expected_sha: Any,
    label: str,
) -> None:
    if not isinstance(relative_name, str) or not relative_name or pathlib.PurePosixPath(relative_name).is_absolute():
        die(f"{label} support path is invalid")
    relative = pathlib.PurePosixPath(relative_name)
    if ".." in relative.parts:
        die(f"{label} support path escapes its component root")
    path = receipt_path.parent / pathlib.Path(*relative.parts)
    if not path.is_file():
        die(f"{label} support file is missing: {relative_name}")
    expected = require_hash(expected_sha, f"{label} support hash")
    if sha256_file(path) != expected:
        die(f"{label} support file hash mismatch: {relative_name}")


def require_original_reference_identity(original: dict[str, Any]) -> None:
    if original.get("reference_build") != EXPECTED_REFERENCE_BUILD:
        die("original historical receipt is bound to the wrong Psycle reference build")
    if original.get("reference_file") != EXPECTED_REFERENCE_FILE:
        die("original historical receipt is bound to the wrong Psycle installer file")
    if original.get("reference_installer_sha256") != EXPECTED_REFERENCE_INSTALLER_SHA256:
        die("original historical receipt is bound to the wrong Psycle installer SHA-256")
    if original.get("reference_installer_size_bytes") != EXPECTED_REFERENCE_INSTALLER_SIZE:
        die("original historical receipt is bound to the wrong Psycle installer size")
    if original.get("reference_executable_sha256") != EXPECTED_REFERENCE_EXECUTABLE_SHA256:
        die("original historical receipt is bound to the wrong Psycle executable SHA-256")


def validate_summary_components(
    *,
    source: dict[str, Any],
    donor_path: pathlib.Path,
    donor: dict[str, Any],
    candidate_path: pathlib.Path,
    candidate: dict[str, Any],
    original_path: pathlib.Path,
    original: dict[str, Any],
) -> None:
    expected_sha = source["sha256"]
    expected_size = source["size_bytes"]

    require_common_receipt(
        donor,
        label="donor",
        evidence_role="cpsycle-donor-observation",
    )
    if donor.get("source_sha256") != expected_sha or donor.get("source_size_bytes") != expected_size:
        die("donor receipt is not bound to the historical source identity")
    if donor.get("load_result") != "accepted":
        die("donor historical observation must record load_result=accepted")
    if donor.get("non_silent_playback") is not True:
        die("donor historical observation must record non_silent_playback=true")
    if donor.get("fixture_redistributed") is not False:
        die("donor historical receipt must record fixture_redistributed=false")
    if donor.get("private_input_required") is not True:
        die("donor historical receipt must record private_input_required=true")
    require_public_support_file(
        donor_path,
        donor.get("source_file"),
        donor.get("source_file_sha256"),
        "donor source",
    )
    require_public_support_file(
        donor_path,
        donor.get("probe_executable"),
        donor.get("probe_executable_sha256"),
        "donor probe",
    )
    require_hash(donor.get("raw_output_sha256"), "donor raw_output_sha256")

    require_common_receipt(
        candidate,
        label="candidate",
        evidence_role="candidate-observation",
    )
    if candidate.get("source_sha256") != expected_sha or candidate.get("source_size_bytes") != expected_size:
        die("candidate receipt is not bound to the historical source identity")
    if candidate.get("direct_it_load") != "rejected":
        die("candidate historical observation must record direct_it_load=rejected")
    if candidate.get("diagnostic_could_not_load_song_file") is not True:
        die("candidate historical rejection lacks its scoped load diagnostic")
    if candidate.get("exit_code") != 2:
        die("candidate historical direct-load rejection exit code changed")
    if candidate.get("private_input_required") is not True:
        die("candidate historical receipt must record private_input_required=true")
    require_public_support_file(
        candidate_path,
        candidate.get("player"),
        candidate.get("player_sha256"),
        "candidate player",
    )
    require_public_support_file(
        candidate_path,
        candidate.get("build_attestation"),
        candidate.get("build_attestation_sha256"),
        "candidate build attestation",
    )
    build_identity = candidate_build_identity(
        candidate_path.parent / candidate["player"],
        candidate_path.parent / candidate["build_attestation"],
    )
    if any(candidate.get(field) != value for field, value in build_identity.items()):
        die("historical candidate receipt differs from its frozen build identity")
    require_hash(candidate.get("raw_output_sha256"), "candidate raw_output_sha256")

    require_common_receipt(
        original,
        label="original",
        evidence_role="original",
    )
    if original.get("fixture_sha256") != expected_sha:
        die("original receipt is not bound to the historical source identity")
    if original.get("fixture_distribution") != "external-hash-bound":
        die("original historical receipt must record external-hash-bound distribution")
    if original.get("fixture_redistributed") is not False:
        die("original historical receipt must record fixture_redistributed=false")
    if original.get("original_psycle_observed") is not True:
        die("original receipt does not record original_psycle_observed=true")
    require_original_reference_identity(original)
    load_result = original.get("load_result")
    if load_result not in {"accepted", "rejected", "inconclusive"}:
        die("original historical receipt has an invalid load_result")
    observation = original.get("observation")
    if not isinstance(observation, str) or not observation.strip():
        die("original historical receipt has no observation")
    for field in ("stdout", "stderr", "ui_evidence"):
        mapping = original.get(field)
        if not isinstance(mapping, dict):
            die(f"original historical receipt {field} must be an object")
        require_public_support_file(
            original_path,
            mapping.get("path"),
            mapping.get("sha256"),
            f"original {field}",
        )
    environment = original.get("environment")
    if not isinstance(environment, dict):
        die("original historical receipt environment must be an object")
    for inventory_key in ("loaded_vc90_runtime", "machine_plugin_inventory"):
        inventory = environment.get(inventory_key)
        if not isinstance(inventory, dict):
            die(f"original historical receipt lacks {inventory_key}")
        require_public_support_file(
            original_path,
            inventory.get("path"),
            inventory.get("sha256"),
            f"original {inventory_key}",
        )


def command_summary(args: argparse.Namespace) -> None:
    source = manifest_source()
    donor = load_json(args.donor)
    candidate = load_json(args.candidate)
    original = load_json(args.original)
    expected_sha = source["sha256"]

    validate_summary_components(
        source=source,
        donor_path=args.donor,
        donor=donor,
        candidate_path=args.candidate,
        candidate=candidate,
        original_path=args.original,
        original=original,
    )

    summary = {
        "schema_version": 1,
        "phase": "6C",
        "contract": SUMMARY_CONTRACT,
        "source": {
            "filename": source["filename"],
            "sha256": expected_sha,
            "size_bytes": source["size_bytes"],
            "title": source["title"],
            "redistribution": "not_committed",
        },
        "observations": {
            "cpsycle_donor": {
                "artifact_root": "donor",
                "receipt": f"donor/{args.donor.name}",
                "receipt_sha256": receipt_sha(args.donor),
                "load_result": donor.get("load_result"),
                "non_silent_playback": donor.get("non_silent_playback"),
            },
            "candidate": {
                "artifact_root": "candidate",
                "receipt": f"candidate/{args.candidate.name}",
                "receipt_sha256": receipt_sha(args.candidate),
                "direct_it_load": candidate.get("direct_it_load"),
            },
            "original_psycle_1_12_0_x86": {
                "artifact_root": "original",
                "receipt": f"original/{args.original.name}",
                "receipt_sha256": receipt_sha(args.original),
                "load_result": original.get("load_result"),
                "observation": original.get("observation"),
            },
        },
        "parity_status": "UNKNOWN",
        "classification_note": (
            "Descriptive three-way historical observation only. This summary does not "
            "promote any compatibility-matrix row and does not treat C-Psycle or the "
            "historical module as an oracle for original Psycle."
        ),
        "redistribution_check": "summary contains identity and observation metadata only; historical IT bytes are excluded",
    }
    write_json(args.output, summary)
    print(args.output)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    verify = sub.add_parser("verify", help="verify the exact external historical IT")
    verify.add_argument("input", type=pathlib.Path)
    verify.add_argument("--output", type=pathlib.Path)
    verify.set_defaults(func=command_verify)

    prepare = sub.add_parser("prepare-private", help="prepare a private observer input root")
    prepare.add_argument("input", type=pathlib.Path)
    prepare.add_argument("root", type=pathlib.Path)
    prepare.set_defaults(func=command_prepare)

    candidate = sub.add_parser("candidate", help="observe frozen candidate direct IT load")
    candidate.add_argument("input", type=pathlib.Path)
    candidate.add_argument("player", type=pathlib.Path)
    candidate.add_argument("output", type=pathlib.Path)
    candidate.add_argument("--attestation", type=pathlib.Path, required=True)
    candidate.set_defaults(func=command_candidate)

    summary = sub.add_parser("summary", help="build a public-safe three-way summary")
    summary.add_argument("--donor", type=pathlib.Path, required=True)
    summary.add_argument("--candidate", type=pathlib.Path, required=True)
    summary.add_argument("--original", type=pathlib.Path, required=True)
    summary.add_argument("--output", type=pathlib.Path, required=True)
    summary.set_defaults(func=command_summary)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
