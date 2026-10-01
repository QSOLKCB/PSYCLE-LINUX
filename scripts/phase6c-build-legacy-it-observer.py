#!/usr/bin/env python3
"""Generate a legacy-IT-enabled copy of the mature Windows original observer."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

EXPECTED_BLOB = "90df1d48db76396a0a6a580efa426643cd612eb9"


def git_blob(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"legacy IT observer patch anchor {label!r} count={count}, expected=1"
        )
    return text.replace(old, new, 1)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    data = args.source.read_bytes()
    normalized = data.replace(b"\r\n", b"\n")
    if b"\r" in normalized:
        raise SystemExit("observer base contains unsupported carriage returns")
    actual = git_blob(normalized)
    if actual != EXPECTED_BLOB:
        raise SystemExit(
            "legacy IT observer base blob changed: "
            f"expected={EXPECTED_BLOB} actual={actual}"
        )
    text = normalized.decode("utf-8")

    text = replace_once(
        text,
        "    [switch]$ObserveSequenceOrder\n)",
        "    [switch]$ObserveSequenceOrder,\n\n"
        "    [switch]$ObserveLegacyIt\n)",
        "parameter",
    )

    legacy_spec = r'''
    if ($ObserveLegacyIt) {
        # Legacy-IT mode is intentionally one-fixture-only. The generated
        # fixture is an evidence input; no candidate parity claim is implied.
        $fixtureSpecs = @(
            [ordered]@{
                name = "legacy-it"
                candidate_receipt = "candidate-legacy-it.json"
                expected_contract = "legacy-it-import-reference"
                expected_song_title = "PSYCLE IT import witness"
                load_warning_required = $false
                expected_load_warning_message = $null
            }
        )
    }
'''
    text = replace_once(
        text,
        "\n    foreach ($spec in $fixtureSpecs) {",
        legacy_spec + "\n    foreach ($spec in $fixtureSpecs) {",
        "fixture specification",
    )

    text = replace_once(
        text,
        '            procedure = if ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        '            procedure = if ($ObserveLegacyIt -and $spec.name -eq "legacy-it") {\n'
        '                "$Procedure; require pinned Microsoft Visual C++ 2008 SP1 x86 runtime version 9.0.30729.6161 from the project-bound Microsoft URL with SHA-256 8742bcbf24ef328a72d2a27b693cc7071e38d3bb4b9b44dec42aa3d2c8d61d92; bind the live loaded VC90 module inventory and runtime receipt before accepting the legacy IT observation"\n'
        '            } elseif ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        "procedure runtime binding",
    )

    vc90_procedure_binding = r'''
$LegacyItVc90RuntimeUrl = $env:PSYCLE_PHASE6C_VC90_RUNTIME_URL
$LegacyItVc90RuntimeSha256 = $env:PSYCLE_PHASE6C_VC90_RUNTIME_SHA256
$LegacyItVc90RuntimeVersion = $env:PSYCLE_PHASE6C_VC90_RUNTIME_VERSION
$LegacyItVc90RuntimeReceipt = $env:PSYCLE_PHASE6C_VC90_RUNTIME_RECEIPT
foreach ($requiredVc90Value in @(
    $LegacyItVc90RuntimeUrl,
    $LegacyItVc90RuntimeSha256,
    $LegacyItVc90RuntimeVersion,
    $LegacyItVc90RuntimeReceipt
)) {
    if ([string]::IsNullOrWhiteSpace([string]$requiredVc90Value)) {
        throw "legacy IT observer requires the pinned VC90 runtime provenance environment"
    }
}
$Procedure = "$Procedure; pinned VC90 x86 runtime required and bound for replay; vc90_runtime_url=$LegacyItVc90RuntimeUrl; vc90_runtime_sha256=$LegacyItVc90RuntimeSha256; vc90_runtime_version=$LegacyItVc90RuntimeVersion; vc90_runtime_receipt=$LegacyItVc90RuntimeReceipt"
'''
    text = replace_once(
        text,
        "\nfunction Fail([string]$Message) {",
        vc90_procedure_binding + "\nfunction Fail([string]$Message) {",
        "VC90 procedure binding",
    )

    args.output.write_text(text, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
