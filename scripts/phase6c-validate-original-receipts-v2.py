#!/usr/bin/env python3
"""Validate native-Windows original-Psycle Phase 6C evidence artifacts.

This validator proves evidence integrity only. It does not classify parity.
PASS/DIFFERENT/MISSING still require committed original and candidate receipts
plus a committed comparison verdict accepted by phase6c-validate-matrix.py.
"""

from __future__ import annotations

import hashlib
import json
import pathlib
import re
import sys

EXPECTED_REFERENCE_FILE = "PsycleInstallerx86-1.12.0.exe"
EXPECTED_REFERENCE_BUILD = "Psycle 1.12.0 x86"
EXPECTED_REFERENCE_SHA256 = (
    "f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769"
)
EXPECTED_REFERENCE_SIZE = 9322919
EXPECTED_VC90_REDISTRIBUTABLE_VERSION = "9.0.30729.6161"
VC90_RUNTIME_FAMILY = "9.0"
VC90_VERSION_PREFIX = VC90_RUNTIME_FAMILY + "."
EXPECTED_VC90_REDISTRIBUTABLE_URL = (
    "https://download.microsoft.com/download/5/D/8/"
    "5D8C65CB-C849-4025-8E95-C3966CAFD8AE/vcredist_x86.exe"
)
EXPECTED_VC90_REDISTRIBUTABLE_SHA256 = (
    "8742bcbf24ef328a72d2a27b693cc7071e38d3bb4b9b44dec42aa3d2c8d61d92"
)
EXPECTED = {
    "psy2": "project-io-psy2-parse",
    "psy3": "project-io-psy3-parse",
}
VC90_MODULE_NAMES = {
    "msvcr90.dll",
    "msvcp90.dll",
    "mfc90.dll",
    "mfc90u.dll",
    "atl90.dll",
}
ALLOWED_LOAD_RESULT = {"accepted", "rejected", "inconclusive"}
ALLOWED_TERMINATION = {
    "already-exited",
    "exited-before-close-request",
    "killed-without-closeable-main-window",
    "killed-after-observation",
    "closed-after-observation",
    "exited-during-close-error",
    "killed-after-close-error",
    "termination-error",
}
ACCEPTED_TERMINATION = {
    "killed-without-closeable-main-window",
    "killed-after-observation",
    "closed-after-observation",
    "killed-after-close-error",
}
ALLOWED_ARTIFACT_SUFFIXES = {".json", ".txt", ".log", ".png", ".md", ".psy"}
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def die(message: str) -> None:
    raise SystemExit(f"phase6c-validate-original-receipts: {message}")


def load_json(path: pathlib.Path) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8-sig"))
    except FileNotFoundError:
        die(f"missing JSON receipt: {path}")
    except json.JSONDecodeError as exc:
        die(f"invalid JSON in {path}: {exc}")
    if not isinstance(value, dict):
        die(f"receipt must be an object: {path}")
    return value


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require_hash(value: object, context: str) -> str:
    if not isinstance(value, str) or SHA256_RE.fullmatch(value) is None:
        die(f"{context} must be a lowercase SHA-256 digest")
    return value


def resolve_artifact_path(root: pathlib.Path, value: object, context: str) -> pathlib.Path:
    if not isinstance(value, str) or not value.strip():
        die(f"{context} must be a non-empty artifact-relative path")
    relative = pathlib.PurePosixPath(value.replace("\\", "/"))
    if relative.is_absolute() or ".." in relative.parts:
        die(f"{context} must remain artifact-relative")
    path = (root / pathlib.Path(*relative.parts)).resolve()
    try:
        path.relative_to(root)
    except ValueError:
        die(f"{context} escapes artifact root")
    if not path.is_file():
        die(f"{context} is missing: {value}")
    return path


def require_artifact_file(
    root: pathlib.Path, mapping: object, context: str, *, optional: bool = False
) -> None:
    if mapping is None and optional:
        return
    if not isinstance(mapping, dict):
        die(f"{context} must be an object")
    path = resolve_artifact_path(root, mapping.get("path"), f"{context}.path")
    expected_hash = require_hash(mapping.get("sha256"), f"{context}.sha256")
    actual_hash = sha256(path)
    if actual_hash != expected_hash:
        die(f"{context} SHA-256 mismatch: expected={expected_hash} actual={actual_hash}")


def validate_redistributable(
    environment: dict[str, object], original_root: pathlib.Path, name: str
) -> None:
    redistributable = environment.get("vc90_redistributable")
    if not isinstance(redistributable, dict):
        die(f"original-{name}.environment.vc90_redistributable must be an object")
    if (
        redistributable.get("redistributable_version")
        != EXPECTED_VC90_REDISTRIBUTABLE_VERSION
    ):
        die(f"original-{name} has the wrong VC90 redistributable version")
    if redistributable.get("url") != EXPECTED_VC90_REDISTRIBUTABLE_URL:
        die(f"original-{name} has the wrong VC90 redistributable URL")
    if redistributable.get("sha256") != EXPECTED_VC90_REDISTRIBUTABLE_SHA256:
        die(f"original-{name} has the wrong VC90 redistributable SHA-256")

    receipt_path = resolve_artifact_path(
        original_root,
        redistributable.get("receipt"),
        f"original-{name}.environment.vc90_redistributable.receipt",
    )
    receipt = receipt_path.read_text(encoding="utf-8-sig")
    required_lines = (
        f"redistributable_version={EXPECTED_VC90_REDISTRIBUTABLE_VERSION}",
        f"url={EXPECTED_VC90_REDISTRIBUTABLE_URL}",
        f"sha256={EXPECTED_VC90_REDISTRIBUTABLE_SHA256}",
        "authenticode_status=Valid",
        "signer_subject=",
    )
    for value in required_lines:
        if value not in receipt:
            die(f"original-{name} VC90 redistributable receipt lacks {value!r}")
    signer_line = next(
        (line for line in receipt.splitlines() if line.startswith("signer_subject=")),
        "",
    )
    if "microsoft" not in signer_line.lower():
        die(f"original-{name} VC90 redistributable receipt is not signed by Microsoft")


def validate_loaded_vc90_runtime(
    environment: dict[str, object],
    original_root: pathlib.Path,
    name: str,
) -> tuple[list[str], bool]:
    mapping = environment.get("loaded_vc90_runtime")
    if not isinstance(mapping, dict):
        die(f"original-{name}.environment.loaded_vc90_runtime must be an object")

    inventory_ref = mapping.get("path")
    expected_ref = f"loaded-vc90-runtime-{name}.json"
    if inventory_ref != expected_ref:
        die(
            f"original-{name} is not bound to its fixture-specific "
            "loaded VC90 runtime inventory"
        )
    inventory_path = resolve_artifact_path(
        original_root,
        inventory_ref,
        f"original-{name}.environment.loaded_vc90_runtime.path",
    )
    expected_hash = require_hash(
        mapping.get("sha256"),
        f"original-{name}.environment.loaded_vc90_runtime.sha256",
    )
    actual_hash = sha256(inventory_path)
    if actual_hash != expected_hash:
        die(
            f"original-{name} loaded VC90 runtime inventory SHA-256 mismatch: "
            f"expected={expected_hash} actual={actual_hash}"
        )

    inventory = load_json(inventory_path)
    if inventory.get("schema_version") != 1:
        die(f"original-{name} loaded VC90 runtime inventory has the wrong schema version")
    if inventory.get("reference_build") != EXPECTED_REFERENCE_BUILD:
        die(f"original-{name} loaded VC90 runtime inventory has the wrong reference build")
    if inventory.get("required_vc90_family") != VC90_RUNTIME_FAMILY:
        die(f"original-{name} loaded VC90 runtime inventory has the wrong runtime family")

    process_id = inventory.get("process_id")
    if not isinstance(process_id, int) or isinstance(process_id, bool) or process_id <= 0:
        die(f"original-{name} loaded VC90 runtime inventory has an invalid process_id")

    diagnostics = inventory.get("diagnostics")
    if not isinstance(diagnostics, list) or any(
        not isinstance(value, str) or not value.strip() for value in diagnostics
    ):
        die(f"original-{name} loaded VC90 runtime diagnostics must be a string array")

    modules = inventory.get("modules")
    if not isinstance(modules, list):
        die(f"original-{name} loaded VC90 runtime modules must be an array")

    seen_names: set[str] = set()
    seen_paths: set[str] = set()
    has_msvcr90 = False
    identity_valid = True
    for index, module in enumerate(modules):
        context = f"original-{name}.loaded_vc90_runtime.modules[{index}]"
        if not isinstance(module, dict):
            die(f"{context} must be an object")

        module_name = module.get("name")
        if not isinstance(module_name, str) or module_name.lower() not in VC90_MODULE_NAMES:
            die(f"{context}.name is not a recognized VC90 runtime module")
        module_name = module_name.lower()
        if module_name in seen_names:
            die(f"{context}.name is duplicated")
        seen_names.add(module_name)
        if module_name == "msvcr90.dll":
            has_msvcr90 = True

        module_path = module.get("path")
        if not isinstance(module_path, str) or not module_path.strip():
            die(f"{context}.path must be non-empty")
        pure_path = pathlib.PureWindowsPath(module_path)
        if not pure_path.is_absolute():
            die(f"{context}.path must be an absolute Windows path")
        normalized_path = str(pure_path).lower()
        if normalized_path in seen_paths:
            die(f"{context}.path is duplicated")
        seen_paths.add(normalized_path)

        size = module.get("size_bytes")
        if not isinstance(size, int) or isinstance(size, bool) or size <= 0:
            die(f"{context}.size_bytes must be a positive integer")
        require_hash(module.get("sha256"), f"{context}.sha256")

        file_version = module.get("file_version")
        if not isinstance(file_version, str) or not file_version.startswith(
            VC90_VERSION_PREFIX
        ):
            identity_valid = False

        raw_version = module.get("file_version_raw")
        product_version = module.get("product_version")
        if raw_version is not None and not isinstance(raw_version, str):
            die(f"{context}.file_version_raw must be string or null")
        if product_version is not None and not isinstance(product_version, str):
            die(f"{context}.product_version must be string or null")

    if not has_msvcr90:
        identity_valid = False

    if not identity_valid and not diagnostics:
        die(
            f"original-{name} loaded VC90 runtime identity is missing or outside "
            "the VC90 9.0 family without a recorded runtime diagnostic"
        )

    return diagnostics, identity_valid


def validate_machine_plugin_inventory(
    environment: dict[str, object],
    original_root: pathlib.Path,
    name: str,
    reference_executable_hash: str,
) -> None:
    mapping = environment.get("machine_plugin_inventory")
    if not isinstance(mapping, dict):
        die(f"original-{name}.environment.machine_plugin_inventory must be an object")

    inventory_ref = mapping.get("path")
    if inventory_ref != f"machine-plugin-inventory-{name}.json":
        die(f"original-{name} is not bound to its fixture-specific machine/plugin inventory")
    inventory_path = resolve_artifact_path(
        original_root,
        inventory_ref,
        f"original-{name}.environment.machine_plugin_inventory.path",
    )
    expected_hash = require_hash(
        mapping.get("sha256"),
        f"original-{name}.environment.machine_plugin_inventory.sha256",
    )
    actual_hash = sha256(inventory_path)
    if actual_hash != expected_hash:
        die(
            f"original-{name} machine/plugin inventory SHA-256 mismatch: "
            f"expected={expected_hash} actual={actual_hash}"
        )

    inventory = load_json(inventory_path)
    if inventory.get("schema_version") != 1:
        die(f"original-{name} machine/plugin inventory has the wrong schema version")
    if inventory.get("reference_build") != EXPECTED_REFERENCE_BUILD:
        die(f"original-{name} machine/plugin inventory has the wrong reference build")
    if inventory.get("reference_executable_sha256") != reference_executable_hash:
        die(f"original-{name} machine/plugin inventory is bound to the wrong executable")
    if inventory.get("preexisting_psycle_registry") is not False:
        die(f"original-{name} machine/plugin inventory did not start from clean Psycle registry state")

    installed = inventory.get("installed_payload_files")
    if not isinstance(installed, list) or not installed:
        die(f"original-{name} machine/plugin inventory has no installed payload manifest")

    psycle_exe_seen = False
    seen_installed_paths: set[str] = set()
    for index, entry in enumerate(installed):
        context = f"original-{name}.machine_plugin_inventory.installed_payload_files[{index}]"
        if not isinstance(entry, dict):
            die(f"{context} must be an object")
        rel = entry.get("path")
        if not isinstance(rel, str) or not rel.strip():
            die(f"{context}.path must be non-empty")
        pure = pathlib.PurePosixPath(rel.replace("\\", "/"))
        if pure.is_absolute() or ".." in pure.parts:
            die(f"{context}.path must remain relative to the transient install root")
        normalized = pure.as_posix().lower()
        if normalized in seen_installed_paths:
            die(f"{context}.path is duplicated")
        seen_installed_paths.add(normalized)

        size = entry.get("size_bytes")
        if not isinstance(size, int) or isinstance(size, bool) or size < 0:
            die(f"{context}.size_bytes must be a non-negative integer")
        digest = require_hash(entry.get("sha256"), f"{context}.sha256")
        if pure.name.lower() == "psycle.exe":
            if digest != reference_executable_hash:
                die(f"{context} psycle.exe hash does not match the receipt executable")
            psycle_exe_seen = True

    if not psycle_exe_seen:
        die(f"original-{name} machine/plugin inventory does not contain psycle.exe")

    registry = inventory.get("psycle_registry")
    if not isinstance(registry, list):
        die(f"original-{name} machine/plugin inventory psycle_registry must be an array")
    for index, entry in enumerate(registry):
        context = f"original-{name}.machine_plugin_inventory.psycle_registry[{index}]"
        if not isinstance(entry, dict):
            die(f"{context} must be an object")
        for field in ("key", "name", "value"):
            if not isinstance(entry.get(field), str):
                die(f"{context}.{field} must be a string")

    roots = inventory.get("plugin_roots")
    if not isinstance(roots, list) or not roots:
        die(f"original-{name} machine/plugin inventory has no plugin-root audit")
    external_count = 0
    installed_scope_seen = False
    seen_roots: set[str] = set()
    for index, root in enumerate(roots):
        context = f"original-{name}.machine_plugin_inventory.plugin_roots[{index}]"
        if not isinstance(root, dict):
            die(f"{context} must be an object")
        root_path = root.get("path")
        if not isinstance(root_path, str) or not root_path.strip():
            die(f"{context}.path must be non-empty")
        pure_root = pathlib.PureWindowsPath(root_path)
        if not pure_root.is_absolute():
            die(f"{context}.path must be an absolute Windows path")
        normalized_root = str(pure_root).lower()
        if normalized_root in seen_roots:
            die(f"{context}.path is duplicated")
        seen_roots.add(normalized_root)

        scope = root.get("scope")
        if scope not in {"installed-payload", "external"}:
            die(f"{context}.scope is invalid")
        if scope == "installed-payload":
            installed_scope_seen = True
        exists = root.get("exists")
        if not isinstance(exists, bool):
            die(f"{context}.exists must be boolean")
        dlls = root.get("dlls")
        if not isinstance(dlls, list):
            die(f"{context}.dlls must be an array")
        if not exists and dlls:
            die(f"{context} cannot list DLLs for a missing root")

        seen_dlls: set[str] = set()
        for dll_index, dll in enumerate(dlls):
            dll_context = f"{context}.dlls[{dll_index}]"
            if not isinstance(dll, dict):
                die(f"{dll_context} must be an object")
            rel = dll.get("path")
            if not isinstance(rel, str) or not rel.strip():
                die(f"{dll_context}.path must be non-empty")
            pure = pathlib.PurePosixPath(rel.replace("\\", "/"))
            if pure.is_absolute() or ".." in pure.parts:
                die(f"{dll_context}.path must remain relative to the audited plugin root")
            normalized = pure.as_posix().lower()
            if normalized in seen_dlls:
                die(f"{dll_context}.path is duplicated")
            seen_dlls.add(normalized)
            size = dll.get("size_bytes")
            if not isinstance(size, int) or isinstance(size, bool) or size < 0:
                die(f"{dll_context}.size_bytes must be a non-negative integer")
            require_hash(dll.get("sha256"), f"{dll_context}.sha256")
            if scope == "external":
                external_count += 1

    if not installed_scope_seen:
        die(f"original-{name} machine/plugin inventory does not audit the installed plugin root")
    claimed_external_count = inventory.get("external_plugin_dll_count")
    if (
        not isinstance(claimed_external_count, int)
        or isinstance(claimed_external_count, bool)
        or claimed_external_count < 0
    ):
        die(f"original-{name} machine/plugin inventory external_plugin_dll_count is invalid")
    if claimed_external_count != external_count:
        die(
            f"original-{name} machine/plugin inventory external DLL count mismatch: "
            f"claimed={claimed_external_count} actual={external_count}"
        )

    baseline = inventory.get("configuration_baseline")
    if baseline != "post-installer HKCU\\Software\\Psycle snapshot restored before this fixture":
        die(f"original-{name} machine/plugin inventory lacks the isolated registry baseline binding")

    note = inventory.get("external_visibility_note")
    if not isinstance(note, str) or not note.strip():
        die(f"original-{name} machine/plugin inventory lacks its visibility-scope note")


def validate_pair(
    name: str,
    contract: str,
    candidate_root: pathlib.Path,
    original_root: pathlib.Path,
) -> str:
    candidate = load_json(candidate_root / f"candidate-{name}.json")
    original = load_json(original_root / f"original-{name}.json")

    if candidate.get("schema_version") != 1 or candidate.get("phase") != "6C":
        die(f"candidate-{name} has unexpected schema_version/phase")
    if candidate.get("contract") != contract or candidate.get("evidence_role") != "candidate":
        die(f"candidate-{name} is bound to the wrong contract/role")

    if original.get("schema_version") != 1 or original.get("phase") != "6C":
        die(f"original-{name} has unexpected schema_version/phase")
    if original.get("scope") != "original-observation":
        die(f"original-{name} has the wrong scope")
    if original.get("contract") != contract or original.get("evidence_role") != "original":
        die(f"original-{name} is bound to the wrong contract/role")
    if original.get("reference_build") != EXPECTED_REFERENCE_BUILD:
        die(f"original-{name} is bound to the wrong reference build")
    if original.get("reference_file") != EXPECTED_REFERENCE_FILE:
        die(f"original-{name} is bound to the wrong reference file")
    if original.get("reference_installer_sha256") != EXPECTED_REFERENCE_SHA256:
        die(f"original-{name} has the wrong installer SHA-256")
    if original.get("reference_installer_size_bytes") != EXPECTED_REFERENCE_SIZE:
        die(f"original-{name} has the wrong installer size")
    reference_executable_hash = require_hash(
        original.get("reference_executable_sha256"),
        f"original-{name}.reference_executable_sha256",
    )

    candidate_fixture_ref = candidate.get("fixture")
    if not isinstance(candidate_fixture_ref, str) or not candidate_fixture_ref.strip():
        die(f"candidate-{name} has no fixture")
    if original.get("candidate_fixture") != candidate_fixture_ref:
        die(f"original-{name} does not identify the source candidate fixture path")

    candidate_fixture = resolve_artifact_path(
        candidate_root, candidate_fixture_ref, f"candidate-{name}.fixture"
    )
    candidate_fixture_hash = require_hash(
        candidate.get("fixture_sha256"), f"candidate-{name}.fixture_sha256"
    )
    if sha256(candidate_fixture) != candidate_fixture_hash:
        die(f"candidate-{name} fixture bytes do not match its claimed SHA-256")

    original_fixture = resolve_artifact_path(
        original_root, original.get("fixture"), f"original-{name}.fixture"
    )
    original_fixture_hash = require_hash(
        original.get("fixture_sha256"), f"original-{name}.fixture_sha256"
    )
    if sha256(original_fixture) != original_fixture_hash:
        die(f"original-{name} fixture bytes do not match its claimed SHA-256")
    if original_fixture_hash != candidate_fixture_hash:
        die(f"original-{name} fixture bytes differ from the candidate artifact fixture")

    procedure = original.get("procedure")
    if not isinstance(procedure, str) or not procedure.strip():
        die(f"original-{name}.procedure must be non-empty")
    if EXPECTED_VC90_REDISTRIBUTABLE_SHA256 not in procedure:
        die(f"original-{name}.procedure does not bind the pinned VC90 runtime")
    observation = original.get("observation")
    if not isinstance(observation, str) or not observation.strip():
        die(f"original-{name}.observation must be non-empty")

    environment = original.get("environment")
    if not isinstance(environment, dict):
        die(f"original-{name}.environment must be an object")
    if environment.get("observation_mode") != "native-windows-github-runner-transient-installed-payload":
        die(f"original-{name} is not bound to the maintained native-Windows observation mode")
    if environment.get("runner_os") != "Windows":
        die(f"original-{name} runner_os is not Windows")
    if environment.get("installer_framework") not in {"inno-setup", "nsis"}:
        die(f"original-{name} has unsupported installer framework metadata")
    validate_redistributable(environment, original_root, name)
    runtime_inventory_diagnostics, runtime_identity_valid = validate_loaded_vc90_runtime(
        environment, original_root, name
    )
    validate_machine_plugin_inventory(
        environment, original_root, name, reference_executable_hash
    )

    result = original.get("load_result")
    if result not in ALLOWED_LOAD_RESULT:
        die(f"original-{name}.load_result is invalid: {result!r}")

    error_marker = original.get("error_marker")
    application_error_marker = original.get("application_error_marker")
    error_marker_scope = original.get("error_marker_scope")
    marker = original.get("load_evidence_marker")
    stable_polls = original.get("stable_marker_polls")
    diagnostics = original.get("ui_automation_diagnostics")
    startup_bootstrap = original.get("startup_bootstrap")
    environment_bootstrap = original.get("environment_bootstrap")
    runtime_diagnostics = original.get("runtime_identity_diagnostics")
    exit_code = original.get("exit_code_before_termination")
    running_before_termination = original.get("process_running_before_termination")
    termination = original.get("termination")
    if error_marker is not None and (
        not isinstance(error_marker, str) or not error_marker.strip()
    ):
        die(f"original-{name}.error_marker must be null or a non-empty string")
    if application_error_marker is not None and (
        not isinstance(application_error_marker, str)
        or not application_error_marker.strip()
    ):
        die(
            f"original-{name}.application_error_marker must be null "
            "or a non-empty string"
        )
    if error_marker_scope not in {
        "none",
        "fixture-load",
        "application-unassociated",
    }:
        die(f"original-{name}.error_marker_scope is invalid: {error_marker_scope!r}")

    if error_marker_scope == "fixture-load":
        if error_marker is None or application_error_marker is None:
            die(
                f"original-{name} fixture-load error scope requires both "
                "fixture and application error evidence"
            )
        if result != "rejected":
            die(
                f"original-{name} fixture-load error scope must classify "
                "the observation as rejected"
            )
    elif error_marker_scope == "application-unassociated":
        if error_marker is not None or application_error_marker is None:
            die(
                f"original-{name} application-unassociated error scope must "
                "contain only application-level error evidence"
            )
        if result != "inconclusive":
            die(
                f"original-{name} unassociated application errors must force "
                "an inconclusive result"
            )
    else:
        if error_marker is not None or application_error_marker is not None:
            die(
                f"original-{name} error_marker_scope=none conflicts with "
                "recorded application error evidence"
            )

    if not isinstance(diagnostics, list) or any(not isinstance(x, str) for x in diagnostics):
        die(f"original-{name}.ui_automation_diagnostics must be a string array")

    if not isinstance(startup_bootstrap, dict):
        die(f"original-{name}.startup_bootstrap must be an object")
    for field in (
        "settings_dialog_seen",
        "signature_verified",
        "attempted",
        "dismissed",
    ):
        if not isinstance(startup_bootstrap.get(field), bool):
            die(f"original-{name}.startup_bootstrap.{field} must be boolean")
    action = startup_bootstrap.get("action")
    if action is not None and action != "invoke-ok":
        die(f"original-{name}.startup_bootstrap.action is invalid")
    outcome = startup_bootstrap.get("outcome")
    allowed_bootstrap_outcomes = {
        "not-seen",
        "dismissed",
        "desktop-root-missing",
        "signature-read-failed",
        "signature-mismatch",
        "invoke-failed",
        "close-verification-failed",
        "close-timeout",
        "automation-failed",
    }
    if outcome not in allowed_bootstrap_outcomes:
        die(f"original-{name}.startup_bootstrap.outcome is invalid: {outcome!r}")
    bootstrap_diagnostics = startup_bootstrap.get("diagnostics")
    if not isinstance(bootstrap_diagnostics, list) or any(
        not isinstance(x, str) or not x.strip() for x in bootstrap_diagnostics
    ):
        die(f"original-{name}.startup_bootstrap.diagnostics must be a string array")
    if any(value not in diagnostics for value in bootstrap_diagnostics):
        die(
            f"original-{name} startup bootstrap diagnostics are not retained "
            "in the sticky UI Automation diagnostics"
        )

    bootstrap_seen = startup_bootstrap["settings_dialog_seen"]
    bootstrap_signature = startup_bootstrap["signature_verified"]
    bootstrap_attempted = startup_bootstrap["attempted"]
    bootstrap_dismissed = startup_bootstrap["dismissed"]

    startup_failure_states = {
        "desktop-root-missing": (False, False, False, None),
        "signature-read-failed": (True, False, False, None),
        "signature-mismatch": (True, False, False, None),
        "invoke-failed": (True, True, True, "invoke-ok"),
        "close-verification-failed": (True, True, True, "invoke-ok"),
        "close-timeout": (True, True, True, "invoke-ok"),
    }
    startup_automation_states = {
        (False, False, False, None),
        (True, False, False, None),
        (True, True, True, "invoke-ok"),
    }

    if outcome == "not-seen":
        if (
            bootstrap_seen
            or bootstrap_signature
            or bootstrap_attempted
            or bootstrap_dismissed
            or action is not None
            or bootstrap_diagnostics
        ):
            die(f"original-{name} startup bootstrap not-seen state is inconsistent")
    elif outcome == "dismissed":
        if not (
            bootstrap_seen
            and bootstrap_signature
            and bootstrap_attempted
            and bootstrap_dismissed
            and action == "invoke-ok"
        ):
            die(f"original-{name} dismissed startup bootstrap state is inconsistent")
        if bootstrap_diagnostics and result != "inconclusive":
            die(
                f"original-{name} dismissed startup bootstrap diagnostics "
                "must force an inconclusive result"
            )
    else:
        if bootstrap_dismissed:
            die(f"original-{name} failed startup bootstrap cannot be marked dismissed")
        startup_state = (
            bootstrap_seen,
            bootstrap_signature,
            bootstrap_attempted,
            action,
        )
        if outcome == "automation-failed":
            if startup_state not in startup_automation_states:
                die(
                    f"original-{name} startup automation-failed state is inconsistent"
                )
        elif startup_state != startup_failure_states.get(outcome):
            die(
                f"original-{name} startup bootstrap state is inconsistent "
                f"for outcome {outcome!r}"
            )
        if not bootstrap_diagnostics:
            die(f"original-{name} failed startup bootstrap lacks a harness diagnostic")
        if result != "inconclusive":
            die(f"original-{name} failed startup bootstrap must force an inconclusive result")

    if not isinstance(environment_bootstrap, dict):
        die(f"original-{name}.environment_bootstrap must be an object")
    directsound_bootstrap = environment_bootstrap.get("directsound")
    if not isinstance(directsound_bootstrap, dict):
        die(f"original-{name}.environment_bootstrap.directsound must be an object")
    for field in ("dialog_seen", "signature_verified", "attempted", "dismissed"):
        if not isinstance(directsound_bootstrap.get(field), bool):
            die(
                f"original-{name}.environment_bootstrap.directsound.{field} "
                "must be boolean"
            )
    directsound_action = directsound_bootstrap.get("action")
    if directsound_action not in {
        None,
        "invoke-ok",
        "invoke-ok-win32-bm-click",
    }:
        die(f"original-{name} DirectSound bootstrap action is invalid")
    directsound_outcome = directsound_bootstrap.get("outcome")
    allowed_directsound_outcomes = {
        "not-seen",
        "dismissed",
        "desktop-root-missing",
        "signature-read-failed",
        "signature-mismatch",
        "invoke-failed",
        "win32-action-failed",
        "close-verification-failed",
        "close-timeout",
        "automation-failed",
    }
    if directsound_outcome not in allowed_directsound_outcomes:
        die(
            f"original-{name} DirectSound bootstrap outcome is invalid: "
            f"{directsound_outcome!r}"
        )
    directsound_diagnostics = directsound_bootstrap.get("diagnostics")
    if not isinstance(directsound_diagnostics, list) or any(
        not isinstance(x, str) or not x.strip() for x in directsound_diagnostics
    ):
        die(f"original-{name} DirectSound bootstrap diagnostics must be a string array")
    if any(value not in diagnostics for value in directsound_diagnostics):
        die(
            f"original-{name} DirectSound bootstrap diagnostics are not retained "
            "in the sticky UI Automation diagnostics"
        )

    directsound_seen = directsound_bootstrap["dialog_seen"]
    directsound_signature = directsound_bootstrap["signature_verified"]
    directsound_attempted = directsound_bootstrap["attempted"]
    directsound_dismissed = directsound_bootstrap["dismissed"]

    directsound_failure_states = {
        "desktop-root-missing": (False, False, False, None),
        "signature-read-failed": (True, False, False, None),
        "signature-mismatch": (True, False, False, None),
        "invoke-failed": (True, True, True, "invoke-ok"),
        "win32-action-failed": (
            True,
            True,
            True,
            "invoke-ok-win32-bm-click",
        ),
        "close-timeout": (
            True,
            True,
            True,
            "invoke-ok-win32-bm-click",
        ),
    }
    directsound_close_verification_states = {
        (True, True, True, "invoke-ok"),
        (True, True, True, "invoke-ok-win32-bm-click"),
    }
    directsound_automation_states = {
        (False, False, False, None),
        (True, False, False, None),
        (True, True, True, "invoke-ok"),
        (True, True, True, "invoke-ok-win32-bm-click"),
    }

    if directsound_outcome == "not-seen":
        if (
            directsound_seen
            or directsound_signature
            or directsound_attempted
            or directsound_dismissed
            or directsound_action is not None
            or directsound_diagnostics
        ):
            die(f"original-{name} DirectSound bootstrap not-seen state is inconsistent")
    elif directsound_outcome == "dismissed":
        if not (
            directsound_seen
            and directsound_signature
            and directsound_attempted
            and directsound_dismissed
            and directsound_action in {
                "invoke-ok",
                "invoke-ok-win32-bm-click",
            }
        ):
            die(f"original-{name} dismissed DirectSound bootstrap state is inconsistent")
        if directsound_diagnostics and result != "inconclusive":
            die(
                f"original-{name} dismissed DirectSound bootstrap diagnostics "
                "must force an inconclusive result"
            )
    else:
        if directsound_dismissed:
            die(f"original-{name} failed DirectSound bootstrap cannot be marked dismissed")
        directsound_state = (
            directsound_seen,
            directsound_signature,
            directsound_attempted,
            directsound_action,
        )
        if directsound_outcome == "close-verification-failed":
            if directsound_state not in directsound_close_verification_states:
                die(
                    f"original-{name} DirectSound close-verification-failed "
                    "state is inconsistent"
                )
        elif directsound_outcome == "automation-failed":
            if directsound_state not in directsound_automation_states:
                die(
                    f"original-{name} DirectSound automation-failed state is inconsistent"
                )
        elif directsound_state != directsound_failure_states.get(directsound_outcome):
            die(
                f"original-{name} DirectSound bootstrap state is inconsistent "
                f"for outcome {directsound_outcome!r}"
            )
        if not directsound_diagnostics:
            die(f"original-{name} failed DirectSound bootstrap lacks a harness diagnostic")
        if result != "inconclusive":
            die(f"original-{name} failed DirectSound bootstrap must force an inconclusive result")

    if not isinstance(runtime_diagnostics, list) or any(
        not isinstance(x, str) or not x.strip() for x in runtime_diagnostics
    ):
        die(f"original-{name}.runtime_identity_diagnostics must be a string array")
    if sorted(set(runtime_diagnostics)) != sorted(set(runtime_inventory_diagnostics)):
        die(
            f"original-{name} runtime identity diagnostics do not match "
            "the hash-bound loaded-runtime inventory"
        )
    if not isinstance(stable_polls, int) or stable_polls < 0:
        die(f"original-{name}.stable_marker_polls must be a non-negative integer")
    if exit_code is not None and (not isinstance(exit_code, int) or isinstance(exit_code, bool)):
        die(f"original-{name}.exit_code_before_termination must be integer or null")
    if not isinstance(running_before_termination, bool):
        die(f"original-{name}.process_running_before_termination must be boolean")
    if termination not in ALLOWED_TERMINATION:
        die(f"original-{name}.termination is invalid: {termination!r}")

    if result == "accepted":
        if not isinstance(marker, str) or not marker.strip():
            die(f"original-{name} accepted result lacks a concrete load evidence marker")
        if error_marker is not None or application_error_marker is not None:
            die(f"original-{name} accepted result also contains application error evidence")
        if error_marker_scope != "none":
            die(f"original-{name} accepted result has a non-empty error scope")
        if diagnostics:
            die(f"original-{name} accepted result contains UI Automation harness diagnostics")
        if bootstrap_seen and not bootstrap_dismissed:
            die(f"original-{name} accepted result did not complete the first-run settings bootstrap")
        if directsound_seen and not directsound_dismissed:
            die(f"original-{name} accepted result did not complete the DirectSound environment bootstrap")
        if runtime_diagnostics or not runtime_identity_valid:
            die(f"original-{name} accepted result lacks verified loaded VC90 runtime identity")
        if stable_polls < 4:
            die(f"original-{name} accepted result lacks the required stable marker window")
        if original.get("main_window_seen") is not True:
            die(f"original-{name} accepted result lacks an observed Psycle-owned window")
        if exit_code is not None:
            die(f"original-{name} accepted result exited before harness termination")
        if running_before_termination is not True:
            die(f"original-{name} accepted result was not running when harness termination began")
        if termination not in ACCEPTED_TERMINATION:
            die(f"original-{name} accepted result lacks successful harness-controlled termination")
    elif result == "rejected":
        if not isinstance(error_marker, str) or not error_marker.strip():
            die(f"original-{name} rejected result lacks concrete fixture-load error evidence")
        if not isinstance(application_error_marker, str) or not application_error_marker.strip():
            die(f"original-{name} rejected result lacks application error evidence")
        if error_marker_scope != "fixture-load":
            die(f"original-{name} rejected result is not scoped to a fixture-load failure")
        if error_marker.startswith("UI_") or "Automation" in error_marker:
            die(f"original-{name} rejection incorrectly uses harness diagnostics as application evidence")
    else:
        if (
            application_error_marker is not None
            and error_marker_scope != "application-unassociated"
        ):
            die(
                f"original-{name} inconclusive application error evidence "
                "is not scoped as application-unassociated"
            )

    if diagnostics and result != "inconclusive":
        die(f"original-{name} UI Automation diagnostics must force an inconclusive result")
    if (runtime_diagnostics or not runtime_identity_valid) and result != "inconclusive":
        die(f"original-{name} loaded VC90 runtime identity diagnostics must force an inconclusive result")

    if original.get("original_psycle_observed") is not True:
        die(f"original-{name} must explicitly record original_psycle_observed=true")
    if original.get("parity_status") != "UNKNOWN":
        die(f"original-{name} observation artifact must not self-promote parity")

    require_artifact_file(original_root, original.get("stdout"), f"original-{name}.stdout")
    require_artifact_file(original_root, original.get("stderr"), f"original-{name}.stderr")
    require_artifact_file(
        original_root, original.get("ui_evidence"), f"original-{name}.ui_evidence"
    )
    require_artifact_file(
        original_root,
        original.get("screenshot"),
        f"original-{name}.screenshot",
        optional=True,
    )
    return result


def main() -> int:
    if len(sys.argv) != 3:
        die("usage: phase6c-validate-original-receipts.py CANDIDATE_ARTIFACT ORIGINAL_ARTIFACT")

    candidate_root = pathlib.Path(sys.argv[1]).resolve()
    original_root = pathlib.Path(sys.argv[2]).resolve()
    if not candidate_root.is_dir():
        die(f"candidate artifact root is missing: {candidate_root}")
    if not original_root.is_dir():
        die(f"original artifact root is missing: {original_root}")

    for path in original_root.rglob("*"):
        if not path.is_file():
            continue
        suffix = path.suffix.lower()
        if suffix not in ALLOWED_ARTIFACT_SUFFIXES:
            die(
                "original evidence artifact contains a prohibited/unexpected file "
                f"type: {path.relative_to(original_root)}"
            )
        if suffix == ".psy":
            relative = path.relative_to(original_root)
            if not relative.parts or relative.parts[0] != "fixtures":
                die(f".psy evidence input is outside fixtures/: {relative}")

    results = {
        name: validate_pair(name, contract, candidate_root, original_root)
        for name, contract in EXPECTED.items()
    }

    print(
        "phase6c-validate-original-receipts: PASS "
        + " ".join(f"{name}={result}" for name, result in results.items())
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
