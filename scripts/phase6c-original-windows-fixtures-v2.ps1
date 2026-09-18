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
$RequiredVc90RuntimeFamily = "9.0"
$Procedure = "download pinned Psycle 1.12.0 x86 installer; verify PE magic/SHA-256/size; require a clean pre-install HKCU\Software\Psycle state; detect a supported installer framework and perform only its documented unattended install into runner-temp; snapshot the post-install Psycle registry baseline and restore it before each fixture; copy the exact project-authored fixture bytes into the evidence artifact; launch installed psycle.exe with the copied fixture; when and only when a Psycle-owned top-level window is exactly named Psycle Settings and exposes the expected OK, Cancel, Defaults and System controls, invoke its OK button once through UI Automation and record the bootstrap outcome; when and only when a later Psycle-owned top-level window is exactly named DirectSound Output driver and exposes the exact Failed to create DirectSound object message plus one OK button, invoke that OK as a separately recorded headless-runner environment bootstrap, using a timeout-bounded Win32 WM_COMMAND/IDOK only if the old MFC button ignores a successful UIA InvokePattern, and only after binding the already-verified DirectSound dialog HWND to the same Psycle process and the standard #32770 dialog class; enumerate top-level windows through a process-id UI Automation condition and inspect their descendants; inventory the complete installed payload, runtime Psycle registry state, configured plugin/machine paths, conventional external VST/Psycle roots, and the VC90 modules actually loaded by the live Psycle process with their real servicing versions and SHA-256s; reject only on concrete application error text; classify only fixture-associated load/parse application errors as rejection while unrelated application errors remain inconclusive; require runtime and UI harness identity to remain clean before behavioral classification; accept only after a stable fixture marker remains present through the full polling window and a final post-inventory UI scan, with no application error or harness diagnostic and while the process is still running when harness termination begins; capture logs/UI/screenshot evidence; terminate the process; delete installer, registry snapshot, and installed payload before artifact upload"

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

        $processCondition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty,
            $Process.Id
        )
        $windows = $desktop.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            $processCondition
        )
        foreach ($window in $windows) {
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


function Invoke-ExpectedFirstRunSettings([System.Diagnostics.Process]$Process) {
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $result = [ordered]@{
        settings_dialog_seen = $false
        signature_verified = $false
        attempted = $false
        action = $null
        dismissed = $false
        outcome = "not-seen"
        diagnostics = @()
    }

    try {
        $Process.Refresh()
        if ($Process.HasExited) {
            return $result
        }

        $desktop = [System.Windows.Automation.AutomationElement]::RootElement
        if ($null -eq $desktop) {
            $diagnostics.Add("settings bootstrap UI Automation returned no desktop root element")
            $result.outcome = "desktop-root-missing"
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }

        $processCondition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty,
            $Process.Id
        )
        $windows = $desktop.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            $processCondition
        )

        foreach ($window in $windows) {
            $windowName = ""
            try {
                $windowName = [string]$window.Current.Name
            }
            catch {
                $diagnostics.Add(
                    "settings bootstrap Psycle top-level window read failed: $($_.Exception.Message)"
                )
                continue
            }
            if ($windowName -cne "Psycle Settings") {
                continue
            }

            $result.settings_dialog_seen = $true
            $nodes = $null
            try {
                $nodes = $window.FindAll(
                    [System.Windows.Automation.TreeScope]::Descendants,
                    [System.Windows.Automation.Condition]::TrueCondition
                )
            }
            catch {
                $diagnostics.Add(
                    "settings bootstrap descendant enumeration failed: $($_.Exception.Message)"
                )
                $result.outcome = "signature-read-failed"
                break
            }

            $names = [System.Collections.Generic.HashSet[string]]::new(
                [System.StringComparer]::Ordinal
            )
            $okButtons = [System.Collections.Generic.List[object]]::new()
            foreach ($node in $nodes) {
                try {
                    $name = [string]$node.Current.Name
                    if (-not [string]::IsNullOrWhiteSpace($name)) {
                        [void]$names.Add($name.Trim())
                    }
                    if ($name -ceq "OK" -and
                        $node.Current.ControlType -eq
                            [System.Windows.Automation.ControlType]::Button) {
                        $okButtons.Add($node)
                    }
                }
                catch {
                    $diagnostics.Add(
                        "settings bootstrap UI element read failed: $($_.Exception.Message)"
                    )
                }
            }

            $requiredNames = @("OK", "Cancel", "Defaults", "System")
            $missingNames = @(
                $requiredNames | Where-Object { -not $names.Contains($_) }
            )
            if ($missingNames.Count -gt 0 -or $okButtons.Count -ne 1) {
                $diagnostics.Add(
                    "settings bootstrap signature mismatch: " +
                    "missing=$($missingNames -join ',') ok_buttons=$($okButtons.Count)"
                )
                $result.outcome = "signature-mismatch"
                break
            }

            $result.signature_verified = $true
            $result.attempted = $true
            $result.action = "invoke-ok"
            try {
                $invokePattern = [System.Windows.Automation.InvokePattern](
                    $okButtons[0].GetCurrentPattern(
                        [System.Windows.Automation.InvokePattern]::Pattern
                    )
                )
                $invokePattern.Invoke()
            }
            catch {
                $diagnostics.Add(
                    "settings bootstrap OK invoke failed: $($_.Exception.Message)"
                )
                $result.outcome = "invoke-failed"
                break
            }

            $closed = $false
            try {
                for ($attempt = 0; $attempt -lt 20; $attempt += 1) {
                    Start-Sleep -Milliseconds 100
                    $Process.Refresh()
                    if ($Process.HasExited) {
                        throw "settings bootstrap process exited during close verification"
                    }

                    $remaining = $desktop.FindAll(
                        [System.Windows.Automation.TreeScope]::Children,
                        $processCondition
                    )
                    $settingsStillPresent = $false
                    foreach ($remainingWindow in $remaining) {
                        try {
                            if ([string]$remainingWindow.Current.Name -ceq "Psycle Settings") {
                                $settingsStillPresent = $true
                                break
                            }
                        }
                        catch {
                            throw (
                                "settings bootstrap close verification failed: " +
                                "$($_.Exception.Message)"
                            )
                        }
                    }
                    if (-not $settingsStillPresent) {
                        $Process.Refresh()
                        if ($Process.HasExited) {
                            throw "settings bootstrap process exited during close verification"
                        }
                        $closed = $true
                        break
                    }
                }
            }
            catch {
                $diagnostics.Add([string]$_.Exception.Message)
                $result.outcome = "close-verification-failed"
            }

            if ($result.outcome -ceq "close-verification-failed") {
                break
            }
            if ($closed) {
                $result.dismissed = $true
                $result.outcome = "dismissed"
            } else {
                $diagnostics.Add(
                    "settings bootstrap dialog remained open after invoking OK"
                )
                $result.outcome = "close-timeout"
            }
            break
        }
    }
    catch {
        $diagnostics.Add(
            "settings bootstrap UI Automation failed: $($_.Exception.Message)"
        )
        $result.outcome = "automation-failed"
    }

    if ($result.outcome -ceq "not-seen" -and $diagnostics.Count -gt 0) {
        $result.outcome = "automation-failed"
    }
    $result.diagnostics = @($diagnostics | Select-Object -Unique)
    return $result
}


function Invoke-ExpectedDirectSoundFailure([System.Diagnostics.Process]$Process) {
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $result = [ordered]@{
        dialog_seen = $false
        signature_verified = $false
        attempted = $false
        action = $null
        dismissed = $false
        outcome = "not-seen"
        diagnostics = @()
    }

    try {
        $Process.Refresh()
        if ($Process.HasExited) {
            return $result
        }

        $desktop = [System.Windows.Automation.AutomationElement]::RootElement
        if ($null -eq $desktop) {
            $diagnostics.Add("DirectSound bootstrap UI Automation returned no desktop root element")
            $result.outcome = "desktop-root-missing"
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }

        $processCondition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty,
            $Process.Id
        )
        $windows = $desktop.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            $processCondition
        )

        foreach ($window in $windows) {
            $windowName = ""
            try {
                $windowName = [string]$window.Current.Name
            }
            catch {
                $diagnostics.Add(
                    "DirectSound bootstrap Psycle top-level window read failed: $($_.Exception.Message)"
                )
                continue
            }
            if ($windowName -cne "DirectSound Output driver") {
                continue
            }

            $result.dialog_seen = $true
            $nodes = $null
            try {
                $nodes = $window.FindAll(
                    [System.Windows.Automation.TreeScope]::Descendants,
                    [System.Windows.Automation.Condition]::TrueCondition
                )
            }
            catch {
                $diagnostics.Add(
                    "DirectSound bootstrap descendant enumeration failed: $($_.Exception.Message)"
                )
                $result.outcome = "signature-read-failed"
                break
            }

            $messageSeen = $false
            $okButtons = [System.Collections.Generic.List[object]]::new()
            foreach ($node in $nodes) {
                try {
                    $name = [string]$node.Current.Name
                    if ($name -ceq "Failed to create DirectSound object") {
                        $messageSeen = $true
                    }
                    if ($name -ceq "OK" -and
                        $node.Current.ControlType -eq
                            [System.Windows.Automation.ControlType]::Button) {
                        $okButtons.Add($node)
                    }
                }
                catch {
                    $diagnostics.Add(
                        "DirectSound bootstrap UI element read failed: $($_.Exception.Message)"
                    )
                }
            }

            if (-not $messageSeen -or $okButtons.Count -ne 1) {
                $diagnostics.Add(
                    "DirectSound bootstrap signature mismatch: " +
                    "message_seen=$messageSeen ok_buttons=$($okButtons.Count)"
                )
                $result.outcome = "signature-mismatch"
                break
            }

            $result.signature_verified = $true
            $result.attempted = $true
            $result.action = "invoke-ok"
            try {
                $invokePattern = [System.Windows.Automation.InvokePattern](
                    $okButtons[0].GetCurrentPattern(
                        [System.Windows.Automation.InvokePattern]::Pattern
                    )
                )
                $invokePattern.Invoke()
            }
            catch {
                $diagnostics.Add(
                    "DirectSound bootstrap OK invoke failed: $($_.Exception.Message)"
                )
                $result.outcome = "invoke-failed"
                break
            }

            $dialogIsClosed = {
                $Process.Refresh()
                if ($Process.HasExited) {
                    throw "DirectSound bootstrap process exited during close verification"
                }
                $remaining = $desktop.FindAll(
                    [System.Windows.Automation.TreeScope]::Children,
                    $processCondition
                )
                foreach ($remainingWindow in $remaining) {
                    try {
                        if ([string]$remainingWindow.Current.Name -ceq
                            "DirectSound Output driver") {
                            return $false
                        }
                    }
                    catch {
                        throw "DirectSound bootstrap close verification failed: $($_.Exception.Message)"
                    }
                }
                $Process.Refresh()
                if ($Process.HasExited) {
                    throw "DirectSound bootstrap process exited during close verification"
                }
                return $true
            }

            $closed = $false
            try {
                for ($attempt = 0; $attempt -lt 5; $attempt += 1) {
                    Start-Sleep -Milliseconds 100
                    if (& $dialogIsClosed) {
                        $closed = $true
                        break
                    }
                }
            }
            catch {
                $diagnostics.Add([string]$_.Exception.Message)
                $result.outcome = "close-verification-failed"
                break
            }

            if (-not $closed) {
                # Psycle 1.12 uses an old MFC message box. On the hosted runner
                # UIA InvokePattern can report success without dispatching the
                # button action, while the UIA child proxy does not expose a
                # live native button HWND. The fallback therefore targets only
                # the already-verified process-owned DirectSound dialog and
                # issues its standard IDOK command after re-validating the
                # native dialog HWND and class.
                $result.action = "invoke-ok-win32-wm-command"
                try {
                    $dialogHandleValue = [int64]$window.Current.NativeWindowHandle
                    if ($dialogHandleValue -eq 0) {
                        throw "DirectSound bootstrap verified dialog has no native HWND"
                    }
                    $dialogHandle = [IntPtr]::new($dialogHandleValue)
                    if (-not [Phase6cNativeButton]::IsWindow($dialogHandle)) {
                        throw "DirectSound bootstrap verified dialog HWND is not a live window"
                    }

                    [uint32]$dialogProcessId = 0
                    [void][Phase6cNativeButton]::GetWindowThreadProcessId(
                        $dialogHandle,
                        [ref]$dialogProcessId
                    )
                    if ($dialogProcessId -ne [uint32]$Process.Id) {
                        throw (
                            "DirectSound bootstrap verified dialog HWND belongs to the wrong process: " +
                            "expected=$($Process.Id) actual=$dialogProcessId"
                        )
                    }

                    $className = [System.Text.StringBuilder]::new(64)
                    $classLength = [Phase6cNativeButton]::GetClassName(
                        $dialogHandle,
                        $className,
                        $className.Capacity
                    )
                    if ($classLength -le 0 -or $className.ToString() -cne "#32770") {
                        throw (
                            "DirectSound bootstrap verified dialog HWND has unexpected class: " +
                            "$($className.ToString())"
                        )
                    }

                    $WM_COMMAND = [uint32]0x0111
                    $IDOK = [IntPtr]::new(1)
                    $SMTO_ABORTIFHUNG = [uint32]0x0002
                    [IntPtr]$messageResult = [IntPtr]::Zero
                    $sendResult = [Phase6cNativeButton]::SendMessageTimeout(
                        $dialogHandle,
                        $WM_COMMAND,
                        $IDOK,
                        [IntPtr]::Zero,
                        $SMTO_ABORTIFHUNG,
                        [uint32]2000,
                        [ref]$messageResult
                    )
                    if ($sendResult -eq [IntPtr]::Zero) {
                        $lastError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
                        throw (
                            "DirectSound bootstrap WM_COMMAND/IDOK failed or timed out: " +
                            "win32_error=$lastError"
                        )
                    }
                }
                catch {
                    $diagnostics.Add(
                        "DirectSound bootstrap Win32 OK action failed: $($_.Exception.Message)"
                    )
                    $result.outcome = "win32-action-failed"
                    break
                }

                try {
                    for ($attempt = 0; $attempt -lt 20; $attempt += 1) {
                        Start-Sleep -Milliseconds 100
                        if (& $dialogIsClosed) {
                            $closed = $true
                            break
                        }
                    }
                }
                catch {
                    $diagnostics.Add([string]$_.Exception.Message)
                    $result.outcome = "close-verification-failed"
                    break
                }
            }

            if ($closed) {
                $result.dismissed = $true
                $result.outcome = "dismissed"
            } else {
                $diagnostics.Add(
                    "DirectSound bootstrap dialog remained open after verified OK actions"
                )
                $result.outcome = "close-timeout"
            }
            break
        }
    }
    catch {
        $diagnostics.Add(
            "DirectSound bootstrap UI Automation failed: $($_.Exception.Message)"
        )
        $result.outcome = "automation-failed"
    }

    if ($result.outcome -ceq "not-seen" -and $diagnostics.Count -gt 0) {
        $result.outcome = "automation-failed"
    }
    $result.diagnostics = @($diagnostics | Select-Object -Unique)
    return $result
}

function Get-FixtureUiAssessment([object[]]$Texts, [string[]]$Markers) {
    $applicationErrorMarker = $null
    $fixtureLoadErrorMarker = $null
    $fixtureMarker = $null

    foreach ($candidateText in @($Texts)) {
        if ([string]::IsNullOrWhiteSpace([string]$candidateText)) {
            continue
        }
        $text = [string]$candidateText

        foreach ($marker in @($Markers)) {
            if ([string]::IsNullOrWhiteSpace([string]$marker)) {
                continue
            }
            if ($text.IndexOf([string]$marker, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
                if ($null -eq $fixtureMarker) {
                    $fixtureMarker = [string]$marker
                }
            }
        }

        $isApplicationError = $text -match '(?i)\b(error|failed|unsupported|corrupt|invalid)\b'
        if ($isApplicationError -and $null -eq $applicationErrorMarker) {
            $applicationErrorMarker = $text
        }

        if ($isApplicationError -and
            $text -match '(?i)\b(load|loading|loaded|open|opening|read|reading|parse|parsing|file|song|project)\b') {
            foreach ($marker in @($Markers)) {
                if ([string]::IsNullOrWhiteSpace([string]$marker)) {
                    continue
                }
                if ($text.IndexOf([string]$marker, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
                    $fixtureLoadErrorMarker = $text
                    break
                }
            }
        }

        if ($fixtureLoadErrorMarker) {
            break
        }
    }

    return [ordered]@{
        fixture_marker = $fixtureMarker
        application_error_marker = $applicationErrorMarker
        fixture_load_error_marker = $fixtureLoadErrorMarker
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

function Write-LoadedVc90RuntimeInventory(
    [System.Diagnostics.Process]$Process,
    [string]$Path
) {
    $modules = [System.Collections.Generic.List[object]]::new()
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $seenPaths = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase
    )

    try {
        $Process.Refresh()
        if ($Process.HasExited) {
            $diagnostics.Add("reference process exited before loaded VC90 runtime inventory")
        } else {
            foreach ($module in $Process.Modules) {
                try {
                    $name = [string]$module.ModuleName
                    if ($name -notmatch '^(?i:msvcr90|msvcp90|mfc90|mfc90u|atl90)\.dll$') {
                        continue
                    }

                    $filePath = [string]$module.FileName
                    if ([string]::IsNullOrWhiteSpace($filePath) -or
                        -not (Test-Path -LiteralPath $filePath -PathType Leaf)) {
                        $diagnostics.Add("loaded VC90 module has no readable file path: $name")
                        continue
                    }
                    $filePath = [System.IO.Path]::GetFullPath($filePath)
                    if (-not $seenPaths.Add($filePath)) {
                        continue
                    }

                    $item = Get-Item -LiteralPath $filePath
                    $versionInfo = $item.VersionInfo
                    $fileVersionRaw = [string]$versionInfo.FileVersion
                    $normalizedVersion = $null
                    if ($fileVersionRaw -match '(\d+)\.(\d+)\.(\d+)\.(\d+)') {
                        $normalizedVersion = "{0}.{1}.{2}.{3}" -f
                            [int]$Matches[1], [int]$Matches[2],
                            [int]$Matches[3], [int]$Matches[4]
                    } else {
                        $diagnostics.Add(
                            "could not normalize loaded VC90 module version for $($name): $fileVersionRaw"
                        )
                    }

                    if ($normalizedVersion -and
                        -not $normalizedVersion.StartsWith(
                            "$RequiredVc90RuntimeFamily.",
                            [System.StringComparison]::Ordinal
                        )) {
                        $diagnostics.Add(
                            "loaded module is outside the VC90 runtime family for $($name): " +
                            "required_family=$RequiredVc90RuntimeFamily actual=$normalizedVersion"
                        )
                    }

                    $modules.Add([ordered]@{
                        name = $name.ToLowerInvariant()
                        path = $filePath
                        size_bytes = [long]$item.Length
                        sha256 = Get-Sha256 $filePath
                        file_version = $normalizedVersion
                        file_version_raw = $fileVersionRaw
                        product_version = [string]$versionInfo.ProductVersion
                    })
                }
                catch {
                    $diagnostics.Add("loaded VC90 module inspection failed: $($_.Exception.Message)")
                }
            }
        }
    }
    catch {
        $diagnostics.Add("loaded VC90 module enumeration failed: $($_.Exception.Message)")
    }

    $hasMsvcr90 = $false
    foreach ($module in $modules) {
        if ($module.name -ieq "msvcr90.dll") {
            $hasMsvcr90 = $true
            break
        }
    }
    if (-not $hasMsvcr90) {
        $diagnostics.Add("live Psycle process did not expose msvcr90.dll in its loaded module set")
    }

    $inventory = [ordered]@{
        schema_version = 1
        reference_build = $ReferenceBuild
        process_id = $Process.Id
        required_vc90_family = $RequiredVc90RuntimeFamily
        modules = $modules.ToArray()
        diagnostics = @($diagnostics | Select-Object -Unique)
    }
    Write-JsonUtf8 $Path $inventory

    return [ordered]@{
        binding = [ordered]@{
            path = [System.IO.Path]::GetFileName($Path)
            sha256 = Get-Sha256 $Path
        }
        diagnostics = @($inventory.diagnostics)
    }
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

    $registryEntries = [System.Collections.Generic.List[object]]::new()
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

    $pluginRoots = [System.Collections.Generic.List[object]]::new()
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
        psycle_registry = $registryEntries.ToArray()
        plugin_roots = $pluginRoots.ToArray()
        external_plugin_dll_count = $externalPluginDllCount
        configuration_baseline = "post-installer HKCU\Software\Psycle snapshot restored before this fixture"
        external_visibility_note = "Fresh runner required no pre-existing HKCU\Software\Psycle configuration. Installed payload, runtime Psycle registry state, configured plugin/machine paths, and conventional VST/Psycle plugin roots are SHA-256 inventoried during this observation."
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
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class Phase6cNativeButton
{
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool IsWindow(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder className, int maxCount);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr SendMessageTimeout(
        IntPtr hWnd,
        uint msg,
        IntPtr wParam,
        IntPtr lParam,
        uint flags,
        uint timeout,
        out IntPtr result
    );
}
"@

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
        $markers = @(
            [System.IO.Path]::GetFileName($fixtureCopy),
            [System.IO.Path]::GetFileNameWithoutExtension($fixtureCopy),
            $spec.expected_song_title
        )
        $matchedMarker = $null
        $applicationErrorMarker = $null
        $fixtureLoadErrorMarker = $null
        $stableMarkerPolls = 0
        $settingsBootstrapTerminal = $false
        $settingsBootstrap = [ordered]@{
            settings_dialog_seen = $false
            signature_verified = $false
            attempted = $false
            action = $null
            dismissed = $false
            outcome = "not-seen"
            diagnostics = @()
        }
        $directSoundBootstrapTerminal = $false
        $directSoundBootstrap = [ordered]@{
            dialog_seen = $false
            signature_verified = $false
            attempted = $false
            action = $null
            dismissed = $false
            outcome = "not-seen"
            diagnostics = @()
        }

        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 500
            $process.Refresh()
            if ($process.HasExited) {
                break
            }
            if (-not $settingsBootstrapTerminal) {
                $bootstrapObservation = Invoke-ExpectedFirstRunSettings $process
                if ($bootstrapObservation.settings_dialog_seen) {
                    $settingsBootstrapTerminal = $true
                }
                if ($bootstrapObservation.outcome -ne "not-seen" -or
                    $bootstrapObservation.settings_dialog_seen) {
                    $settingsBootstrap = $bootstrapObservation
                }
                foreach ($diagnostic in @($bootstrapObservation.diagnostics)) {
                    if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                        [void]$uiDiagnosticSet.Add([string]$diagnostic)
                    }
                }
            }

            if (-not $directSoundBootstrapTerminal) {
                $driverBootstrapObservation = Invoke-ExpectedDirectSoundFailure $process
                if ($driverBootstrapObservation.dialog_seen) {
                    $directSoundBootstrapTerminal = $true
                }
                if ($driverBootstrapObservation.outcome -ne "not-seen" -or
                    $driverBootstrapObservation.dialog_seen) {
                    $directSoundBootstrap = $driverBootstrapObservation
                }
                foreach ($diagnostic in @($driverBootstrapObservation.diagnostics)) {
                    if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                        [void]$uiDiagnosticSet.Add([string]$diagnostic)
                    }
                }
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
            $assessment = Get-FixtureUiAssessment -Texts $combined -Markers $markers

            if (-not $directSoundBootstrapTerminal -and
                [string]$assessment.application_error_marker -ceq
                    "Failed to create DirectSound object") {
                $driverBootstrapObservation = Invoke-ExpectedDirectSoundFailure $process
                if ($driverBootstrapObservation.dialog_seen) {
                    $directSoundBootstrapTerminal = $true
                }
                if ($driverBootstrapObservation.outcome -ne "not-seen" -or
                    $driverBootstrapObservation.dialog_seen) {
                    $directSoundBootstrap = $driverBootstrapObservation
                }
                foreach ($diagnostic in @($driverBootstrapObservation.diagnostics)) {
                    if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                        [void]$uiDiagnosticSet.Add([string]$diagnostic)
                    }
                }
                $uiDiagnostics = @($uiDiagnosticSet | Sort-Object)
                if ($driverBootstrapObservation.dialog_seen) {
                    $assessment.application_error_marker = $null
                }
            }

            if ($null -eq $applicationErrorMarker -and $assessment.application_error_marker) {
                $applicationErrorMarker = [string]$assessment.application_error_marker
            }
            if ($assessment.fixture_load_error_marker) {
                $fixtureLoadErrorMarker = [string]$assessment.fixture_load_error_marker
            }

            $pollMarker = $assessment.fixture_marker
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

            if ($fixtureLoadErrorMarker) {
                break
            }
        }

        $process.Refresh()
        $exitCode = $null
        if ($process.HasExited) {
            $exitCode = $process.ExitCode
        }

        $machinePluginInventoryPath = Join-Path $outRoot ("machine-plugin-inventory-{0}.json" -f $spec.name)
        $machinePluginInventory = Write-MachinePluginInventory -InstallRoot $installRoot -ExecutablePath $psycleExe.FullName -Path $machinePluginInventoryPath -PreexistingPsycleRegistry $preexistingPsycleRegistry
        $environment["machine_plugin_inventory"] = $machinePluginInventory

        $loadedRuntimeInventoryPath = Join-Path $outRoot ("loaded-vc90-runtime-{0}.json" -f $spec.name)
        $loadedRuntimeObservation = Write-LoadedVc90RuntimeInventory -Process $process -Path $loadedRuntimeInventoryPath
        $environment["loaded_vc90_runtime"] = $loadedRuntimeObservation.binding
        $runtimeIdentityDiagnostics = @($loadedRuntimeObservation.diagnostics)

        # Inventory hashing can take long enough for a late parse/machine error
        # dialog to appear. Re-scan the complete Psycle-owned UI immediately
        # before liveness capture and harness termination.
        $process.Refresh()
        if (-not $process.HasExited) {
            $windowTitle = $process.MainWindowTitle
            $finalUiObservation = Get-UiObservation $process
            if ($process.MainWindowHandle -ne 0 -or [int]$finalUiObservation.top_level_window_count -gt 0) {
                $mainWindowSeen = $true
            }
            $uiValues = @($finalUiObservation.values)
            foreach ($diagnostic in @($finalUiObservation.diagnostics)) {
                if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                    [void]$uiDiagnosticSet.Add([string]$diagnostic)
                }
            }
            $uiDiagnostics = @($uiDiagnosticSet | Sort-Object)
            $finalCombined = @($windowTitle) + $uiValues
            $finalAssessment = Get-FixtureUiAssessment -Texts $finalCombined -Markers $markers
            if (-not $directSoundBootstrapTerminal -and
                [string]$finalAssessment.application_error_marker -ceq
                    "Failed to create DirectSound object") {
                $driverBootstrapObservation = Invoke-ExpectedDirectSoundFailure $process
                if ($driverBootstrapObservation.dialog_seen) {
                    $directSoundBootstrapTerminal = $true
                }
                if ($driverBootstrapObservation.outcome -ne "not-seen" -or
                    $driverBootstrapObservation.dialog_seen) {
                    $directSoundBootstrap = $driverBootstrapObservation
                }
                foreach ($diagnostic in @($driverBootstrapObservation.diagnostics)) {
                    if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                        [void]$uiDiagnosticSet.Add([string]$diagnostic)
                    }
                }
                $uiDiagnostics = @($uiDiagnosticSet | Sort-Object)
                if ($driverBootstrapObservation.dialog_seen) {
                    $finalAssessment.application_error_marker = $null
                }
            }
            if ($null -eq $applicationErrorMarker -and $finalAssessment.application_error_marker) {
                $applicationErrorMarker = [string]$finalAssessment.application_error_marker
            }
            if ($finalAssessment.fixture_load_error_marker) {
                $fixtureLoadErrorMarker = [string]$finalAssessment.fixture_load_error_marker
            }

            $finalMarker = $finalAssessment.fixture_marker
            if ($finalMarker -and $uiDiagnostics.Count -eq 0) {
                if ($matchedMarker -eq $finalMarker) {
                    $stableMarkerPolls += 1
                } else {
                    $matchedMarker = $finalMarker
                    $stableMarkerPolls = 1
                }
            } else {
                $matchedMarker = $finalMarker
                $stableMarkerPolls = 0
            }
        }

        $process.Refresh()
        $exitedBeforeHarnessTermination = $process.HasExited
        if ($exitedBeforeHarnessTermination -and $null -eq $exitCode) {
            $exitCode = $process.ExitCode
        }
        $processRunningBeforeTermination = -not $exitedBeforeHarnessTermination

        $evidenceLines = [System.Collections.Generic.List[string]]::new()
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
        $evidenceLines.Add("application_error_marker=$applicationErrorMarker")
        $evidenceLines.Add("fixture_load_error_marker=$fixtureLoadErrorMarker")
        $evidenceLines.Add("settings_bootstrap_seen=$($settingsBootstrap.settings_dialog_seen)")
        $evidenceLines.Add("settings_bootstrap_signature_verified=$($settingsBootstrap.signature_verified)")
        $evidenceLines.Add("settings_bootstrap_attempted=$($settingsBootstrap.attempted)")
        $evidenceLines.Add("settings_bootstrap_action=$($settingsBootstrap.action)")
        $evidenceLines.Add("settings_bootstrap_dismissed=$($settingsBootstrap.dismissed)")
        $evidenceLines.Add("settings_bootstrap_outcome=$($settingsBootstrap.outcome)")
        $evidenceLines.Add("directsound_bootstrap_seen=$($directSoundBootstrap.dialog_seen)")
        $evidenceLines.Add("directsound_bootstrap_signature_verified=$($directSoundBootstrap.signature_verified)")
        $evidenceLines.Add("directsound_bootstrap_attempted=$($directSoundBootstrap.attempted)")
        $evidenceLines.Add("directsound_bootstrap_action=$($directSoundBootstrap.action)")
        $evidenceLines.Add("directsound_bootstrap_dismissed=$($directSoundBootstrap.dismissed)")
        $evidenceLines.Add("directsound_bootstrap_outcome=$($directSoundBootstrap.outcome)")
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

        if ($uiDiagnostics.Count -gt 0) {
            $loadResult = "inconclusive"
            $observation = "ui-automation-harness-diagnostic-prevents-behaviour-classification"
        } elseif ($runtimeIdentityDiagnostics.Count -gt 0) {
            $loadResult = "inconclusive"
            $observation = "loaded-vc90-runtime-identity-diagnostic-prevents-behaviour-classification"
        } elseif ($fixtureLoadErrorMarker) {
            $loadResult = "rejected"
            $observation = "fixture-associated-native-window-evidence-reports-load-error"
        } elseif ($applicationErrorMarker) {
            $loadResult = "inconclusive"
            $observation = "application-error-not-bound-to-fixture-load"
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
            error_marker = $fixtureLoadErrorMarker
            application_error_marker = $applicationErrorMarker
            error_marker_scope = if ($fixtureLoadErrorMarker) {
                "fixture-load"
            } elseif ($applicationErrorMarker) {
                "application-unassociated"
            } else {
                "none"
            }
            ui_automation_diagnostics = @($uiDiagnostics)
            startup_bootstrap = $settingsBootstrap
            environment_bootstrap = [ordered]@{
                directsound = $directSoundBootstrap
            }
            runtime_identity_diagnostics = @($runtimeIdentityDiagnostics)
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
- Rejection rule: only an error-bearing UI text item that is explicitly associated with this fixture and load/open/read/parse/project semantics can classify the fixture as rejected; unrelated Psycle application errors force an inconclusive result instead.
- Acceptance rule: the observer continues through the full polling window, performs a final complete Psycle-owned UI scan after runtime inventory hashing, and accepts only with a stable fixture marker, no application error, no UI Automation diagnostic, and the reference process still running when harness termination begins.
- First-run bootstrap: only a Psycle-owned top-level window exactly named Psycle Settings with the expected OK, Cancel, Defaults and System controls may be automated, and only its OK button is invoked. Bootstrap ambiguity/failure is a sticky UI Automation diagnostic and therefore inconclusive.
- Headless-runner audio bootstrap: only a Psycle-owned top-level window exactly named DirectSound Output driver with the exact Failed to create DirectSound object message and exactly one OK button may be dismissed. UIA InvokePattern is tried first; a LegacyIAccessible default-action fallback is permitted only for that same verified MFC button and is recorded in the receipt. It is recorded separately as environment bootstrap evidence and never treated as fixture rejection; ambiguity/failure is sticky and inconclusive.
- UI Automation failures and loaded-runtime identity failures are sticky harness diagnostics across the full observation and yield an inconclusive observation, never a rejection or later acceptance result.
- Loaded VC90 runtime identity: each fixture records a hash-bound inventory of the VC90 CRT/MFC/ATL modules actually loaded by the live Psycle process, including absolute module path, size, SHA-256, and file/product version. Windows SxS servicing revisions are recorded as observed and may differ between CRT/MFC components; missing identity or a module outside the VC90 9.0 family prevents behavioural classification.
- Configuration isolation: the post-install HKCU\Software\Psycle baseline is restored before each fixture so PSY2 cannot influence PSY3.
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
    if (-not $preexistingPsycleRegistry -and (Test-Path -LiteralPath "HKCU:\Software\Psycle")) {
        Remove-Item -LiteralPath "HKCU:\Software\Psycle" -Recurse -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path -LiteralPath $workRoot) {
        Remove-Item -LiteralPath $workRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}