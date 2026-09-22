# Optional Phase 6C debugger helper for the pinned original-Psycle Sampler crash.
# This helper is diagnostic only. It never promotes compatibility or parity.

function Get-Phase6cFaultSha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Write-Phase6cFaultJson([string]$Path, [object]$Value) {
    $json = $Value | ConvertTo-Json -Depth 12
    [System.IO.File]::WriteAllText(
        $Path,
        $json + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false)
    )
}

function Get-Phase6cFaultRelativePath([string]$Root, [string]$Path) {
    return [System.IO.Path]::GetRelativePath(
        [System.IO.Path]::GetFullPath($Root),
        [System.IO.Path]::GetFullPath($Path)
    ).Replace("\", "/")
}

function Get-Phase6cX86Cdb {
    $candidates = [System.Collections.Generic.List[string]]::new()
    $programFilesX86 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
    if (-not [string]::IsNullOrWhiteSpace($programFilesX86)) {
        $candidates.Add(
            (Join-Path $programFilesX86 "Windows Kits\10\Debuggers\x86\cdb.exe")
        )
    }

    try {
        $command = Get-Command cdb.exe -ErrorAction Stop
        if ($null -ne $command -and
            -not [string]::IsNullOrWhiteSpace([string]$command.Source)) {
            $candidates.Add([string]$command.Source)
        }
    }
    catch {
        # Tool absence is an allowed inconclusive result.
    }

    $seen = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase
    )
    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        $full = [System.IO.Path]::GetFullPath($candidate)
        if (-not $seen.Add($full)) {
            continue
        }
        if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
            continue
        }

        if ($full -notmatch '(?i)[\\/]Debuggers[\\/]x86[\\/]cdb\.exe$') {
            continue
        }
        return Get-Item -LiteralPath $full
    }
    return $null
}

function Convert-Phase6cFaultHex([string]$Value) {
    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $null
    }
    $clean = $Value.Trim().ToLowerInvariant().Replace([string][char]0x60, "")
    if ($clean.StartsWith("0x")) {
        $clean = $clean.Substring(2)
    }
    if ($clean -notmatch '^[0-9a-f]+$') {
        return $null
    }
    try {
        return [Convert]::ToUInt64($clean, 16)
    }
    catch {
        return $null
    }
}

function Get-Phase6cFaultModuleSnapshot([System.Diagnostics.Process]$Process) {
    $Process.Refresh()
    if ($Process.HasExited) {
        throw "reference process exited before debugger module snapshot"
    }

    $modules = [System.Collections.Generic.List[object]]::new()
    foreach ($module in $Process.Modules) {
        try {
            $baseSigned = [int64]$module.BaseAddress.ToInt64()
            $base = if ($baseSigned -lt 0) {
                [uint64]($baseSigned + 0x100000000L)
            } else {
                [uint64]$baseSigned
            }
            $size = [uint64]$module.ModuleMemorySize
            $path = [string]$module.FileName
            $sha = $null
            if (-not [string]::IsNullOrWhiteSpace($path) -and
                (Test-Path -LiteralPath $path -PathType Leaf)) {
                $sha = Get-Phase6cFaultSha256 $path
                $path = [System.IO.Path]::GetFullPath($path)
            }
            $modules.Add([ordered]@{
                name = [string]$module.ModuleName
                path = $path
                base_address_hex = ("0x{0:x8}" -f $base)
                size_bytes = [int64]$size
                end_address_hex = ("0x{0:x8}" -f ($base + $size))
                sha256 = $sha
            })
        }
        catch {
            throw "module snapshot failed for $($module.ModuleName): $($_.Exception.Message)"
        }
    }
    return @($modules | Sort-Object base_address_hex)
}

function Invoke-Phase6cFaultLocationRender(
    [System.Diagnostics.Process]$Process,
    [string]$ExpectedTitle,
    [string]$OutputPath,
    [string]$EvidenceDirectory,
    [string]$ArtifactRoot
) {
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $result = [ordered]@{
        schema_version = 1
        scope = "pinned-original-sampler-fault-location"
        outcome = "inconclusive"
        debugger = $null
        module_snapshot = $null
        debugger_log = $null
        render_attempt = $null
        exception = $null
        function_location = "unresolved"
        diagnostics = @()
    }

    New-Item -ItemType Directory -Path $EvidenceDirectory -Force | Out-Null
    $modulePath = Join-Path $EvidenceDirectory "modules.json"
    $logPath = Join-Path $EvidenceDirectory "cdb.log"

    $debugger = Get-Phase6cX86Cdb
    if ($null -eq $debugger) {
        $result.outcome = "inconclusive-tool-unavailable"
        $diagnostics.Add(
            "x86 cdb.exe was not available from the preinstalled Windows debugger locations"
        )
        $result.diagnostics = $diagnostics.ToArray()
        return $result
    }

    $toolInfo = $debugger.VersionInfo
    $result.debugger = [ordered]@{
        name = "cdb.exe"
        architecture = "x86"
        path = [System.IO.Path]::GetFullPath($debugger.FullName)
        sha256 = Get-Phase6cFaultSha256 $debugger.FullName
        file_version = [string]$toolInfo.FileVersion
        product_version = [string]$toolInfo.ProductVersion
        provenance = "preinstalled-github-windows-runner-tool"
    }

    try {
        $modules = @(Get-Phase6cFaultModuleSnapshot $Process)
        Write-Phase6cFaultJson $modulePath ([ordered]@{
            schema_version = 1
            process_id = $Process.Id
            modules = $modules
        })
        $result.module_snapshot = [ordered]@{
            path = Get-Phase6cFaultRelativePath $ArtifactRoot $modulePath
            sha256 = Get-Phase6cFaultSha256 $modulePath
        }
    }
    catch {
        $result.outcome = "inconclusive-module-snapshot-failed"
        $diagnostics.Add($_.Exception.Message)
        $result.diagnostics = $diagnostics.ToArray()
        return $result
    }

    $commands = 'sxd av;g;.ecxr;.exr -1;r;kv;lm;q'
    $argumentList = @(
        "-p",
        [string]$Process.Id,
        "-g",
        "-G",
        "-pd",
        "-logo",
        ('"{0}"' -f $logPath),
        "-c",
        ('"{0}"' -f $commands)
    )

    try {
        $debuggerProcess = Start-Process -FilePath $debugger.FullName -ArgumentList $argumentList -WindowStyle Hidden -PassThru
    }
    catch {
        $result.outcome = "inconclusive-debugger-start-failed"
        $diagnostics.Add("cdb start failed: $($_.Exception.Message)")
        $result.diagnostics = $diagnostics.ToArray()
        return $result
    }

    $attached = $false
    for ($poll = 0; $poll -lt 50; $poll += 1) {
        Start-Sleep -Milliseconds 100
        $Process.Refresh()
        $debuggerProcess.Refresh()
        if ($Process.HasExited -or $debuggerProcess.HasExited) {
            break
        }
        if (Test-Path -LiteralPath $logPath -PathType Leaf) {
            if ((Get-Item -LiteralPath $logPath).Length -gt 0) {
                $attached = $true
                break
            }
        }
    }

    if (-not $attached) {
        $debuggerProcess.Refresh()
        $result.outcome = "inconclusive-debugger-attach-failed"
        $diagnostics.Add(
            "cdb did not remain attached with a non-empty log before the diagnostic render"
        )
        if (-not $debuggerProcess.HasExited) {
            try { Stop-Process -Id $debuggerProcess.Id -Force } catch { }
        }
        if (Test-Path -LiteralPath $logPath -PathType Leaf) {
            $result.debugger_log = [ordered]@{
                path = Get-Phase6cFaultRelativePath $ArtifactRoot $logPath
                sha256 = Get-Phase6cFaultSha256 $logPath
            }
        }
        $result.diagnostics = $diagnostics.ToArray()
        return $result
    }

    $renderAttempt = Invoke-Phase6cAudioRender $Process $ExpectedTitle $OutputPath
    $result.render_attempt = $renderAttempt

    $debuggerProcess.Refresh()
    if (-not $debuggerProcess.HasExited) {
        if (-not $debuggerProcess.WaitForExit(15000)) {
            $diagnostics.Add("cdb did not finish after the render attempt")
            try { Stop-Process -Id $debuggerProcess.Id -Force } catch { }
        }
    }

    if (-not (Test-Path -LiteralPath $logPath -PathType Leaf)) {
        $result.outcome = "inconclusive-debugger-log-missing"
        $diagnostics.Add("cdb produced no log")
        $result.diagnostics = $diagnostics.ToArray()
        return $result
    }

    $result.debugger_log = [ordered]@{
        path = Get-Phase6cFaultRelativePath $ArtifactRoot $logPath
        sha256 = Get-Phase6cFaultSha256 $logPath
    }

    $logText = Get-Content -LiteralPath $logPath -Raw
    $codeMatch = [regex]::Match(
        $logText,
        '(?im)(?:Access violation\s*-\s*code|ExceptionCode:)\s*(c0000005)'
    )
    $addressMatches = [regex]::Matches(
        $logText,
        '(?im)ExceptionAddress:\s*([0-9a-f' + [char]0x60 + ']+)'
    )
    $addressText = $null
    if ($addressMatches.Count -gt 0) {
        $addressText = $addressMatches[$addressMatches.Count - 1].Groups[1].Value
    } else {
        $registerMatches = [regex]::Matches(
            $logText,
            '(?im)\beip=([0-9a-f' + [char]0x60 + ']+)'
        )
        if ($registerMatches.Count -gt 0) {
            $addressText = $registerMatches[$registerMatches.Count - 1].Groups[1].Value
        }
    }

    $address = Convert-Phase6cFaultHex $addressText
    $moduleMatch = $null
    if ($null -ne $address) {
        foreach ($module in $modules) {
            $moduleBase = Convert-Phase6cFaultHex ([string]$module.base_address_hex)
            $moduleEnd = Convert-Phase6cFaultHex ([string]$module.end_address_hex)
            if ($null -ne $moduleBase -and $null -ne $moduleEnd -and
                $address -ge $moduleBase -and $address -lt $moduleEnd) {
                $moduleMatch = $module
                break
            }
        }
    }

    $exception = [ordered]@{
        expected_code_hex = "0xc0000005"
        observed_code_hex = if ($codeMatch.Success) {
            "0x" + $codeMatch.Groups[1].Value.ToLowerInvariant()
        } else {
            $null
        }
        process_exit_code = $renderAttempt.process_exit_code
        address_hex = if ($null -ne $address) {
            "0x{0:x8}" -f $address
        } else {
            $null
        }
        module = $null
        symbol_resolution = "not-required-for-module-offset"
    }
    if ($null -ne $moduleMatch -and $null -ne $address) {
        $moduleBase = Convert-Phase6cFaultHex ([string]$moduleMatch.base_address_hex)
        $exception.module = [ordered]@{
            name = [string]$moduleMatch.name
            path = [string]$moduleMatch.path
            sha256 = [string]$moduleMatch.sha256
            base_address_hex = [string]$moduleMatch.base_address_hex
            size_bytes = [int64]$moduleMatch.size_bytes
            offset_hex = "0x{0:x}" -f ($address - $moduleBase)
        }
    }
    $result.exception = $exception

    if (-not $codeMatch.Success) {
        $result.outcome = "inconclusive-exception-code-not-captured"
        $diagnostics.Add("cdb log did not bind the expected c0000005 exception")
    } elseif ($renderAttempt.process_exit_code -ne -1073741819) {
        $result.outcome = "inconclusive-exit-code-mismatch"
        $diagnostics.Add(
            "instrumented render did not retain the expected -1073741819 process exit"
        )
    } elseif ($null -eq $address) {
        $result.outcome = "inconclusive-exception-address-not-captured"
        $diagnostics.Add("cdb log did not expose an exception instruction address")
    } elseif ($null -eq $moduleMatch) {
        $result.outcome = "inconclusive-exception-module-unresolved"
        $diagnostics.Add(
            "exception address did not fall inside the pre-render loaded-module snapshot"
        )
    } else {
        $result.outcome = "captured-module-offset"
    }

    $result.diagnostics = $diagnostics.ToArray()
    return $result
}
