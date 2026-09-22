param(
    [Parameter(Mandatory = $true)]
    [string]$CandidateArtifactRoot,

    [string]$Out = "phase6c-original-evidence",

    [switch]$ObserveSerialization,

    [switch]$ObserveSequenceOrder,

    [switch]$ObserveBpmLpbTick,

    [switch]$ObserveDelayedRetrigger,

    [switch]$CollectSamplerFaultLocation
)

$ErrorActionPreference = "Stop"

if ($CollectSamplerFaultLocation -and -not $ObserveDelayedRetrigger) {
    throw "phase6c-original-windows-fixtures: Sampler fault-location collection requires ObserveDelayedRetrigger"
}

$baseObserver = Join-Path $PSScriptRoot "phase6c-original-windows-fixtures-v2.ps1"
if ($ObserveDelayedRetrigger) {
    & $baseObserver `
        -CandidateArtifactRoot $CandidateArtifactRoot `
        -Out $Out `
        -ObserveSerialization:$ObserveSerialization `
        -ObserveSequenceOrder:$ObserveSequenceOrder `
        -ObserveDelayedRetrigger `
        -CollectSamplerFaultLocation:$CollectSamplerFaultLocation
} elseif ($ObserveBpmLpbTick) {
    & $baseObserver `
        -CandidateArtifactRoot $CandidateArtifactRoot `
        -Out $Out `
        -ObserveSerialization:$ObserveSerialization `
        -ObserveSequenceOrder:$ObserveSequenceOrder `
        -ObserveBpmLpbTick
} else {
    & $baseObserver `
        -CandidateArtifactRoot $CandidateArtifactRoot `
        -Out $Out `
        -ObserveSerialization:$ObserveSerialization `
        -ObserveSequenceOrder:$ObserveSequenceOrder
}
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$outRoot = if ([System.IO.Path]::IsPathRooted($Out)) {
    [System.IO.Path]::GetFullPath($Out)
} else {
    [System.IO.Path]::GetFullPath((Join-Path (Join-Path $PSScriptRoot "..") $Out))
}

$runtimeUrl = $env:PSYCLE_PHASE6C_VC90_RUNTIME_URL
$runtimeSha = $env:PSYCLE_PHASE6C_VC90_RUNTIME_SHA256
$redistributableVersion = $env:PSYCLE_PHASE6C_VC90_RUNTIME_VERSION
$runtimeReceipt = $env:PSYCLE_PHASE6C_VC90_RUNTIME_RECEIPT

if ([string]::IsNullOrWhiteSpace($runtimeUrl) -or
    [string]::IsNullOrWhiteSpace($runtimeSha) -or
    [string]::IsNullOrWhiteSpace($redistributableVersion) -or
    [string]::IsNullOrWhiteSpace($runtimeReceipt)) {
    throw "phase6c-original-windows-fixtures: missing pinned VC90 redistributable environment metadata"
}

if (-not (Test-Path -LiteralPath $runtimeReceipt -PathType Leaf)) {
    throw "phase6c-original-windows-fixtures: missing VC90 runtime receipt: $runtimeReceipt"
}
Copy-Item -LiteralPath $runtimeReceipt -Destination (Join-Path $outRoot "vc90-runtime.txt")

$receiptNames = @("psy2", "psy3")
if ($ObserveSerialization -and (Test-Path -LiteralPath (Join-Path $outRoot "original-psy3-reopen.json"))) {
    $receiptNames += "psy3-reopen"
}
if ($ObserveSequenceOrder -and (Test-Path -LiteralPath (Join-Path $outRoot "original-sequence-order.json"))) {
    $receiptNames += "sequence-order"
}
if ($ObserveBpmLpbTick -and (Test-Path -LiteralPath (Join-Path $outRoot "original-bpm-lpb-tick.json"))) {
    $receiptNames += "bpm-lpb-tick"
}
if ($ObserveDelayedRetrigger -and (Test-Path -LiteralPath (Join-Path $outRoot "original-delayed-retrigger.json"))) {
    $receiptNames += "delayed-retrigger"
}
if ($ObserveDelayedRetrigger -and (Test-Path -LiteralPath (Join-Path $outRoot "original-delayed-retrigger-execution.json"))) {
    $receiptNames += "delayed-retrigger-execution"
}
if ($ObserveDelayedRetrigger -and (Test-Path -LiteralPath (Join-Path $outRoot "original-delayed-retrigger-sampulse-runtime.json"))) {
    $receiptNames += "delayed-retrigger-sampulse-runtime"
}
if ($ObserveDelayedRetrigger) {
    foreach ($variant in @("control", "fd", "fb", "fa", "fe")) {
        $name = "delayed-retrigger-isolation-$variant"
        if (Test-Path -LiteralPath (Join-Path $outRoot ("original-{0}.json" -f $name))) {
            $receiptNames += $name
        }
    }
}
if ($ObserveDelayedRetrigger) {
    foreach ($variant in @("master-only", "sampler-empty", "sample-state", "ordinary-note")) {
        $name = "delayed-retrigger-substrate-$variant"
        if (Test-Path -LiteralPath (Join-Path $outRoot ("original-{0}.json" -f $name))) {
            $receiptNames += $name
        }
    }
}
if ($ObserveDelayedRetrigger) {
    foreach ($variant in @(
        "note-no-previous-inst",
        "note-missing-sample",
        "note-sample-default-inst",
        "note-sample-serialized-inst"
    )) {
        $name = "sampler-voice-startup-$variant"
        if (Test-Path -LiteralPath (Join-Path $outRoot ("original-{0}.json" -f $name))) {
            $receiptNames += $name
        }
    }
}
if ($ObserveDelayedRetrigger) {
    foreach ($variant in @(
        "release-no-active-voice",
        "delayed-note-short",
        "delayed-note-long",
        "ordinary-note-short"
    )) {
        $name = "sampler-work-boundary-$variant"
        if (Test-Path -LiteralPath (Join-Path $outRoot ("original-{0}.json" -f $name))) {
            $receiptNames += $name
        }
    }
}
if ($CollectSamplerFaultLocation -and
    (Test-Path -LiteralPath (Join-Path $outRoot "original-sampler-fault-location.json"))) {
    $receiptNames += "sampler-fault-location"
}
foreach ($name in $receiptNames) {
    $receiptPath = Join-Path $outRoot ("original-{0}.json" -f $name)
    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    $receipt.procedure = "$($receipt.procedure); before launch install pinned Microsoft Visual C++ 2008 SP1 x86 redistributable version $redistributableVersion from $runtimeUrl with SHA-256 $runtimeSha; loaded VC90 module identity is recorded separately from the live Psycle process"
    $receipt.environment | Add-Member -NotePropertyName vc90_redistributable -NotePropertyValue ([ordered]@{
        redistributable_version = $redistributableVersion
        url = $runtimeUrl
        sha256 = $runtimeSha
        receipt = "vc90-runtime.txt"
    }) -Force
    $json = $receipt | ConvertTo-Json -Depth 12
    [System.IO.File]::WriteAllText(
        $receiptPath,
        $json + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false)
    )
}
