#!/usr/bin/env python3
"""Create the timing-enabled original observer from the frozen Phase 6C harness.

The mature Windows observer is deliberately patched only in CI. Exact Git blob
and anchor checks prevent this adapter from silently applying to a changed
harness; the generated file is parsed before execution and is never committed.
"""
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
        raise SystemExit(f"timing observer patch anchor {label!r} count={count}, expected=1")
    return text.replace(old, new, 1)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = args.source.read_bytes()
    actual = git_blob(data)
    if actual != EXPECTED_BLOB:
        raise SystemExit(
            f"timing observer base blob changed: expected={EXPECTED_BLOB} actual={actual}"
        )
    text = data.decode("utf-8")

    text = replace_once(
        text,
        "    [switch]$ObserveSequenceOrder\n)",
        "    [switch]$ObserveSequenceOrder,\n\n    [switch]$ObserveBpmLpbTick\n)",
        "parameter",
    )
    text = replace_once(
        text,
        "Add-Type -AssemblyName System.Windows.Forms\nAdd-Type -TypeDefinition @\"",
        "Add-Type -AssemblyName System.Windows.Forms\n. (Join-Path $PSScriptRoot \"phase6c-original-timing-ui.ps1\")\nAdd-Type -TypeDefinition @\"",
        "helper import",
    )
    timing_spec = '''
    if ($ObserveBpmLpbTick) {
        $fixtureSpecs += [ordered]@{
            name = "bpm-lpb-tick"
            candidate_receipt = "candidate-bpm-lpb-tick.json"
            expected_contract = "sequencer-bpm-lpb-tick"
            expected_song_title = "PSYCLE-LINUX Phase 6C timing fixture"
            load_warning_required = $true
            expected_load_warning_message = "This file is from a newer version of Psycle! This process will try to load it anyway."
            expected_timing_values = [ordered]@{
                tempo = 137
                lines_per_beat = 8
                ticks_per_beat = 24
                extra_tick_per_line = 0
                real_tempo = 137
                real_ticks_per_beat = 24
            }
        }
    }
'''
    text = replace_once(
        text,
        "\n    foreach ($spec in $fixtureSpecs) {",
        timing_spec + "\n    foreach ($spec in $fixtureSpecs) {",
        "fixture specification",
    )

    timing_poll = '''
        $timingExpected = $null
        $timingObserved = [ordered]@{}
        $timingFingerprint = $null
        $timingStablePolls = 0
        $timingDialogBootstrap = $null
        if ($null -ne $spec.expected_timing_values) {
            $timingExpected = $spec.expected_timing_values
            $timingDialogBootstrap = Open-Phase6cSongInformationDialog $process
            foreach ($diagnostic in @($timingDialogBootstrap.diagnostics)) {
                if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                    [void]$uiDiagnosticSet.Add([string]$diagnostic)
                }
            }
            $uiDiagnostics = @($uiDiagnosticSet | Sort-Object)
            if ($timingDialogBootstrap.opened -and $uiDiagnostics.Count -eq 0) {
                for ($timingPoll = 0; $timingPoll -lt 4; $timingPoll += 1) {
                    $timingObservation = Get-Phase6cTimingUiObservation $process
                    foreach ($diagnostic in @($timingObservation.diagnostics)) {
                        if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                            [void]$uiDiagnosticSet.Add([string]$diagnostic)
                        }
                    }
                    $uiDiagnostics = @($uiDiagnosticSet | Sort-Object)
                    if ($timingObservation.complete -and $uiDiagnostics.Count -eq 0) {
                        $candidateValues = [ordered]@{}
                        foreach ($timingKey in @($timingExpected.Keys)) {
                            $candidateValues[$timingKey] = [int]$timingObservation.values[$timingKey]
                        }
                        $fingerprint = ($candidateValues | ConvertTo-Json -Compress)
                        if ($null -ne $timingFingerprint -and $fingerprint -ceq $timingFingerprint) {
                            $timingStablePolls += 1
                        } else {
                            $timingFingerprint = $fingerprint
                            $timingStablePolls = 1
                        }
                        $timingObserved = $candidateValues
                    } else {
                        $timingObserved = [ordered]@{}
                        $timingFingerprint = $null
                        $timingStablePolls = 0
                    }
                    if ($timingPoll -lt 3) { Start-Sleep -Milliseconds 250 }
                }
            }
        }
'''
    text = replace_once(
        text,
        "\n        $process.Refresh()\n        $exitCode = $null",
        timing_poll + "\n        $process.Refresh()\n        $exitCode = $null",
        "timing poll",
    )

    timing_result = '''
        $timingUi = $null
        if ($null -ne $timingExpected) {
            $timingResult = "inconclusive"
            $timingMatch = $null
            $timingReceiptValues = [ordered]@{}
            if ($cleanAcceptedLoadEvidence -and $timingStablePolls -ge 4 -and
                $timingObserved.Count -eq $timingExpected.Count) {
                $timingResult = "observed"
                $timingMatch = $true
                foreach ($timingKey in @($timingExpected.Keys)) {
                    $timingReceiptValues[$timingKey] = [int]$timingObserved[$timingKey]
                    if ([int]$timingObserved[$timingKey] -ne [int]$timingExpected[$timingKey]) {
                        $timingMatch = $false
                    }
                }
            }
            $timingUi = [ordered]@{
                expected_values = $timingExpected
                observed_values = $timingReceiptValues
                stable_polls = $timingStablePolls
                result = $timingResult
                matches_fixture_expected = $timingMatch
                dialog_bootstrap = $timingDialogBootstrap
            }
        }
'''
    text = replace_once(
        text,
        "\n        $evidenceLines = [System.Collections.Generic.List[string]]::new()",
        timing_result + "\n        $evidenceLines = [System.Collections.Generic.List[string]]::new()",
        "timing result",
    )
    text = replace_once(
        text,
        "            procedure = if ($ObserveSequenceOrder -and $spec.name -eq \"sequence-order\") {",
        "            procedure = if ($ObserveBpmLpbTick -and $spec.name -eq \"bpm-lpb-tick\") {\n                \"$Procedure; for the sequencer-bpm-lpb-tick contract inspect the exact process-owned Song Information timing controls without editing them, require four identical complete value bindings, and apply the same clean accepted-load, runtime-identity and liveness gates before interpreting timing metadata\"\n            } elseif ($ObserveSequenceOrder -and $spec.name -eq \"sequence-order\") {",
        "procedure",
    )
    text = replace_once(
        text,
        "            sequence_order_ui = $sequenceOrderUi\n            runtime_identity_diagnostics",
        "            sequence_order_ui = $sequenceOrderUi\n            timing_ui = $timingUi\n            runtime_identity_diagnostics",
        "receipt",
    )
    text = replace_once(
        text,
        "- Sequence/order observation: when enabled,",
        "- BPM/LPB/tick observation: when enabled, the harness opens only the unique process-owned File > Song Properties menu item (unless exactly one Song Information window is already open), verifies the resulting Song Information dialog, then reads its timing controls without editing them. Tempo, LPB, TPB, extra tick, real tempo and real TPB must form one complete stable value set for at least four polls and satisfy the same clean load/runtime/liveness gate; ambiguity or automation diagnostics remain inconclusive.\n- Sequence/order observation: when enabled,",
        "summary",
    )

    args.output.write_text(text, encoding="utf-8", newline="\n")
    print(f"phase6c-build-timing-observer: PASS output={args.output}")


if __name__ == "__main__":
    main()
