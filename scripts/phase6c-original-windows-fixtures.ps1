param(
    [Parameter(Mandatory = $true)]
    [string]$CandidateArtifactRoot,

    [string]$Out = "phase6c-original-evidence"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$ReferenceBuild = "Psycle 1.12.0 x86"
$ReferenceFile = "PsycleInstallerx86-1.12.0.exe"
$ReferenceUrl = "https://sourceforge.net/projects/psycle/files/psycle/1.12/$ReferenceFile/download"
$ExpectedInstallerSha256 = "f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769"
$ExpectedInstallerSize = 9322919
$Procedure = "download pinned Psycle 1.12.0 x86 installer; verify PE magic/SHA-256/size; transiently extract with 7-Zip on native Windows; launch extracted psycle.exe with <fixture>; observe process/window/UI text for the fixture identity; capture text/screenshot evidence where available; terminate the process; delete installer and extracted payload before artifact upload"

function Fail([string]$Message) {
    throw "phase6c-original-windows-fixtures: $Message"
}

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Resolve-ChildPath([string]$Root, [string]$RelativePath) {
    $rootFull = [System.IO.Path]::GetFullPath($Root)
    $candidate = [System.IO.Path]::GetFullPath((Join-Path $rootFull $RelativePath))
    $separator = [System.IO.Path]::DirectorySeparatorChar
    $prefix = $rootFull.TrimEnd($separator) + $separator
    if (-not $candidate.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Fail "artifact-relative path escapes root: $RelativePath"
    }
    return $candidate
}

function Get-UiText([System.Diagnostics.Process]$Process) {
    $values = New-Object System.Collections.Generic.List[string]
    try {
        $Process.Refresh()
        if ($Process.HasExited -or $Process.MainWindowHandle -eq 0) {
            return @()
        }
        $root = [System.Windows.Automation.AutomationElement]::FromHandle([IntPtr]$Process.MainWindowHandle)
        if ($null -eq $root) {
            return @()
        }
        $nodes = $root.FindAll(
            [System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition
        )
        foreach ($node in $nodes) {
            try {
                $name = $node.Current.Name
                if (-not [string]::IsNullOrWhiteSpace($name)) {
                    $values.Add($name.Trim())
                }
            }
            catch {
                # A UI Automation element can disappear between enumeration and read.
            }
        }
    }
    catch {
        $values.Add("UI_AUTOMATION_ERROR: $($_.Exception.Message)")
    }
    return @($values | Select-Object -Unique)
}

function Save-DesktopScreenshot([string]$Path) {
    try {
        $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
        if ($bounds.Width -le 0 -or $bounds.Height -le 0) {
            return $false
        }
        $bitmap = New-Object System.Drawing.Bitmap($bounds.Width, $bounds.Height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bitmap.Size)
            $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
        }
        finally {
            $graphics.Dispose()
            $bitmap.Dispose()
        }
        return $true
    }
    catch {
        return $false
    }
}

function Write-JsonUtf8([string]$Path, [object]$Value) {
    $json = $Value | ConvertTo-Json -Depth 12
    [System.IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, (New-Object System.Text.UTF8Encoding($false)))
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$candidateRoot = [System.IO.Path]::GetFullPath($CandidateArtifactRoot)
$outRoot = if ([System.IO.Path]::IsPathRooted($Out)) {
    [System.IO.Path]::GetFullPath($Out)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $repoRoot $Out))
}

if (-not (Test-Path -LiteralPath $candidateRoot -PathType Container)) {
    Fail "missing candidate artifact root: $candidateRoot"
}
if (Test-Path -LiteralPath $outRoot) {
    if ((Get-ChildItem -LiteralPath $outRoot -Force | Select-Object -First 1)) {
        Fail "refusing non-empty output directory: $outRoot"
    }
} else {
    New-Item -ItemType Directory -Path $outRoot | Out-Null
}

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$sevenZip = Join-Path $env:ProgramFiles "7-Zip\7z.exe"
if (-not (Test-Path -LiteralPath $sevenZip -PathType Leaf)) {
    $command = Get-Command 7z.exe -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        Fail "7-Zip is required to inspect/extract the pinned installer"
    }
    $sevenZip = $command.Source
}

$workRoot = Join-Path $env:RUNNER_TEMP ("psycle-phase6c-original-" + [Guid]::NewGuid().ToString("N"))
$extractRoot = Join-Path $workRoot "payload"
$installerPath = Join-Path $workRoot $ReferenceFile
$extractLog = Join-Path $outRoot "installer-extract.log"
New-Item -ItemType Directory -Path $workRoot | Out-Null
New-Item -ItemType Directory -Path $extractRoot | Out-Null

try {
    & curl.exe --fail --location --retry 3 --silent --show-error --output $installerPath $ReferenceUrl
    if ($LASTEXITCODE -ne 0) {
        Fail "SourceForge installer download failed with curl exit $LASTEXITCODE"
    }

    $installerInfo = Get-Item -LiteralPath $installerPath
    $installerSha = Get-Sha256 $installerPath
    if ($installerInfo.Length -ne $ExpectedInstallerSize) {
        Fail "reference installer size mismatch: expected=$ExpectedInstallerSize actual=$($installerInfo.Length)"
    }
    if ($installerSha -ne $ExpectedInstallerSha256) {
        Fail "reference installer SHA-256 mismatch: expected=$ExpectedInstallerSha256 actual=$installerSha"
    }

    $stream = [System.IO.File]::OpenRead($installerPath)
    try {
        $first = $stream.ReadByte()
        $second = $stream.ReadByte()
    }
    finally {
        $stream.Dispose()
    }
    if ($first -ne 0x4d -or $second -ne 0x5a) {
        Fail "reference installer is not a PE executable"
    }

    $sevenZipVersion = (& $sevenZip | Select-Object -First 2) -join " | "
    & $sevenZip x "-o$extractRoot" -y $installerPath *> $extractLog
    if ($LASTEXITCODE -ne 0) {
        Fail "7-Zip extraction failed with exit $LASTEXITCODE"
    }

    $psycleExecutables = @(
        Get-ChildItem -LiteralPath $extractRoot -Recurse -File |
            Where-Object { $_.Name -ieq "psycle.exe" }
    )
    if ($psycleExecutables.Count -ne 1) {
        $paths = ($psycleExecutables | ForEach-Object { $_.FullName }) -join "; "
        Fail "expected exactly one extracted psycle.exe; found=$($psycleExecutables.Count) paths=$paths"
    }
    $psycleExe = $psycleExecutables[0]
    $psycleExeSha = Get-Sha256 $psycleExe.FullName
    $versionInfo = $psycleExe.VersionInfo

    $environment = [ordered]@{
        observation_mode = "native-windows-github-runner-transient-extracted-payload"
        runner_os = $env:RUNNER_OS
        runner_arch = $env:RUNNER_ARCH
        image_os = $env:ImageOS
        image_version = $env:ImageVersion
        os_version = [Environment]::OSVersion.VersionString
        powershell_version = $PSVersionTable.PSVersion.ToString()
        seven_zip = $sevenZipVersion
    }

    $fixtureSpecs = @(
        [ordered]@{
            name = "psy2"
            candidate_receipt = "candidate-psy2.json"
            expected_contract = "project-io-psy2-parse"
            expected_song_title = "QSOL PSY2 compatibility fixture"
        },
        [ordered]@{
            name = "psy3"
            candidate_receipt = "candidate-psy3.json"
            expected_contract = "project-io-psy3-parse"
            expected_song_title = "PSYCLE-LINUX Phase 4 synthetic fixture"
        }
    )

    foreach ($spec in $fixtureSpecs) {
        $candidateReceiptPath = Join-Path $candidateRoot $spec.candidate_receipt
        if (-not (Test-Path -LiteralPath $candidateReceiptPath -PathType Leaf)) {
            Fail "missing candidate receipt: $candidateReceiptPath"
        }
        $candidate = Get-Content -LiteralPath $candidateReceiptPath -Raw | ConvertFrom-Json
        if ($candidate.contract -ne $spec.expected_contract -or $candidate.evidence_role -ne "candidate") {
            Fail "candidate receipt identity mismatch for $($spec.name)"
        }
        if ([string]::IsNullOrWhiteSpace([string]$candidate.fixture)) {
            Fail "candidate receipt has no fixture path for $($spec.name)"
        }
        $fixturePath = Resolve-ChildPath $candidateRoot ([string]$candidate.fixture)
        if (-not (Test-Path -LiteralPath $fixturePath -PathType Leaf)) {
            Fail "candidate fixture is missing: $fixturePath"
        }
        $fixtureSha = Get-Sha256 $fixturePath
        if ($fixtureSha -ne ([string]$candidate.fixture_sha256).ToLowerInvariant()) {
            Fail "fixture SHA-256 does not match candidate receipt for $($spec.name)"
        }

        $stdoutPath = Join-Path $outRoot ("original-{0}.stdout.log" -f $spec.name)
        $stderrPath = Join-Path $outRoot ("original-{0}.stderr.log" -f $spec.name)
        $uiPath = Join-Path $outRoot ("original-{0}.ui.txt" -f $spec.name)
        $screenshotPath = Join-Path $outRoot ("original-{0}.png" -f $spec.name)
        $receiptPath = Join-Path $outRoot ("original-{0}.json" -f $spec.name)

        $isolatedProfile = Join-Path $workRoot ("profile-" + $spec.name)
        $appData = Join-Path $isolatedProfile "AppData\Roaming"
        $localAppData = Join-Path $isolatedProfile "AppData\Local"
        New-Item -ItemType Directory -Path $appData -Force | Out-Null
        New-Item -ItemType Directory -Path $localAppData -Force | Out-Null
        $env:APPDATA = $appData
        $env:LOCALAPPDATA = $localAppData
        $env:HOME = $isolatedProfile

        $argument = '"' + $fixturePath + '"'
        $process = Start-Process -FilePath $psycleExe.FullName `
            -ArgumentList $argument `
            -WorkingDirectory $psycleExe.DirectoryName `
            -RedirectStandardOutput $stdoutPath `
            -RedirectStandardError $stderrPath `
            -PassThru

        $deadline = (Get-Date).AddSeconds(30)
        $mainWindowSeen = $false
        $windowTitle = ""
        $uiValues = @()
        $matchedMarker = $null
        $errorMarker = $null

        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 500
            $process.Refresh()
            if ($process.HasExited) {
                break
            }
            if ($process.MainWindowHandle -ne 0) {
                $mainWindowSeen = $true
                $windowTitle = $process.MainWindowTitle
                $uiValues = @(Get-UiText $process)
                $combined = @($windowTitle) + $uiValues
                $markers = @(
                    [System.IO.Path]::GetFileName($fixturePath),
                    [System.IO.Path]::GetFileNameWithoutExtension($fixturePath),
                    $spec.expected_song_title
                )
                foreach ($marker in $markers) {
                    if ([string]::IsNullOrWhiteSpace($marker)) { continue }
                    if ($combined | Where-Object { $_.IndexOf($marker, [System.StringComparison]::OrdinalIgnoreCase) -ge 0 }) {
                        $matchedMarker = $marker
                        break
                    }
                }
                foreach ($candidateText in $combined) {
                    if ($candidateText -match '(?i)\b(error|failed|unsupported|corrupt|invalid)\b') {
                        $errorMarker = $candidateText
                        break
                    }
                }
                if ($matchedMarker -or $errorMarker) {
                    break
                }
            }
        }

        $process.Refresh()
        $exitCode = $null
        if ($process.HasExited) {
            $exitCode = $process.ExitCode
        }

        $evidenceLines = New-Object System.Collections.Generic.List[string]
        $evidenceLines.Add("reference_build=$ReferenceBuild")
        $evidenceLines.Add("contract=$($spec.expected_contract)")
        $evidenceLines.Add("fixture=$($candidate.fixture)")
        $evidenceLines.Add("fixture_sha256=$fixtureSha")
        $evidenceLines.Add("process_id=$($process.Id)")
        $evidenceLines.Add("main_window_seen=$mainWindowSeen")
        $evidenceLines.Add("main_window_title=$windowTitle")
        $evidenceLines.Add("matched_marker=$matchedMarker")
        $evidenceLines.Add("error_marker=$errorMarker")
        if ($null -ne $exitCode) {
            $evidenceLines.Add("exit_code=$exitCode")
        }
        $evidenceLines.Add("")
        $evidenceLines.Add("UI_TEXT:")
        foreach ($value in $uiValues) {
            $evidenceLines.Add($value)
        }
        [System.IO.File]::WriteAllLines($uiPath, $evidenceLines, (New-Object System.Text.UTF8Encoding($false)))

        $screenshotCaptured = Save-DesktopScreenshot $screenshotPath
        $screenshot = $null
        if ($screenshotCaptured -and (Test-Path -LiteralPath $screenshotPath -PathType Leaf)) {
            $screenshot = [ordered]@{
                path = [System.IO.Path]::GetFileName($screenshotPath)
                sha256 = Get-Sha256 $screenshotPath
            }
        }

        if ($matchedMarker) {
            $loadResult = "accepted"
            $observation = "native-window-evidence-identifies-loaded-fixture"
        } elseif ($errorMarker) {
            $loadResult = "rejected"
            $observation = "native-window-evidence-reports-load-error"
        } elseif ($process.HasExited) {
            $loadResult = "inconclusive"
            $observation = "reference-process-exited-without-fixture-load-marker"
        } elseif ($mainWindowSeen) {
            $loadResult = "inconclusive"
            $observation = "reference-window-opened-without-fixture-load-marker"
        } else {
            $loadResult = "inconclusive"
            $observation = "reference-process-produced-no-observable-main-window"
        }

        $termination = "already-exited"
        if (-not $process.HasExited) {
            try {
                [void]$process.CloseMainWindow()
                if (-not $process.WaitForExit(5000)) {
                    $process.Kill()
                    $process.WaitForExit()
                    $termination = "killed-after-observation"
                } else {
                    $termination = "closed-after-observation"
                }
            }
            catch {
                try {
                    $process.Kill()
                    $process.WaitForExit()
                    $termination = "killed-after-close-error"
                }
                catch {
                    $termination = "termination-error"
                }
            }
        }

        $receipt = [ordered]@{
            schema_version = 1
            phase = "6C"
            scope = "original-observation"
            contract = $spec.expected_contract
            evidence_role = "original"
            reference_build = $ReferenceBuild
            reference_file = $ReferenceFile
            reference_installer_sha256 = $ExpectedInstallerSha256
            reference_installer_size_bytes = $ExpectedInstallerSize
            reference_executable = "transient extracted psycle.exe from pinned installer"
            reference_executable_sha256 = $psycleExeSha
            reference_executable_file_version = $versionInfo.FileVersion
            reference_executable_product_version = $versionInfo.ProductVersion
            fixture = [string]$candidate.fixture
            fixture_sha256 = $fixtureSha
            procedure = $Procedure
            observation = $observation
            load_result = $loadResult
            load_evidence_marker = $matchedMarker
            error_marker = $errorMarker
            main_window_seen = $mainWindowSeen
            main_window_title = $windowTitle
            exit_code_before_termination = $exitCode
            termination = $termination
            environment = $environment
            stdout = [ordered]@{
                path = [System.IO.Path]::GetFileName($stdoutPath)
                sha256 = Get-Sha256 $stdoutPath
            }
            stderr = [ordered]@{
                path = [System.IO.Path]::GetFileName($stderrPath)
                sha256 = Get-Sha256 $stderrPath
            }
            ui_evidence = [ordered]@{
                path = [System.IO.Path]::GetFileName($uiPath)
                sha256 = Get-Sha256 $uiPath
            }
            screenshot = $screenshot
            original_psycle_observed = $true
            parity_status = "UNKNOWN"
            parity_note = "This is version-pinned original-reference observation evidence only. Compatibility remains UNKNOWN until a versioned candidate receipt and comparison verdict are committed."
        }
        Write-JsonUtf8 $receiptPath $receipt

        Write-Host ("phase6c-original-windows-fixtures: {0} load_result={1} observation={2} marker={3}" -f `
            $spec.name, $loadResult, $observation, $matchedMarker)
    }

    $summary = @"
# Phase 6C Original Psycle Native-Windows Evidence

- Reference: `$ReferenceBuild` / `$ReferenceFile`.
- Installer SHA-256: `$ExpectedInstallerSha256`.
- Installer size: `$ExpectedInstallerSize` bytes.
- Observation environment: native GitHub-hosted Windows runner.
- Reference installer and extracted executable payload: transient only; not retained in this evidence directory.
- Inputs: exact fixture paths and SHA-256 identities from the Phase 6C candidate artifact.
- Classification policy: these observations do not change compatibility status by themselves; rows remain UNKNOWN until versioned original + candidate receipts and a comparison verdict are committed.
"@
    [System.IO.File]::WriteAllText((Join-Path $outRoot "summary.md"), $summary, (New-Object System.Text.UTF8Encoding($false)))
}
finally {
    if (Test-Path -LiteralPath $workRoot) {
        Remove-Item -LiteralPath $workRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
