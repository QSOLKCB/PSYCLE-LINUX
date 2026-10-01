#!/usr/bin/env python3
"""Generate a private-input SickMaate observer from the mature Windows observer."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

EXPECTED_BLOB = "4a10fb12805ff10a85cfedc0118443220a7dc32a"
EXPECTED_SHA256 = "cab23d8f66a6815b3248457e4f38de74f0a6d61c2691c47a78e582edb63cd432"
EXPECTED_SIZE = 306462


def git_blob(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"historical IT observer patch anchor {label!r} count={count}, expected=1"
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
            "historical IT observer base blob changed: "
            f"expected={EXPECTED_BLOB} actual={actual}"
        )
    text = normalized.decode("utf-8")

    text = replace_once(
        text,
        "    [switch]$ObserveSequenceOrder\n)",
        "    [switch]$ObserveSequenceOrder,\n\n"
        "    [switch]$ObserveHistoricalLegacyIt\n)",
        "parameter",
    )

    historical_spec = rf'''
    if ($ObserveHistoricalLegacyIt) {{
        $fixtureSpecs = @(
            [ordered]@{{
                name = "historical-legacy-it"
                candidate_receipt = "candidate-historical-legacy-it.json"
                expected_contract = "legacy-impulse-tracker-import-reference"
                expected_song_title = "SickMaate"
                load_warning_required = $false
                expected_load_warning_message = $null
            }}
        )
    }}
'''
    text = replace_once(
        text,
        "\n    foreach ($spec in $fixtureSpecs) {",
        historical_spec + "\n    foreach ($spec in $fixtureSpecs) {",
        "fixture specification",
    )

    copy_old = '''        $fixtureDir = Join-Path $fixtureArtifactRoot $spec.name
        New-Item -ItemType Directory -Path $fixtureDir -Force | Out-Null
        $fixtureCopy = Join-Path $fixtureDir ([System.IO.Path]::GetFileName($candidateFixturePath))
        Copy-Item -LiteralPath $candidateFixturePath -Destination $fixtureCopy
        if ((Get-Sha256 $fixtureCopy) -ne $fixtureSha) {
            Fail "copied evidence fixture SHA-256 mismatch for $($spec.name)"
        }
        $fixtureReceiptPath = "fixtures/$($spec.name)/$([System.IO.Path]::GetFileName($fixtureCopy))"
'''
    copy_new = rf'''        $historicalExternal = (
            $ObserveHistoricalLegacyIt -and
            $spec.name -eq "historical-legacy-it"
        )
        if ($historicalExternal) {{
            if ($fixtureSha -ne "{EXPECTED_SHA256}") {{
                Fail "historical IT SHA-256 differs from the manifest-bound witness"
            }}
            if ((Get-Item -LiteralPath $candidateFixturePath).Length -ne {EXPECTED_SIZE}) {{
                Fail "historical IT size differs from the manifest-bound witness"
            }}
            $fixtureCopy = Join-Path $workRoot ([System.IO.Path]::GetFileName($candidateFixturePath))
            Copy-Item -LiteralPath $candidateFixturePath -Destination $fixtureCopy
            if ((Get-Sha256 $fixtureCopy) -ne $fixtureSha) {{
                Fail "transient historical IT copy SHA-256 mismatch"
            }}
            $fixtureReceiptPath = "external/$([System.IO.Path]::GetFileName($fixtureCopy))"
        }} else {{
            $fixtureDir = Join-Path $fixtureArtifactRoot $spec.name
            New-Item -ItemType Directory -Path $fixtureDir -Force | Out-Null
            $fixtureCopy = Join-Path $fixtureDir ([System.IO.Path]::GetFileName($candidateFixturePath))
            Copy-Item -LiteralPath $candidateFixturePath -Destination $fixtureCopy
            if ((Get-Sha256 $fixtureCopy) -ne $fixtureSha) {{
                Fail "copied evidence fixture SHA-256 mismatch for $($spec.name)"
            }}
            $fixtureReceiptPath = "fixtures/$($spec.name)/$([System.IO.Path]::GetFileName($fixtureCopy))"
        }}
'''
    text = replace_once(text, copy_old, copy_new, "transient historical fixture")

    text = replace_once(
        text,
        '            procedure = if ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        '            procedure = if ($ObserveHistoricalLegacyIt -and $spec.name -eq "historical-legacy-it") {\n'
        '                "$Procedure; observe the manifest-bound external SickMaate IT witness from a transient private copy only; require SHA-256 '
        + EXPECTED_SHA256
        + ' and size '
        + str(EXPECTED_SIZE)
        + '; do not copy historical module bytes into the evidence artifact; require pinned Microsoft Visual C++ 2008 SP1 x86 runtime version 9.0.30729.6161 with the project-bound runtime receipt"\n'
        '            } elseif ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        "historical procedure",
    )

    text = replace_once(
        text,
        "            fixture_sha256 = $fixtureSha\n",
        "            fixture_sha256 = $fixtureSha\n"
        '            fixture_distribution = if ($historicalExternal) { "external-hash-bound" } else { "artifact" }\n'
        "            fixture_redistributed = (-not $historicalExternal)\n",
        "historical distribution receipt",
    )

    vc90_procedure_binding = r'''
$HistoricalItVc90RuntimeUrl = $env:PSYCLE_PHASE6C_VC90_RUNTIME_URL
$HistoricalItVc90RuntimeSha256 = $env:PSYCLE_PHASE6C_VC90_RUNTIME_SHA256
$HistoricalItVc90RuntimeVersion = $env:PSYCLE_PHASE6C_VC90_RUNTIME_VERSION
$HistoricalItVc90RuntimeReceipt = $env:PSYCLE_PHASE6C_VC90_RUNTIME_RECEIPT
foreach ($requiredVc90Value in @(
    $HistoricalItVc90RuntimeUrl,
    $HistoricalItVc90RuntimeSha256,
    $HistoricalItVc90RuntimeVersion,
    $HistoricalItVc90RuntimeReceipt
)) {
    if ([string]::IsNullOrWhiteSpace([string]$requiredVc90Value)) {
        throw "historical IT observer requires the pinned VC90 runtime provenance environment"
    }
}
$Procedure = "$Procedure; pinned VC90 x86 runtime required and bound for replay; vc90_runtime_url=$HistoricalItVc90RuntimeUrl; vc90_runtime_sha256=$HistoricalItVc90RuntimeSha256; vc90_runtime_version=$HistoricalItVc90RuntimeVersion; vc90_runtime_receipt=$HistoricalItVc90RuntimeReceipt"
'''
    text = replace_once(
        text,
        "\nfunction Fail([string]$Message) {",
        vc90_procedure_binding + "\nfunction Fail([string]$Message) {",
        "VC90 procedure binding",
    )

    vc90_environment_binding = r'''
    $HistoricalItVc90RuntimeArtifact = Join-Path $outRoot "vc90-runtime.txt"
    if (-not (Test-Path -LiteralPath $HistoricalItVc90RuntimeReceipt -PathType Leaf)) {
        throw "historical IT observer missing pinned VC90 runtime receipt: $HistoricalItVc90RuntimeReceipt"
    }
    Copy-Item -LiteralPath $HistoricalItVc90RuntimeReceipt -Destination $HistoricalItVc90RuntimeArtifact -Force
    $environment["vc90_redistributable"] = [ordered]@{
        redistributable_version = $HistoricalItVc90RuntimeVersion
        url = $HistoricalItVc90RuntimeUrl
        sha256 = $HistoricalItVc90RuntimeSha256
        receipt = "vc90-runtime.txt"
    }
'''
    text = replace_once(
        text,
        '        installer_framework = $installerFramework\n    }\n\n    $postInstallRegistryExists',
        '        installer_framework = $installerFramework\n    }\n' +
        vc90_environment_binding +
        '\n    $postInstallRegistryExists',
        "VC90 structured environment binding",
    )

    args.output.write_text(text, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
