function Get-Phase6cTimingInteger([object]$Element) {
    try {
        $patternObject = $null
        if ($Element.TryGetCurrentPattern(
                [System.Windows.Automation.ValuePattern]::Pattern,
                [ref]$patternObject)) {
            $valuePattern = [System.Windows.Automation.ValuePattern]$patternObject
            $raw = [string]$valuePattern.Current.Value
            if ($raw -cmatch '^[0-9]+$') {
                return [int]$raw
            }
        }
    }
    catch { }

    try {
        $raw = ([string]$Element.Current.Name).Trim()
        if ($raw -cmatch '^[0-9]+$') {
            return [int]$raw
        }
    }
    catch { }
    return $null
}

function Test-Phase6cSameVisualElement([object]$Left, [object]$Right) {
    try {
        if ([System.Windows.Automation.Automation]::Compare($Left, $Right)) {
            return $true
        }
    }
    catch { }

    try {
        $leftBounds = $Left.Current.BoundingRectangle
        $rightBounds = $Right.Current.BoundingRectangle
        if ($leftBounds.IsEmpty -or $rightBounds.IsEmpty) {
            return $false
        }
        return (
            [Math]::Abs($leftBounds.X - $rightBounds.X) -lt 0.5 -and
            [Math]::Abs($leftBounds.Y - $rightBounds.Y) -lt 0.5 -and
            [Math]::Abs($leftBounds.Width - $rightBounds.Width) -lt 0.5 -and
            [Math]::Abs($leftBounds.Height - $rightBounds.Height) -lt 0.5
        )
    }
    catch {
        return $false
    }
}

function Find-Phase6cSongInformationDialogs(
    [System.Diagnostics.Process]$Process,
    [System.Windows.Automation.AutomationElement]$Desktop
) {
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $matches = [System.Collections.Generic.List[object]]::new()
    $requiredLabels = @(
        "Tempo",
        "Lines per beat",
        "Ticks per beat",
        "Extra tick per line",
        "Real tempo",
        "Real ticks per beat"
    )

    try {
        $processCondition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty,
            $Process.Id
        )
        $windows = $Desktop.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            $processCondition
        )
        $walker = [System.Windows.Automation.TreeWalker]::ControlViewWalker

        foreach ($window in $windows) {
            $nodes = [System.Collections.Generic.List[object]]::new()
            $nodes.Add($window)
            try {
                foreach ($node in $window.FindAll(
                    [System.Windows.Automation.TreeScope]::Descendants,
                    [System.Windows.Automation.Condition]::TrueCondition
                )) {
                    $nodes.Add($node)
                }
            }
            catch {
                $diagnostics.Add(
                    "timing dialog finder traversal failed: $($_.Exception.Message)"
                )
                continue
            }

            foreach ($node in $nodes) {
                $name = ""
                try { $name = [string]$node.Current.Name }
                catch {
                    $diagnostics.Add(
                        "timing dialog finder element read failed: $($_.Exception.Message)"
                    )
                    continue
                }
                if ($name -cne "Song Information") {
                    continue
                }

                $scope = $node
                for ($depth = 0; $depth -lt 8 -and $null -ne $scope; $depth += 1) {
                    $names = [System.Collections.Generic.HashSet[string]]::new(
                        [System.StringComparer]::Ordinal
                    )
                    try {
                        $scopeName = [string]$scope.Current.Name
                        if (-not [string]::IsNullOrWhiteSpace($scopeName)) {
                            [void]$names.Add($scopeName)
                        }
                        foreach ($descendant in $scope.FindAll(
                            [System.Windows.Automation.TreeScope]::Descendants,
                            [System.Windows.Automation.Condition]::TrueCondition
                        )) {
                            try {
                                $descendantName = [string]$descendant.Current.Name
                                if (-not [string]::IsNullOrWhiteSpace($descendantName)) {
                                    [void]$names.Add($descendantName)
                                }
                            }
                            catch { }
                        }
                    }
                    catch {
                        break
                    }

                    $hasAllLabels = $true
                    foreach ($label in $requiredLabels) {
                        if (-not $names.Contains($label)) {
                            $hasAllLabels = $false
                            break
                        }
                    }
                    if ($hasAllLabels) {
                        $duplicate = $false
                        foreach ($existing in $matches) {
                            if ([System.Windows.Automation.Automation]::Compare(
                                    $existing, $scope)) {
                                $duplicate = $true
                                break
                            }
                        }
                        if (-not $duplicate) {
                            $matches.Add($scope)
                        }
                        break
                    }

                    $parent = $null
                    try { $parent = $walker.GetParent($scope) }
                    catch { break }
                    if ($null -eq $parent) {
                        break
                    }
                    try {
                        if ([int]$parent.Current.ProcessId -ne $Process.Id) {
                            break
                        }
                    }
                    catch { break }
                    $scope = $parent
                }
            }
        }
    }
    catch {
        $diagnostics.Add(
            "timing dialog finder UI Automation failed: $($_.Exception.Message)"
        )
    }

    return [ordered]@{
        matches = $matches.ToArray()
        diagnostics = @($diagnostics | Select-Object -Unique)
    }
}

function Open-Phase6cSongInformationDialog([System.Diagnostics.Process]$Process) {
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $result = [ordered]@{
        opened = $false
        dialog_seen_before = $false
        file_menu_invoked = $false
        menu_item_seen = $false
        attempted = $false
        outcome = "not-opened"
        diagnostics = @()
    }

    try {
        $Process.Refresh()
        if ($Process.HasExited) {
            $result.outcome = "process-exited"
            return $result
        }

        $desktop = [System.Windows.Automation.AutomationElement]::RootElement
        if ($null -eq $desktop) {
            $diagnostics.Add("timing dialog bootstrap UI Automation returned no desktop root element")
            $result.outcome = "desktop-root-missing"
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }
        $processCondition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty,
            $Process.Id
        )

        $findDialogs = {
            $found = Find-Phase6cSongInformationDialogs $Process $desktop
            foreach ($diagnostic in @($found.diagnostics)) {
                if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                    $diagnostics.Add([string]$diagnostic)
                }
            }
            return @($found.matches)
        }

        $findMenuItems = {
            param([string]$ExactName)
            $found = [System.Collections.Generic.List[object]]::new()
            $windows = $desktop.FindAll(
                [System.Windows.Automation.TreeScope]::Children,
                $processCondition
            )
            foreach ($window in $windows) {
                try {
                    $nodes = $window.FindAll(
                        [System.Windows.Automation.TreeScope]::Descendants,
                        [System.Windows.Automation.Condition]::TrueCondition
                    )
                    foreach ($node in $nodes) {
                        try {
                            if (
                                $node.Current.ControlType -eq
                                    [System.Windows.Automation.ControlType]::MenuItem -and
                                [string]$node.Current.Name -ceq $ExactName
                            ) {
                                $found.Add($node)
                            }
                        }
                        catch {
                            $diagnostics.Add(
                                "timing dialog bootstrap menu-item read failed: $($_.Exception.Message)"
                            )
                        }
                    }
                }
                catch {
                    $diagnostics.Add(
                        "timing dialog bootstrap window traversal failed: $($_.Exception.Message)"
                    )
                }
            }
            return $found.ToArray()
        }

        $dialogs = @(& $findDialogs)
        if ($diagnostics.Count -gt 0) {
            $result.outcome = "automation-failed"
            $result.diagnostics = @($diagnostics | Select-Object -Unique)
            return $result
        }
        if ($dialogs.Count -gt 1) {
            $diagnostics.Add(
                "timing dialog bootstrap expected at most one Song Information window: count=$($dialogs.Count)"
            )
            $result.outcome = "dialog-ambiguous"
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }
        if ($dialogs.Count -eq 1) {
            $result.dialog_seen_before = $true
            $result.opened = $true
            $result.outcome = "already-open"
            return $result
        }

        # Psycle 1.12 exposes the timing dialog as File > Song Properties and
        # titles the resulting process-owned window "Song Information".
        $songPropertyItems = @(& $findMenuItems "Song Properties")
        if ($diagnostics.Count -gt 0) {
            $result.outcome = "automation-failed"
            $result.diagnostics = @($diagnostics | Select-Object -Unique)
            return $result
        }

        if ($songPropertyItems.Count -eq 0) {
            $fileItems = @(& $findMenuItems "File")
            if ($diagnostics.Count -gt 0) {
                $result.outcome = "automation-failed"
                $result.diagnostics = @($diagnostics | Select-Object -Unique)
                return $result
            }
            if ($fileItems.Count -ne 1) {
                $diagnostics.Add(
                    "timing dialog bootstrap expected exactly one File menu item: count=$($fileItems.Count)"
                )
                $result.outcome = "file-menu-ambiguous"
                $result.diagnostics = $diagnostics.ToArray()
                return $result
            }
            if (-not $fileItems[0].Current.IsEnabled) {
                $diagnostics.Add("timing dialog bootstrap File menu item is disabled")
                $result.outcome = "file-menu-disabled"
                $result.diagnostics = $diagnostics.ToArray()
                return $result
            }

            $filePatternObject = $null
            $result.attempted = $true
            if ($fileItems[0].TryGetCurrentPattern(
                    [System.Windows.Automation.ExpandCollapsePattern]::Pattern,
                    [ref]$filePatternObject)) {
                ([System.Windows.Automation.ExpandCollapsePattern]$filePatternObject).Expand()
            } elseif ($fileItems[0].TryGetCurrentPattern(
                    [System.Windows.Automation.InvokePattern]::Pattern,
                    [ref]$filePatternObject)) {
                ([System.Windows.Automation.InvokePattern]$filePatternObject).Invoke()
            } else {
                $diagnostics.Add(
                    "timing dialog bootstrap File menu item has neither ExpandCollapsePattern nor InvokePattern"
                )
                $result.outcome = "file-menu-not-openable"
                $result.diagnostics = $diagnostics.ToArray()
                return $result
            }
            $result.file_menu_invoked = $true
            Start-Sleep -Milliseconds 150
            $songPropertyItems = @(& $findMenuItems "Song Properties")
        }

        if ($diagnostics.Count -gt 0) {
            $result.outcome = "automation-failed"
            $result.diagnostics = @($diagnostics | Select-Object -Unique)
            return $result
        }
        if ($songPropertyItems.Count -ne 1) {
            $diagnostics.Add(
                "timing dialog bootstrap expected exactly one Song Properties menu item: count=$($songPropertyItems.Count)"
            )
            $result.outcome = "song-properties-menu-ambiguous"
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }

        $result.menu_item_seen = $true
        if (-not $songPropertyItems[0].Current.IsEnabled) {
            $diagnostics.Add("timing dialog bootstrap Song Properties menu item is disabled")
            $result.outcome = "song-properties-menu-disabled"
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }
        $invokeObject = $null
        if (-not $songPropertyItems[0].TryGetCurrentPattern(
                [System.Windows.Automation.InvokePattern]::Pattern,
                [ref]$invokeObject)) {
            $diagnostics.Add("timing dialog bootstrap Song Properties menu item has no InvokePattern")
            $result.outcome = "song-properties-menu-not-invokable"
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }

        $result.attempted = $true
        ([System.Windows.Automation.InvokePattern]$invokeObject).Invoke()

        for ($attempt = 0; $attempt -lt 20; $attempt += 1) {
            Start-Sleep -Milliseconds 100
            $Process.Refresh()
            if ($Process.HasExited) {
                $diagnostics.Add("timing dialog bootstrap process exited while opening Song Information")
                $result.outcome = "process-exited"
                break
            }
            $dialogs = @(& $findDialogs)
            if ($diagnostics.Count -gt 0) {
                $result.outcome = "automation-failed"
                break
            }
            if ($dialogs.Count -gt 1) {
                $diagnostics.Add(
                    "timing dialog bootstrap opened multiple Song Information windows: count=$($dialogs.Count)"
                )
                $result.outcome = "dialog-ambiguous"
                break
            }
            if ($dialogs.Count -eq 1) {
                $result.opened = $true
                $result.outcome = "opened"
                break
            }
        }

        if (-not $result.opened -and $diagnostics.Count -eq 0) {
            $diagnostics.Add("timing dialog bootstrap timed out waiting for Song Information")
            $result.outcome = "open-timeout"
        }
    }
    catch {
        $diagnostics.Add("timing dialog bootstrap UI Automation failed: $($_.Exception.Message)")
        $result.outcome = "automation-failed"
    }

    $result.diagnostics = @($diagnostics | Select-Object -Unique)
    return $result
}

function Get-Phase6cTimingUiObservation([System.Diagnostics.Process]$Process) {
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $result = [ordered]@{
        window_seen = $false
        complete = $false
        values = [ordered]@{}
        control_ids = [ordered]@{}
        diagnostics = @()
    }
    # Psycle 1.12 CSongpDlg binds these fields to fixed Win32 resource IDs
    # in SongpDlg.cpp / resources.hpp. Win32 UI Automation exposes those native
    # control IDs as AutomationId, which is stronger than duplicated label text.
    $controlIds = [ordered]@{
        tempo = '2146'                # IDC_EDIT_TEMPO
        lines_per_beat = '2148'       # IDC_EDIT_LPB
        ticks_per_beat = '2150'       # IDC_EDIT_TPB
        extra_tick_per_line = '2151'  # IDC_EDIT_EXTRATICK
        real_ticks_per_beat = '2153'  # IDC_EDIT_REALTPB
        real_tempo = '2154'           # IDC_EDIT_REALTEMPO
    }
    $result.control_ids = $controlIds

    try {
        $Process.Refresh()
        if ($Process.HasExited) {
            return $result
        }
        $desktop = [System.Windows.Automation.AutomationElement]::RootElement
        if ($null -eq $desktop) {
            $diagnostics.Add('timing observer UI Automation returned no desktop root element')
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }
        $dialogSearch = Find-Phase6cSongInformationDialogs $Process $desktop
        foreach ($diagnostic in @($dialogSearch.diagnostics)) {
            if (-not [string]::IsNullOrWhiteSpace([string]$diagnostic)) {
                $diagnostics.Add([string]$diagnostic)
            }
        }
        $matches = @($dialogSearch.matches)
        if ($diagnostics.Count -gt 0) {
            $result.diagnostics = @($diagnostics | Select-Object -Unique)
            return $result
        }
        if ($matches.Count -eq 0) {
            return $result
        }
        $result.window_seen = $true
        if ($matches.Count -ne 1) {
            $diagnostics.Add("timing observer expected exactly one Song Information scope: count=$($matches.Count)")
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }

        $nodes = $matches[0].FindAll(
            [System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition
        )
        foreach ($key in $controlIds.Keys) {
            $matchesById = [System.Collections.Generic.List[object]]::new()
            foreach ($node in $nodes) {
                try {
                    if (
                        [string]$node.Current.AutomationId -ceq [string]$controlIds[$key] -and
                        $node.Current.ControlType -eq
                            [System.Windows.Automation.ControlType]::Edit
                    ) {
                        $duplicateVisual = $false
                        foreach ($existing in $matchesById) {
                            if (Test-Phase6cSameVisualElement $existing $node) {
                                $duplicateVisual = $true
                                break
                            }
                        }
                        if (-not $duplicateVisual) {
                            $matchesById.Add($node)
                        }
                    }
                }
                catch {
                    $diagnostics.Add(
                        "timing observer control-id '$($controlIds[$key])' read failed: $($_.Exception.Message)"
                    )
                }
            }

            if ($matchesById.Count -ne 1) {
                $diagnostics.Add(
                    "timing observer expected exactly one Edit control for '$key' " +
                    "AutomationId=$($controlIds[$key]): count=$($matchesById.Count)"
                )
                continue
            }

            $value = Get-Phase6cTimingInteger $matchesById[0]
            if ($null -eq $value) {
                $diagnostics.Add(
                    "timing observer control '$key' AutomationId=$($controlIds[$key]) " +
                    "does not expose an integer value"
                )
                continue
            }
            $result.values[$key] = [int]$value
        }
        if ($diagnostics.Count -eq 0 -and $result.values.Count -eq $controlIds.Count) {
            $result.complete = $true
        }
    }
    catch {
        $diagnostics.Add("timing observer UI Automation failed: $($_.Exception.Message)")
    }
    $result.diagnostics = @($diagnostics | Select-Object -Unique)
    return $result
}
