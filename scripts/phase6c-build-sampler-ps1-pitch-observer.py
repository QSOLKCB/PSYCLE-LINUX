#!/usr/bin/env python3
"""Generate a Sampler-PS1-pitch-enabled copy of the mature Windows observer."""
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
            f"Sampler PS1 pitch observer patch anchor {label!r} "
            f"count={count}, expected=1"
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
            "Sampler PS1 pitch observer base blob changed: "
            f"expected={EXPECTED_BLOB} actual={actual}"
        )
    text = normalized.decode("utf-8")

    text = replace_once(
        text,
        "    [switch]$ObserveSequenceOrder\n)",
        "    [switch]$ObserveSequenceOrder,\n\n"
        "    [switch]$ObserveSamplerPs1Pitch\n)",
        "parameter",
    )
    text = replace_once(
        text,
        "Add-Type -AssemblyName System.Windows.Forms\nAdd-Type -TypeDefinition @\"",
        "Add-Type -AssemblyName System.Windows.Forms\n"
        ". (Join-Path $PSScriptRoot \"phase6c-original-audio-render.ps1\")\n"
        "Add-Type -TypeDefinition @\"",
        "offline render helper",
    )

    pitch_spec = r'''
    if ($ObserveSamplerPs1Pitch) {
        # Pitch mode is intentionally one-fixture-only. Do not make this
        # artifact depend on unrelated parse/serialization candidate receipts.
        $fixtureSpecs = @(
            [ordered]@{
                name = "sampler-ps1-pitch"
                candidate_receipt = "candidate-sampler-ps1-pitch.json"
                expected_contract = "sampler-ps1-pitch-runtime"
                expected_song_title = "PSYCLE-LINUX Phase 6C Sampler PS1 22.05 kHz pitch witness"
                load_warning_required = $true
                expected_load_warning_message = "This file is from a newer version of Psycle! This process will try to load it anyway."
            }
        )
    }
'''
    text = replace_once(
        text,
        "\n    foreach ($spec in $fixtureSpecs) {",
        pitch_spec + "\n    foreach ($spec in $fixtureSpecs) {",
        "fixture specification",
    )

    runtime_block = r'''
        $runtimeExecution = $null
        if ($ObserveSamplerPs1Pitch -and $spec.name -eq "sampler-ps1-pitch") {
            $renderDirectory = Join-Path $outRoot "sampler-ps1-pitch"
            if (-not (Test-Path -LiteralPath $renderDirectory)) {
                New-Item -ItemType Directory -Path $renderDirectory | Out-Null
            }
            $preRenderLoad = [ordered]@{
                schema_version = 1
                clean_accepted_load = [bool]$cleanAcceptedLoadEvidence
                stable_marker_polls = $stableMarkerPolls
                matched_marker = $matchedMarker
                load_warning_dismissed = [bool]$loadWarningBootstrap.dismissed
                process_running_before_render = [bool]$processRunningBeforeTermination
            }
            $renderSettings = [ordered]@{
                sample_rate = 44100
                bits_per_sample = 16
                channels = "mono-mix"
                dither = $false
                range = "entire-song"
            }
            $attempts = @()
            $renderBindings = @()
            $deterministic = $false
            $runtimeOutcome = "inconclusive"

            if ($cleanAcceptedLoadEvidence -and [bool]$loadWarningBootstrap.dismissed) {
                $renderOnePath = Join-Path $renderDirectory "original-sampler-ps1-pitch-1.wav"
                $renderOne = Invoke-Phase6cAudioRender $process $windowTitle $renderOnePath
                $attempts += $renderOne
                if ($renderOne.outcome -eq "rendered" -and $null -ne $renderOne.output) {
                    $renderBindings += [ordered]@{
                        path = "sampler-ps1-pitch/$($renderOne.output.path)"
                        sha256 = [string]$renderOne.output.sha256
                    }
                    if (-not [bool]$renderOne.process_exited -and [bool]$renderOne.dialog_closed) {
                        $renderTwoPath = Join-Path $renderDirectory "original-sampler-ps1-pitch-2.wav"
                        $renderTwo = Invoke-Phase6cAudioRender $process $windowTitle $renderTwoPath
                        $attempts += $renderTwo
                        if ($renderTwo.outcome -eq "rendered" -and $null -ne $renderTwo.output) {
                            $renderBindings += [ordered]@{
                                path = "sampler-ps1-pitch/$($renderTwo.output.path)"
                                sha256 = [string]$renderTwo.output.sha256
                            }
                        }
                    }
                }

                if ($attempts.Count -gt 0) {
                    $lastAttempt = $attempts[-1]
                    $deterministic = (
                        $renderBindings.Count -eq 2 -and
                        $attempts.Count -eq 2 -and
                        [string]$renderBindings[0].sha256 -ceq [string]$renderBindings[1].sha256 -and
                        -not [bool]$lastAttempt.process_exited
                    )
                    if ($deterministic) {
                        $runtimeOutcome = "rendered-twice"
                    } elseif ([bool]$lastAttempt.process_exited -and
                        $null -ne $lastAttempt.process_exit_code) {
                        $runtimeOutcome = "reference-process-exited-during-render"
                    }
                }
            }

            $runtimeExecution = [ordered]@{
                schema_version = 1
                outcome = $runtimeOutcome
                deterministic = [bool]$deterministic
                parity_status = "UNKNOWN"
                pre_render_load = $preRenderLoad
                settings = $renderSettings
                attempts = @($attempts)
                renders = @($renderBindings)
                interpretation_boundary = if ($runtimeOutcome -eq "rendered-twice") {
                    "two byte-identical source-pinned 44.1-kHz mono renders of the exact 22.05-kHz authored PS1 witness are retained for duration analysis; the broad Sampler PS1 contract remains UNKNOWN"
                } else {
                    "the exact witness did not yield two deterministic completed original renders; retain the load/render/process evidence and keep scoped pitch UNKNOWN"
                }
            }
        }

'''
    text = replace_once(
        text,
        "        $evidenceLines = [System.Collections.Generic.List[string]]::new()",
        runtime_block + "        $evidenceLines = [System.Collections.Generic.List[string]]::new()",
        "runtime pitch observation",
    )

    text = replace_once(
        text,
        '            procedure = if ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        '            procedure = if ($ObserveSamplerPs1Pitch -and $spec.name -eq "sampler-ps1-pitch") {\n'
        '                "$Procedure; after the clean accepted-load gate, invoke only the source-pinned Psycle 1.12.0 Render as Wav File command and exact dialog controls; render the exact hybrid PS1 witness twice as mono 44.1 kHz 16-bit PCM with dither disabled; preserve either deterministic completed output or exact process-exit/inconclusive evidence; do not infer pitch or broad Sampler parity from source alone"\n'
        '            } elseif ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        "procedure",
    )

    text = replace_once(
        text,
        "            sequence_order_ui = $sequenceOrderUi\n            runtime_identity_diagnostics = @($runtimeIdentityDiagnostics)",
        "            sequence_order_ui = $sequenceOrderUi\n"
        "            runtime_execution = $runtimeExecution\n"
        "            runtime_identity_diagnostics = @($runtimeIdentityDiagnostics)",
        "runtime receipt",
    )

    args.output.write_text(text, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
