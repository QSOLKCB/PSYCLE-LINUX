param(
    [Parameter(Mandatory = $true)]
    [string]$CandidateArtifactRoot,

    [string]$Out = "phase6c-original-evidence"
)

$ErrorActionPreference = "Stop"

& (Join-Path $PSScriptRoot "phase6c-original-windows-fixtures-v2.ps1") `
    -CandidateArtifactRoot $CandidateArtifactRoot `
    -Out $Out
exit $LASTEXITCODE
