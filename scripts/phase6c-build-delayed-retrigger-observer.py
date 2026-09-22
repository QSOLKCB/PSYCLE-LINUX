#!/usr/bin/env python3
"""Generate a delayed/retrigger-enabled copy of the mature Windows observer."""
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
            f"delayed/retrigger observer patch anchor {label!r} "
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
            f"delayed/retrigger observer base blob changed: "
            f"expected={EXPECTED_BLOB} actual={actual}"
        )
    text = normalized.decode("utf-8")

    text = replace_once(
        text,
        "    [switch]$ObserveSequenceOrder\n)",
        "    [switch]$ObserveSequenceOrder,\n\n"
        "    [switch]$ObserveDelayedRetrigger\n)",
        "parameter",
    )

    delayed_spec = r'''
    if ($ObserveDelayedRetrigger) {
        $fixtureSpecs += [ordered]@{
            name = "delayed-retrigger"
            candidate_receipt = "candidate-delayed-retrigger.json"
            expected_contract = "sequencer-delayed-retrigger"
            expected_song_title = "PSYCLE-LINUX Phase 6C delayed/retrigger fixture"
            load_warning_required = $true
            expected_load_warning_message = "This file is from a newer version of Psycle! This process will try to load it anyway."
        }
        $fixtureSpecs += [ordered]@{
            name = "delayed-retrigger-execution"
            candidate_receipt = "candidate-delayed-retrigger-execution.json"
            expected_contract = "sequencer-delayed-retrigger-execution-observer"
            expected_song_title = "PSYCLE-LINUX Phase 6C delayed/retrigger execution witness"
            load_warning_required = $true
            expected_load_warning_message = "This file is from a newer version of Psycle! This process will try to load it anyway."
        }
        foreach ($isolationVariant in @("control", "fd", "fb", "fa", "fe")) {
            $variantLabel = if ($isolationVariant -eq "control") {
                "control"
            } else {
                $isolationVariant.ToUpperInvariant()
            }
            $fixtureSpecs += [ordered]@{
                name = "delayed-retrigger-isolation-$isolationVariant"
                candidate_receipt = "candidate-delayed-retrigger-isolation-$isolationVariant.json"
                expected_contract = "sequencer-delayed-retrigger-render-isolation"
                expected_song_title = "PSYCLE-LINUX Phase 6C delayed/retrigger render isolation $variantLabel"
                load_warning_required = $true
                expected_load_warning_message = "This file is from a newer version of Psycle! This process will try to load it anyway."
            }
        }
    }
'''
    text = replace_once(
        text,
        "Add-Type -AssemblyName System.Windows.Forms\nAdd-Type -TypeDefinition @\"",
        "Add-Type -AssemblyName System.Windows.Forms\n"
        ". (Join-Path $PSScriptRoot \"phase6c-original-audio-render.ps1\")\n"
        "Add-Type -TypeDefinition @\"",
        "offline render helper",
    )

    text = replace_once(
        text,
        "\n    foreach ($spec in $fixtureSpecs) {",
        delayed_spec + "\n    foreach ($spec in $fixtureSpecs) {",
        "fixture specification",
    )

    runtime_block = r'''
        $runtimeExecution = $null
        if ($ObserveDelayedRetrigger -and $spec.name -eq "delayed-retrigger-execution") {
            $renderDirectory = Join-Path $outRoot "delayed-retrigger-execution"
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
            if ($cleanAcceptedLoadEvidence -and [bool]$loadWarningBootstrap.dismissed) {
                $renderOnePath = Join-Path $renderDirectory "original-delayed-retrigger-execution-1.wav"
                $renderTwoPath = Join-Path $renderDirectory "original-delayed-retrigger-execution-2.wav"
                $renderOne = Invoke-Phase6cAudioRender $process $windowTitle $renderOnePath
                $attempts = @($renderOne)
                $renderBindings = @()
                if ($renderOne.outcome -eq "rendered" -and $null -ne $renderOne.output) {
                    $renderBindings += [ordered]@{
                        path = "delayed-retrigger-execution/$($renderOne.output.path)"
                        sha256 = [string]$renderOne.output.sha256
                    }
                    $process.Refresh()
                    if (-not $process.HasExited) {
                        $renderTwo = Invoke-Phase6cAudioRender $process $windowTitle $renderTwoPath
                        $attempts += $renderTwo
                        if ($renderTwo.outcome -eq "rendered" -and $null -ne $renderTwo.output) {
                            $renderBindings += [ordered]@{
                                path = "delayed-retrigger-execution/$($renderTwo.output.path)"
                                sha256 = [string]$renderTwo.output.sha256
                            }
                        }
                    }
                }
                $deterministic = (
                    $renderBindings.Count -eq 2 -and
                    [string]$renderBindings[0].sha256 -ceq [string]$renderBindings[1].sha256
                )
                $lastAttempt = $attempts[-1]
                $runtimeOutcome = if ($deterministic) {
                    "rendered-twice"
                } elseif ([bool]$lastAttempt.save_invoked -and [bool]$lastAttempt.process_exited) {
                    "reference-process-exited-during-render"
                } else {
                    "inconclusive"
                }
                $runtimeExecution = [ordered]@{
                    schema_version = 1
                    outcome = $runtimeOutcome
                    deterministic = $deterministic
                    pre_render_load = $preRenderLoad
                    settings = $renderSettings
                    renders = @($renderBindings)
                    attempts = @($attempts)
                }
            } else {
                $runtimeExecution = [ordered]@{
                    schema_version = 1
                    outcome = "inconclusive"
                    deterministic = $false
                    pre_render_load = $preRenderLoad
                    settings = $renderSettings
                    renders = @()
                    attempts = @()
                    diagnostics = @("clean accepted load and dismissed Load Warning are required before runtime execution observation")
                }
            }
        }

        if ($ObserveDelayedRetrigger -and
            [string]$spec.name -like "delayed-retrigger-isolation-*") {
            $renderDirectory = Join-Path $outRoot ([string]$spec.name)
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
            if ($cleanAcceptedLoadEvidence -and [bool]$loadWarningBootstrap.dismissed) {
                $renderName = "original-$($spec.name)-1.wav"
                $renderPath = Join-Path $renderDirectory $renderName
                $renderOne = Invoke-Phase6cAudioRender $process $windowTitle $renderPath
                $attempts = @($renderOne)
                $renderBindings = @()
                if ($renderOne.outcome -eq "rendered" -and $null -ne $renderOne.output) {
                    $renderBindings += [ordered]@{
                        path = "$($spec.name)/$($renderOne.output.path)"
                        sha256 = [string]$renderOne.output.sha256
                    }
                }
                $runtimeOutcome = if ($renderBindings.Count -eq 1) {
                    "rendered-once"
                } elseif ([bool]$renderOne.save_invoked -and [bool]$renderOne.process_exited) {
                    "reference-process-exited-during-render"
                } else {
                    "inconclusive"
                }
                $runtimeExecution = [ordered]@{
                    schema_version = 1
                    outcome = $runtimeOutcome
                    deterministic = $false
                    pre_render_load = $preRenderLoad
                    settings = $renderSettings
                    renders = @($renderBindings)
                    attempts = @($attempts)
                }
            } else {
                $runtimeExecution = [ordered]@{
                    schema_version = 1
                    outcome = "inconclusive"
                    deterministic = $false
                    pre_render_load = $preRenderLoad
                    settings = $renderSettings
                    renders = @()
                    attempts = @()
                    diagnostics = @("clean accepted load and dismissed Load Warning are required before render isolation")
                }
            }
        }

'''
    text = replace_once(
        text,
        "        $evidenceLines = [System.Collections.Generic.List[string]]::new()",
        runtime_block + "        $evidenceLines = [System.Collections.Generic.List[string]]::new()",
        "runtime execution observation",
    )

    text = replace_once(
        text,
        '            procedure = if ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        '            procedure = if ($ObserveDelayedRetrigger -and $spec.name -eq "delayed-retrigger-execution") {\n'
        '                "$Procedure; after the clean accepted-load gate, invoke only the source-pinned Psycle 1.12.0 Render as Wav File command and exact dialog controls; attempt the additive sampled command witness as mono 44.1 kHz 16-bit PCM with dither disabled; if the first render succeeds, repeat it for deterministic waveform validation; if either attempted render exits the reference process, retain the exact process-exit/output evidence without promoting runtime command execution or parity"\n'
        '            } elseif ($ObserveDelayedRetrigger -and $spec.name -eq "delayed-retrigger") {\n'
        '                "$Procedure; for the sequencer-delayed-retrigger contract load the exact frozen command fixture under pinned Psycle 1.12.0 x86 and retain the clean accepted-load/liveness evidence; command execution semantics are recorded separately from the pinned original source and are not inferred from this UI load receipt"\n'
        '            } elseif ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        "procedure",
    )

    text = replace_once(
        text,
        "            sequence_order_ui = $sequenceOrderUi\n            runtime_identity_diagnostics = @($runtimeIdentityDiagnostics)",
        "            sequence_order_ui = $sequenceOrderUi\n"
        "            runtime_execution = $runtimeExecution\n"
        "            runtime_identity_diagnostics = @($runtimeIdentityDiagnostics)",
        "runtime execution receipt",
    )

    args.output.write_text(text, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
