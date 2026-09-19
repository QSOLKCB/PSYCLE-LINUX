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

function Get-Phase6cTimingUiObservation([System.Diagnostics.Process]$Process) {
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $result = [ordered]@{
        window_seen = $false
        complete = $false
        values = [ordered]@{}
        diagnostics = @()
    }
    $labels = [ordered]@{
        tempo = 'Tempo'
        lines_per_beat = 'Lines per beat'
        ticks_per_beat = 'Ticks per beat'
        extra_tick_per_line = 'Extra tick per line'
        real_tempo = 'Real tempo'
        real_ticks_per_beat = 'Real ticks per beat'
    }

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
        $processCondition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty,
            $Process.Id
        )
        $windows = $desktop.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            $processCondition
        )
        $matches = [System.Collections.Generic.List[object]]::new()
        foreach ($window in $windows) {
            try {
                if ([string]$window.Current.Name -ceq 'Song Information') {
                    $matches.Add($window)
                }
            }
            catch {
                $diagnostics.Add("timing observer top-level window read failed: $($_.Exception.Message)")
            }
        }
        if ($matches.Count -eq 0) {
            return $result
        }
        $result.window_seen = $true
        if ($matches.Count -ne 1) {
            $diagnostics.Add("timing observer expected exactly one Song Information window: count=$($matches.Count)")
            $result.diagnostics = $diagnostics.ToArray()
            return $result
        }

        $nodes = $matches[0].FindAll(
            [System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition
        )
        $labelElements = @{}
        foreach ($key in $labels.Keys) {
            $found = [System.Collections.Generic.List[object]]::new()
            foreach ($node in $nodes) {
                try {
                    if ([string]$node.Current.Name -ceq [string]$labels[$key]) {
                        $found.Add($node)
                    }
                }
                catch {
                    $diagnostics.Add("timing observer label read failed: $($_.Exception.Message)")
                }
            }
            if ($found.Count -ne 1) {
                $diagnostics.Add("timing observer label '$($labels[$key])' count=$($found.Count)")
            } else {
                $labelElements[$key] = $found[0]
            }
        }
        if ($diagnostics.Count -gt 0) {
            $result.diagnostics = @($diagnostics | Select-Object -Unique)
            return $result
        }

        $used = [System.Collections.Generic.List[object]]::new()
        foreach ($key in $labels.Keys) {
            $label = $labelElements[$key]
            $labelBounds = $label.Current.BoundingRectangle
            $candidates = [System.Collections.Generic.List[object]]::new()
            foreach ($node in $nodes) {
                $value = Get-Phase6cTimingInteger $node
                if ($null -eq $value) {
                    continue
                }
                $alreadyUsed = $false
                foreach ($usedNode in $used) {
                    if ([System.Windows.Automation.Automation]::Compare($usedNode, $node)) {
                        $alreadyUsed = $true
                        break
                    }
                }
                if ($alreadyUsed) {
                    continue
                }

                $boundByLabel = $false
                try {
                    $labeledBy = $node.Current.LabeledBy
                    if ($null -ne $labeledBy -and
                        [System.Windows.Automation.Automation]::Compare($labeledBy, $label)) {
                        $boundByLabel = $true
                    }
                }
                catch { }

                $bounds = $node.Current.BoundingRectangle
                if ($bounds.IsEmpty -or $bounds.Width -le 0 -or $bounds.Height -le 0) {
                    continue
                }
                $labelCenter = $labelBounds.X + ($labelBounds.Width / 2.0)
                $valueCenter = $bounds.X + ($bounds.Width / 2.0)
                $verticalGap = $bounds.Y - ($labelBounds.Y + $labelBounds.Height)
                $columnMatch = (
                    $verticalGap -ge -3 -and $verticalGap -le 35 -and
                    [Math]::Abs($valueCenter - $labelCenter) -le
                        [Math]::Max(35.0, ($labelBounds.Width + $bounds.Width) / 2.0)
                )
                if ($boundByLabel -or $columnMatch) {
                    $candidates.Add([ordered]@{
                        element = $node
                        value = $value
                        rank = if ($boundByLabel) { -100000.0 } else {
                            [Math]::Abs($verticalGap) + [Math]::Abs($valueCenter - $labelCenter)
                        }
                    })
                }
            }
            $orderedCandidates = @($candidates | Sort-Object rank)
            if ($orderedCandidates.Count -eq 0) {
                $diagnostics.Add("timing observer found no numeric control for '$($labels[$key])'")
                continue
            }
            if ($orderedCandidates.Count -gt 1 -and
                [Math]::Abs([double]$orderedCandidates[0].rank - [double]$orderedCandidates[1].rank) -lt 0.001) {
                $diagnostics.Add("timing observer numeric control for '$($labels[$key])' is ambiguous")
                continue
            }
            $selected = $orderedCandidates[0]
            $used.Add($selected.element)
            $result.values[$key] = [int]$selected.value
        }
        if ($diagnostics.Count -eq 0 -and $result.values.Count -eq $labels.Count) {
            $result.complete = $true
        }
    }
    catch {
        $diagnostics.Add("timing observer UI Automation failed: $($_.Exception.Message)")
    }
    $result.diagnostics = @($diagnostics | Select-Object -Unique)
    return $result
}
