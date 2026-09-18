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
$Procedure = "download pinned Psycle 1.12.0 x86 installer; verify PE magic/SHA-256/size; require a clean pre-install HKCU\\Software\\Psycle state; detect a supported installer framework and perform only its documented unattended install into runner-temp; snapshot the post-install Psycle registry baseline and restore it before each fixture; copy the exact project-authored fixture bytes into the evidence artifact; launch installed psycle.exe with the copied fixture; enumerate every top-level window owned by the reference process and its descendants; inventory the complete installed payload, runtime Psycle registry state, configured plugin/machine paths, and conventional external VST/Psycle roots for that observation; reject only on concrete application error text; accept only after a stable fixture marker is observed with no application error or UI Automation failure and while the process is still running when harness termination begins; capture logs/UI/screenshot evidence; terminate the process; delete installer, registry snapshot, and installed payload before artifact upload"

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

function Get-InstallerFramework([string]$Path) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $ascii = [System.Text.Encoding]::ASCII.GetString($bytes)
    $utf16 = [System.Text.Encoding]::Unicode.GetString($bytes)

    if ($ascii.IndexOf("Inno Setup Setup Data", [System.StringComparison]::OrdinalIgnoreCase) -ge 0 -or
        $utf16.IndexOf("Inno Setup Setup Data", [System.StringComparison]::OrdinalIgnoreCase) -ge 0 -or
        $ascii.IndexOf("Inno Setup", [System.StringComparison]::OrdinalIgnoreCase) -ge 0 -or
        $utf16.IndexOf("Inno Setup", [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
        return "inno-setup"
    }

    if ($ascii.IndexOf("Nullsoft Install System", [System.StringComparison]::OrdinalIgnoreCase) -ge 0 -or
        $utf16.IndexOf("Nullsoft Install System", [System.StringComparison]::OrdinalIgnoreCase) -ge 0 -or
        $ascii.IndexOf("Nullsoft", [System.StringComparison]::OrdinalIgnoreCase) -ge 0 -or
        $utf16.IndexOf("Nullsoft", [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
        return "nsis"
    }

    return "unknown"
}

function Get-UiObservation([System.Diagnostics.Process]$Process) {
    $values = New-Object System.Collections.Generic.List[string]
    $diagnostics = New-Object System.Collections.Generic.List[string]
    $topLevelWindowCount = 0

    try {
        $Process.Refresh()
        if ($Process.HasExited) {
            return [ordered]@{
                values = @()
                diagnostics = @()
                top_level_window_count = 0
            }
        }

        $desktop = [System.Windows.Automation.AutomationElement]::RootElement
        if ($null -eq $desktop) {
            return [ordered]@{
                values = @()
                diagnostics = @("UI Automation returned no desktop root element")
                top_level_window_count = 0
            }
        }

        $windows = $desktop.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            [System.Windows.Automation.Condition]::TrueCondition
        )
        foreach ($window in $windows) {
            try {
                $ownerProcessId = $window.Current.ProcessId
            }
            catch {
                continue
            }
            if ($ownerProcessId -ne $Process.Id) {
                continue
            }

            $topLevelWindowCount += 1
            try {
                $windowName = $window.Current.Name
                if (-not [string]::IsNullOrWhiteSpace($windowName)) {
                    $values.Add($windowName.Trim())
                }
            }
            catch {
                $diagnostics.Add("Psycle top-level window read failed: $($_.Exception.Message)")
            }

            try {
                $nodes = $window.FindAll(
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
                        $diagnostics.Add("Psycle UI element read failed: $($_.Exception.Message)")
                    }
                }
            }
            catch {
                $diagnostics.Add("Psycle top-level window traversal failed: $($_.Exception.Message)")
            }
        }
    }
    catch {
        $diagnostics.Add("UI Automation failed while enumerating Psycle top-level windows: $($_.Exception.Message)")
    }

    return [ordered]@{
        values = @($values | Select-Object -Unique)
        diagnostics = @($diagnostics | Select-Object -Unique)
        top_level_window_count = $topLevelWindowCount
    }
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
    [System.IO.File]::WriteAllText(
        $Path,
        $json + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false)
    )
}

function Write-MachinePluginInventory(
    [string]$InstallRoot,
    [string]$ExecutablePath,
    [string]$Path,
    [bool]$PreexistingPsycleRegistry
) {
    $installFull = [System.IO.Path]::GetFullPath($InstallRoot)
    $executableFull = [System.IO.Path]::GetFullPath($ExecutablePath)
    $installPrefix = $installFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar

    $installedFiles = @(
        Get-ChildItem -LiteralPath $installFull -Recurse -File |
            Sort-Object FullName |
            ForEach-Object {
                [ordered]@{
                    path = [System.IO.Path]::GetRelativePath($installFull, $_.FullName).Replace("\", "/")
                    size_bytes = [long]$_.Length
                    sha256 = Get-Sha256 $_.FullName
                }
            }
    )

    $registryEntries = New-Object System.Collections.Generic.List[object]
    $registryRoot = "HKCU:\Software\Psycle"
    if (Test-Path -LiteralPath $registryRoot) {
        $registryKeys = @((Get-Item -LiteralPath $registryRoot))
        $registryKeys += @(Get-ChildItem -LiteralPath $registryRoot -Recurse -ErrorAction SilentlyContinue)
        foreach ($key in $registryKeys) {
            try {
                $properties = Get-ItemProperty -LiteralPath $key.PSPath -ErrorAction Stop
                foreach ($property in $properties.PSObject.Properties) {
                    if ($property.Name -match "^PS(Path|ParentPath|ChildName|Drive|Provider)$") {
                        continue
                    }
                    $rawValue = $property.Value
                    $textValue = if ($null -eq $rawValue) {
                        ""
                    } elseif ($rawValue -is [System.Array]) {
                        (@($rawValue | ForEach-Object { [string]$_ }) -join ";")
                    } else {
                        [string]$rawValue
                    }
                    $registryEntries.Add([ordered]@{
                        key = [string]$key.Name
                        name = [string]$property.Name
                        value = $textValue
                    })
                }
            }
            catch {
                Fail "could not inventory Psycle registry key $($key.Name): $($_.Exception.Message)"
            }
        }
    }

    $pluginRootSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $registryEntries) {
        $searchable = "$($entry.key) $($entry.name) $($entry.value)"
        if ($searchable -notmatch "(?i)(plugin|vst|machine)") {
            continue
        }
        foreach ($piece in ([string]$entry.value -split ";")) {
            $candidate = [Environment]::ExpandEnvironmentVariables($piece.Trim().Trim('"'))
            if ([string]::IsNullOrWhiteSpace($candidate)) {
                continue
            }
            try {
                if ([System.IO.Path]::IsPathRooted($candidate)) {
                    $candidate = [System.IO.Path]::GetFullPath($candidate)
                } elseif ($candidate -match "[\\/]") {
                    $candidate = [System.IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $executableFull) $candidate))
                } else {
                    continue
                }
                [void]$pluginRootSet.Add($candidate)
            }
            catch {
                Fail "invalid plugin/machine path in Psycle registry inventory: $candidate"
            }
        }
    }

    $programFilesX86 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
    $knownRoots = @(
        (Join-Path $installFull "PsyclePlugins"),
        (Join-Path $installFull "VstPlugins"),
        (Join-Path $installFull "Vst64Plugins"),
        (Join-Path $env:USERPROFILE "PsyclePlugins"),
        (Join-Path $env:USERPROFILE "VstPlugins"),
        (Join-Path $env:USERPROFILE "Vst64Plugins"),
        (Join-Path $env:USERPROFILE "Documents\Psycle\PsyclePlugins"),
        (Join-Path $env:USERPROFILE "Documents\Psycle\VstPlugins"),
        (Join-Path $env:USERPROFILE "Documents\Psycle\Vst64Plugins"),
        (Join-Path $env:ProgramFiles "Steinberg\VstPlugins"),
        (Join-Path $env:ProgramFiles "Common Files\VST2")
    )
    if (-not [string]::IsNullOrWhiteSpace($programFilesX86)) {
        $knownRoots += (Join-Path $programFilesX86 "Steinberg\VstPlugins")
        $knownRoots += (Join-Path $programFilesX86 "Common Files\VST2")
    }
    foreach ($root in $knownRoots) {
        if (-not [string]::IsNullOrWhiteSpace([string]$root)) {
            [void]$pluginRootSet.Add([System.IO.Path]::GetFullPath([string]$root))
        }
    }

    $pluginRoots = New-Object System.Collections.Generic.List[object]
    $externalPluginDllCount = 0
    foreach ($root in @($pluginRootSet | Sort-Object)) {
        $rootFull = [System.IO.Path]::GetFullPath([string]$root)
        $insideInstall = $rootFull.Equals($installFull, [System.StringComparison]::OrdinalIgnoreCase) -or
            $rootFull.StartsWith($installPrefix, [System.StringComparison]::OrdinalIgnoreCase)
        $exists = Test-Path -LiteralPath $rootFull -PathType Container
        $dlls = @()
        if ($exists) {
            $dlls = @(
                Get-ChildItem -LiteralPath $rootFull -Recurse -File -ErrorAction SilentlyContinue |
                    Where-Object { $_.Extension -ieq ".dll" } |
                    Sort-Object FullName |
                    ForEach-Object {
                        [ordered]@{
                            path = [System.IO.Path]::GetRelativePath($rootFull, $_.FullName).Replace("\", "/")
                            size_bytes = [long]$_.Length
                            sha256 = Get-Sha256 $_.FullName
                        }
                    }
            )
        }
        if (-not $insideInstall) {
            $externalPluginDllCount += $dlls.Count
        }
        $pluginRoots.Add([ordered]@{
            path = $rootFull
            scope = if ($insideInstall) { "installed-payload" } else { "external" }
            exists = [bool]$exists
            dlls = @($dlls)
        })
    }

    $inventory = [ordered]@{
        schema_version = 1
        reference_build = $ReferenceBuild
        reference_executable_sha256 = Get-Sha256 $executableFull
        preexisting_psycle_registry = $PreexistingPsycleRegistry
        installed_payload_files = @($installedFiles)
        psycle_registry = @($registryEntries)
        plugin_roots = @($pluginRoots)
        external_plugin_dll_count = $externalPluginDllCount
        configuration_baseline = "post-installer HKCU\\Software\\Psycle snapshot restored before this fixture"
        external_visibility_note = "Fresh runner required no pre-existing HKCU\\Software\\Psycle configuration. Installed payload, runtime Psycle registry state, configured plugin/machine paths, and conventional VST/Psycle plugin roots are SHA-256 inventoried during this observation."
    }
    Write-JsonUtf8 $Path $inventory
    return [ordered]@{
        path = [System.IO.Path]::GetFileName($Path)
        sha256 = Get-Sha256 $Path
    }
}

function Write-InstallerDiagnostics([string]$FrameworkPath, [string]$InstallPath) {
    if (Test-Path -LiteralPath $FrameworkPath -PathType Leaf) {
        Write-Host "--- installer-framework.txt ---"
        Get-Content -LiteralPath $FrameworkPath | ForEach-Object { Write-Host $_ }
    }
    if (Test-Path -LiteralPath $InstallPath -PathType Leaf) {
        Write-Host "--- installer-install.log ---"
        Get-Content -LiteralPath $InstallPath | ForEach-Object { Write-Host $_ }
    }
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

$workRoot = Join-Path $env:RUNNER_TEMP ("psycle-phase6c-original-" + [Guid]::NewGuid().ToString("N"))
$installRoot = Join-Path $workRoot "installed"
$installerPath = Join-Path $workRoot $ReferenceFile
$frameworkLog = Join-Path $outRoot "installer-framework.txt"
$installLog = Join-Path $outRoot "installer-install.log"
$fixtureArtifactRoot = Join-Path $outRoot "fixtures"
New-Item -ItemType Directory -Path $workRoot | Out-Null
New-Item -ItemType Directory -Path $installRoot | Out-Null
New-Item -ItemType Directory -Path $fixtureArtifactRoot | Out-Null

$preexistingPsycleRegistry = Test-Path -LiteralPath "HKCU:\Software\Psycle"
if ($preexistingPsycleRegistry) {
    Fail "runner contains pre-existing HKCU\Software\Psycle configuration; refusing contaminated original-reference observation"
}

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

    $installerFramework = Get-InstallerFramework $installerPath
    $installerVersionInfo = $installerInfo.VersionInfo
    [System.IO.File]::WriteAllLines(
        $frameworkLog,
        @(
            "reference_file=$ReferenceFile",
            "reference_sha256=$installerSha",
            "reference_size=$($installerInfo.Length)",
            "framework=$installerFramework",
            "file_description=$($installerVersionInfo.FileDescription)",
            "product_name=$($installerVersionInfo.ProductName)",
            "file_version=$($installerVersionInfo.FileVersion)",
            "product_version=$($installerVersionInfo.ProductVersion)"
        ),
        [System.Text.UTF8Encoding]::new($false)
    )

    $installerArguments = $null
    if ($installerFramework -eq "inno-setup") {
        $installerArguments = @(
            "/VERYSILENT",
            "/SUPPRESSMSGBOXES",
            "/NORESTART",
            "/SP-",
            "/NOICONS",
            "/DIR=`"$installRoot`"",
            "/LOG=`"$installLog`""
        )
    } elseif ($installerFramework -eq "nsis") {
        $installerArguments = @("/S", "/D=$installRoot")
    } else {
        Write-InstallerDiagnostics $frameworkLog $installLog
        Fail "unsupported/unknown installer framework; refusing to guess unattended switches"
    }

    $installerProcess = Start-Process -FilePath $installerPath -ArgumentList $installerArguments -PassThru
    if (-not $installerProcess.WaitForExit(120000)) {
        try { $installerProcess.Kill() } catch { }
        Write-InstallerDiagnostics $frameworkLog $installLog
        Fail "reference installer did not finish within 120 seconds"
    }
    if ($installerProcess.ExitCode -ne 0) {
        Write-InstallerDiagnostics $frameworkLog $installLog
        Fail "reference installer returned nonzero exit code $($installerProcess.ExitCode)"
    }

    if ($installerFramework -eq "nsis") {
        [System.IO.File]::WriteAllText(
            $installLog,
            "framework=nsis`nexit_code=0`ninstall_root=$installRoot`n",
            [System.Text.UTF8Encoding]::new($false)
        )
    }
    Write-InstallerDiagnostics $frameworkLog $installLog

    $psycleExecutables = @(
        Get-ChildItem -LiteralPath $installRoot -Recurse -File |
            Where-Object { $_.Name -ieq "psycle.exe" }
    )
    if ($psycleExecutables.Count -ne 1) {
        $paths = ($psycleExecutables | ForEach-Object { $_.FullName }) -join "; "
        Fail "expected exactly one installed psycle.exe; found=$($psycleExecutables.Count) paths=$paths"
    }

    $psycleExe = $psycleExecutables[0]
    $psycleExeSha = Get-Sha256 $psycleExe.FullName
    $versionInfo = $psycleExe.VersionInfo

    $environment = [ordered]@{
        observation_mode = "native-windows-github-runner-transient-installed-payload"
        runner_os = $env:RUNNER_OS
        runner_arch = $env:RUNNER_ARCH
        image_os = $env:ImageOS
        image_version = $env:ImageVersion
        os_version = [Environment]::OSVersion.VersionString
        powershell_version = $PSVersionTable.PSVersion.ToString()
        installer_framework = $installerFramework
    }

    $postInstallRegistryExists = Test-Path -LiteralPath "HKCU:\Software\Psycle"
    $registrySnapshotPath = Join-Path $workRoot "psycle-post-install.reg"
    if ($postInstallRegistryExists) {
        & reg.exe export "HKCU\Software\Psycle" $registrySnapshotPath /y | Out-Null
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $registrySnapshotPath -PathType Leaf)) {
            Fail "could not snapshot post-install HKCU\Software\Psycle registry baseline"
        }
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
        if (Test-Path -LiteralPath "HKCU:\Software\Psycle") {
            Remove-Item -LiteralPath "HKCU:\Software\Psycle" -Recurse -Force
        }
        if ($postInstallRegistryExists) {
            & reg.exe import $registrySnapshotPath | Out-Null
            if ($LASTEXITCODE -ne 0) {
                Fail "could not restore post-install Psycle registry baseline for $($spec.name)"
            }
        }
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

        $candidateFixturePath = Resolve-ChildPath $candidateRoot ([string]$candidate.fixture)
        if (-not (Test-Path -LiteralPath $candidateFixturePath -PathType Leaf)) {
            Fail "candidate fixture is missing: $candidateFixturePath"
        }
        $fixtureSha = Get-Sha256 $candidateFixturePath
        if ($fixtureSha -ne ([string]$candidate.fixture_sha256).ToLowerInvariant()) {
            Fail "fixture SHA-256 does not match candidate receipt for $($spec.name)"
        }

        $fixtureDir = Join-Path $fixtureArtifactRoot $spec.name
        New-Item -ItemType Directory -Path $fixtureDir -Force | Out-Null
        $fixtureCopy = Join-Path $fixtureDir ([System.IO.Path]::GetFileName($candidateFixturePath))
        Copy-Item -LiteralPath $candidateFixturePath -Destination $fixtureCopy
        if ((Get-Sha256 $fixtureCopy) -ne $fixtureSha) {
            Fail "copied evidence fixture SHA-256 mismatch for $($spec.name)"
        }
        $fixtureReceiptPath = "fixtures/$($spec.name)/$([System.IO.Path]::GetFileName($fixtureCopy))"

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

        $argument = '"' + $fixtureCopy + '"'
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
        $uiDiagnostics = @()
        $uiDiagnosticSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
        $matchedMarker = $null
        $errorMarker = $null
        $stableMarkerPolls = 0

        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 500
            $process.Refresh()
            if ($process.HasExited) {
                break
            }
            $windowTitle = $process.MainWindowTitle
            $uiObservation = Get-UiObservation $process
            if ($process.MainWindowHandle -ne 0 -or [int]$uiObservation.top_level_window_count -gt 0) {
                $mainWindowSeen = $true
            }
            $uiValues = @($uiObservation.values)
            foreach ($diagnostic in @($uiObservation.diagnostics)) {
                if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                    [void]$uiDiagnosticSet.Add([string]$diagnostic)
                }
            }
            $uiDiagnostics = @($uiDiagnosticSet | Sort-Object)
            $combined = @($windowTitle) + $uiValues

            $errorMarker = $null
            foreach ($candidateText in $combined) {
                if ($candidateText -match '(?i)\b(error|failed|unsupported|corrupt|invalid)\b') {
                    $errorMarker = $candidateText
                    break
                }
            }
            if ($errorMarker) {
                break
            }

            $pollMarker = $null
            $markers = @(
                [System.IO.Path]::GetFileName($fixtureCopy),
                [System.IO.Path]::GetFileNameWithoutExtension($fixtureCopy),
                $spec.expected_song_title
            )
            foreach ($marker in $markers) {
                if ([string]::IsNullOrWhiteSpace($marker)) { continue }
                if ($combined | Where-Object { $_.IndexOf($marker, [System.StringComparison]::OrdinalIgnoreCase) -ge 0 }) {
                    $pollMarker = $marker
                    break
                }
            }

            if ($pollMarker -and $uiDiagnostics.Count -eq 0) {
                if ($matchedMarker -eq $pollMarker) {
                    $stableMarkerPolls += 1
                } else {
                    $matchedMarker = $pollMarker
                    $stableMarkerPolls = 1
                }
            } else {
                $matchedMarker = $pollMarker
                $stableMarkerPolls = 0
            }

            if ($stableMarkerPolls -ge 4) {
                break
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
        $evidenceLines.Add("candidate_fixture=$($candidate.fixture)")
        $evidenceLines.Add("fixture=$fixtureReceiptPath")
        $evidenceLines.Add("fixture_sha256=$fixtureSha")
        $evidenceLines.Add("process_id=$($process.Id)")
        $evidenceLines.Add("main_window_seen=$mainWindowSeen")
        $evidenceLines.Add("main_window_title=$windowTitle")
        $evidenceLines.Add("matched_marker=$matchedMarker")
        $evidenceLines.Add("stable_marker_polls=$stableMarkerPolls")
        $evidenceLines.Add("error_marker=$errorMarker")
        if ($null -ne $exitCode) {
            $evidenceLines.Add("exit_code=$exitCode")
        }
        $evidenceLines.Add("")
        $evidenceLines.Add("UI_TEXT:")
        foreach ($value in $uiValues) {
            $evidenceLines.Add($value)
        }
        $evidenceLines.Add("")
        $evidenceLines.Add("UI_AUTOMATION_DIAGNOSTICS:")
        foreach ($value in $uiDiagnostics) {
            $evidenceLines.Add($value)
        }
        [System.IO.File]::WriteAllLines($uiPath, $evidenceLines, [System.Text.UTF8Encoding]::new($false))

        $screenshotCaptured = Save-DesktopScreenshot $screenshotPath
        $screenshot = $null
        if ($screenshotCaptured -and (Test-Path -LiteralPath $screenshotPath -PathType Leaf)) {
            $screenshot = [ordered]@{
                path = [System.IO.Path]::GetFileName($screenshotPath)
                sha256 = Get-Sha256 $screenshotPath
            }
        }

        $machinePluginInventoryPath = Join-Path $outRoot ("machine-plugin-inventory-{0}.json" -f $spec.name)
        $machinePluginInventory = Write-MachinePluginInventory -InstallRoot $installRoot -ExecutablePath $psycleExe.FullName -Path $machinePluginInventoryPath -PreexistingPsycleRegistry $preexistingPsycleRegistry
        $environment["machine_plugin_inventory"] = $machinePluginInventory

        $process.Refresh()
        $exitedBeforeHarnessTermination = $process.HasExited
        if ($exitedBeforeHarnessTermination -and $null -eq $exitCode) {
            $exitCode = $process.ExitCode
        }
        $processRunningBeforeTermination = -not $exitedBeforeHarnessTermination

        $termination = "already-exited"
        if ($processRunningBeforeTermination) {
            try {
                $closeRequested = [bool]$process.CloseMainWindow()
                if (-not $closeRequested) {
                    $process.Refresh()
                    if ($process.HasExited) {
                        $exitCode = $process.ExitCode
                        $exitedBeforeHarnessTermination = $true
                        $processRunningBeforeTermination = $false
                        $termination = "exited-before-close-request"
                    } else {
                        $process.Kill()
                        $process.WaitForExit()
                        $termination = "killed-without-closeable-main-window"
                    }
                } elseif (-not $process.WaitForExit(5000)) {
                    $process.Kill()
                    $process.WaitForExit()
                    $termination = "killed-after-observation"
                } else {
                    $termination = "closed-after-observation"
                }
            }
            catch {
                try {
                    $process.Refresh()
                    if ($process.HasExited) {
                        $exitCode = $process.ExitCode
                        $exitedBeforeHarnessTermination = $true
                        $processRunningBeforeTermination = $false
                        $termination = "exited-during-close-error"
                    } else {
                        $process.Kill()
                        $process.WaitForExit()
                        $termination = "killed-after-close-error"
                    }
                }
                catch {
                    $termination = "termination-error"
                }
            }
        }

        if ($errorMarker) {
            $loadResult = "rejected"
            $observation = "native-window-evidence-reports-load-error"
        } elseif ($uiDiagnostics.Count -gt 0) {
            $loadResult = "inconclusive"
            $observation = "ui-automation-harness-diagnostic-prevents-behaviour-classification"
        } elseif ($exitedBeforeHarnessTermination) {
            $loadResult = "inconclusive"
            $observation = "reference-process-exited-before-harness-termination"
        } elseif ($stableMarkerPolls -ge 4 -and $matchedMarker) {
            $loadResult = "accepted"
            $observation = "stable-native-window-evidence-identifies-loaded-fixture-without-error"
        } elseif ($mainWindowSeen) {
            $loadResult = "inconclusive"
            $observation = "reference-window-opened-without-stable-fixture-load-marker"
        } else {
            $loadResult = "inconclusive"
            $observation = "reference-process-produced-no-observable-main-window"
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
            reference_executable = "transient installed psycle.exe from pinned installer"
            reference_executable_sha256 = $psycleExeSha
            reference_executable_file_version = $versionInfo.FileVersion
            reference_executable_product_version = $versionInfo.ProductVersion
            candidate_fixture = [string]$candidate.fixture
            fixture = $fixtureReceiptPath
            fixture_sha256 = $fixtureSha
            procedure = $Procedure
            observation = $observation
            load_result = $loadResult
            load_evidence_marker = $matchedMarker
            stable_marker_polls = $stableMarkerPolls
            error_marker = $errorMarker
            ui_automation_diagnostics = @($uiDiagnostics)
            main_window_seen = $mainWindowSeen
            main_window_title = $windowTitle
            exit_code_before_termination = $exitCode
            process_running_before_termination = $processRunningBeforeTermination
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

        Write-Host ("phase6c-original-windows-fixtures: {0} load_result={1} observation={2} marker={3} stable_polls={4} ui_diagnostics={5}" -f `
            $spec.name, $loadResult, $observation, $matchedMarker, $stableMarkerPolls, $uiDiagnostics.Count)
    }

    $summary = @"
# Phase 6C Original Psycle Native-Windows Evidence

- Reference: `$ReferenceBuild` / `$ReferenceFile`.
- Installer SHA-256: `$ExpectedInstallerSha256`.
- Installer size: `$ExpectedInstallerSize` bytes.
- Observation environment: native GitHub-hosted Windows runner.
- Reference installer and installed executable payload: transient only; not retained in this evidence directory.
- Inputs: exact project-authored fixture bytes copied into this artifact and SHA-256-bound to the corresponding candidate receipts.
- Acceptance rule: application load errors from every Psycle-owned top-level window take precedence over any filename/title marker; acceptance requires a stable fixture marker across four polls, no application error marker, no UI Automation diagnostic, and the reference process still running when harness termination begins.
- UI Automation failures are sticky harness diagnostics across the full observation and yield an inconclusive observation, never a rejection or later acceptance result.
- Configuration isolation: the post-install HKCU\\Software\\Psycle baseline is restored before each fixture so PSY2 cannot influence PSY3.
- Machine/plugin environment: the full installed payload, runtime Psycle registry state, configured plugin/machine roots, and conventional external VST/Psycle plugin roots are recorded in a fixture-specific SHA-256-bound machine-plugin inventory during each observation.
- Classification policy: these observations do not change compatibility status by themselves; rows remain UNKNOWN until versioned original + candidate receipts and a comparison verdict are committed.
"@
    [System.IO.File]::WriteAllText(
        (Join-Path $outRoot "summary.md"),
        $summary,
        [System.Text.UTF8Encoding]::new($false)
    )
}
finally {
    if (Test-Path -LiteralPath $workRoot) {
        Remove-Item -LiteralPath $workRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}