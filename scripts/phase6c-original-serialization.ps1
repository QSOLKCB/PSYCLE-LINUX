# Project-authored observation helper. Loaded by the pinned original observer.
# No keyboard injection, guessed command IDs, or overwrite confirmation.
Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class Phase6cSaveMenu {
    private delegate bool EnumProc(IntPtr window, IntPtr ignored);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumProc callback, IntPtr ignored);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll")] private static extern IntPtr GetMenu(IntPtr window);
    [DllImport("user32.dll")] private static extern IntPtr GetSubMenu(IntPtr menu, int position);
    [DllImport("user32.dll")] private static extern int GetMenuItemCount(IntPtr menu);
    [DllImport("user32.dll")] private static extern uint GetMenuItemID(IntPtr menu, int position);
    [DllImport("user32.dll")] private static extern uint GetMenuState(IntPtr menu, uint position, uint flags);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetMenuString(IntPtr menu, uint position, StringBuilder text, int count, uint flags);
    [DllImport("user32.dll", SetLastError=true)] private static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    public class Command {
        public long Window;
        public uint Process;
        public string WindowTitle;
        public string Menu;
        public string Label;
        public uint Id;
        public bool Enabled;
    }
    static string Label(IntPtr menu, int position) {
        var text = new StringBuilder(1024);
        if (GetMenuString(menu, (uint)position, text, text.Capacity, 0x400) == 0) return "";
        return text.ToString();
    }
    static string Plain(string text) { return text.Split('\t')[0].Replace("&", "").Trim(); }
    public static Command[] Inspect(uint process, string title) {
        var result = new List<Command>();
        EnumWindows(delegate(IntPtr window, IntPtr ignored) {
            uint owner; GetWindowThreadProcessId(window, out owner);
            var text = new StringBuilder(1024); GetWindowText(window,text,text.Capacity);
            if (owner != process || text.ToString() != title) return true;
            var menu = GetMenu(window);
            for (int i=0; i<GetMenuItemCount(menu); ++i) {
                if (Plain(Label(menu,i)) != "File") continue;
                var fileMenu=GetSubMenu(menu,i);
                for (int j=0; j<GetMenuItemCount(fileMenu); ++j) {
                    uint state=GetMenuState(fileMenu,(uint)j,0x400);
                    result.Add(new Command {Window=window.ToInt64(), Process=owner, WindowTitle=title,
                        Menu="File",Label=Label(fileMenu,j),Id=GetMenuItemID(fileMenu,j),
                        Enabled=state != 0xffffffff && (state & 3)==0});
                }
            }
            return true;
        },IntPtr.Zero);
        return result.ToArray();
    }
    public static bool Invoke(uint process, string title, Command expected) {
        // Re-enumerate immediately before dispatch, retaining PID/title/menu/ID binding.
        var matches=new List<Command>();
        foreach(var item in Inspect(process,title)) {
            string label=Plain(item.Label);
            if (label=="Save as..." || label=="Save As..." || label=="Save As" || label=="Save As\u2026") matches.Add(item);
        }
        if(matches.Count!=1) return false;
        var command=matches[0];
        if(!command.Enabled || command.Id==0 || command.Id==0xffffffff ||
           command.Window!=expected.Window || command.Id!=expected.Id || command.Label!=expected.Label) return false;
        return PostMessage(new IntPtr(command.Window),0x0111,new IntPtr(command.Id),IntPtr.Zero);
    }
}
"@

function Invoke-Phase6cSaveAs(
    [System.Diagnostics.Process]$Process,
    [string]$ExpectedTitle,
    [string]$OutputPath
) {
    $result = [ordered]@{
        schema_version = 1
        process_id = $Process.Id
        outcome = "inconclusive"
        command_verified = $false
        command_dispatched = $false
        dialog_verified = $false
        path_set = $false
        save_invoked = $false
        dialog_closed = $false
        stable_output_polls = 0
        menu_inventory = @()
        dialog_inventory = @()
        diagnostics = @()
    }
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    try {
        if (Test-Path -LiteralPath $OutputPath) { throw "serialization output already exists" }
        New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($OutputPath)) -Force | Out-Null
        $Process.Refresh()
        if ($Process.HasExited) { throw "reference exited before save observation" }
        if ([string]::IsNullOrWhiteSpace($ExpectedTitle)) { throw "missing verified loaded-fixture window title" }
        $commands = @([Phase6cSaveMenu]::Inspect([uint32]$Process.Id, $ExpectedTitle))
        $result.menu_inventory = @($commands | ForEach-Object {
            [ordered]@{ title=$_.WindowTitle; menu=$_.Menu; label=$_.Label; id=$_.Id; enabled=$_.Enabled }
        })
        $matches = @($commands | Where-Object {
            ($_.Label.Split("`t")[0].Replace("&", "").Trim()) -cin @("Save As...", "Save As", "Save As…", "Save as...")
        })
        if ($matches.Count -ne 1 -or -not $matches[0].Enabled) { throw "Save As menu signature missing, disabled or ambiguous" }
        $result.command_verified = $true
        if (-not [Phase6cSaveMenu]::Invoke([uint32]$Process.Id, $ExpectedTitle, $matches[0])) {
            throw "verified Save As command dispatch failed"
        }
        $result.command_dispatched = $true
        $condition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $Process.Id)
        $dialog = $null
        for ($poll=0; $poll -lt 30; $poll++) {
            $Process.Refresh()
            if ($Process.HasExited) { throw "reference exited waiting for Save As" }
            $windows = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
                [System.Windows.Automation.TreeScope]::Children, $condition)
            $dialogs = @($windows | Where-Object { $_.Current.Name -ceq "Save As" })
            $result.dialog_inventory = @($windows | ForEach-Object { $_.Current.Name })
            if ($dialogs.Count -gt 1) { throw "ambiguous process-owned Save As dialogs" }
            if ($dialogs.Count -eq 1) { $dialog=$dialogs[0]; break }
            Start-Sleep -Milliseconds 200
        }
        if ($null -eq $dialog) { throw "Save As dialog not observed" }
        $nodes = $dialog.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition)
        $result.dialog_inventory = @($nodes | ForEach-Object {
            [ordered]@{ name=$_.Current.Name; id=$_.Current.AutomationId; type=$_.Current.ControlType.ProgrammaticName; enabled=$_.Current.IsEnabled }
        })
        $edits = @($nodes | Where-Object {
            $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Edit -and
            $_.Current.AutomationId -cin @("1001", "1148") -and $_.Current.IsEnabled
        })
        $buttons = @($nodes | Where-Object {
            $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and
            $_.Current.Name.Replace("&", "") -ceq "Save" -and $_.Current.IsEnabled
        })
        if ($edits.Count -ne 1 -or $buttons.Count -ne 1) { throw "Save As file-name/Save signature missing or ambiguous" }
        $value = $edits[0].GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
        $invoke = $buttons[0].GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
        if ($value.Current.IsReadOnly) { throw "Save As filename is read-only" }
        $result.dialog_verified = $true
        $value.SetValue($OutputPath)
        if ($value.Current.Value -cne $OutputPath) { throw "Save As path verification failed" }
        $result.path_set = $true
        $invoke.Invoke()
        $result.save_invoked = $true
        $previousHash = $null
        for ($poll=0; $poll -lt 40; $poll++) {
            Start-Sleep -Milliseconds 250
            $Process.Refresh()
            if ($Process.HasExited) { throw "reference exited during save" }
            $scan = Get-UiObservation $Process
            if ($scan.diagnostics.Count -gt 0) { throw ($scan.diagnostics -join "; ") }
            $result.dialog_closed = -not (@($scan.values | Where-Object { $_ -ceq "Save As" }).Count)
            $assessment = Get-FixtureUiAssessment $scan.values @([System.IO.Path]::GetFileName($OutputPath))
            if ($assessment.application_error_marker) { throw "application error during save: $($assessment.application_error_marker)" }
            if ($result.dialog_closed -and (Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
                $bytes=[System.IO.File]::ReadAllBytes($OutputPath)
                if ($bytes.Length -ge 8 -and [System.Text.Encoding]::ASCII.GetString($bytes,0,8) -ceq "PSY3SONG") {
                    $hash=Get-Sha256 $OutputPath
                    if ($hash -eq $previousHash) { $result.stable_output_polls++ } else { $result.stable_output_polls=1 }
                    $previousHash=$hash
                } else { $result.stable_output_polls=0 }
            } else { $result.stable_output_polls=0 }
        }
        if (-not $result.dialog_closed -or $result.stable_output_polls -lt 4) { throw "no stable PSY3 save output after dialog dismissal" }
        $result.outcome = "saved"
    } catch {
        $diagnostics.Add($_.Exception.Message)
    }
    $result.diagnostics = @($diagnostics.ToArray())
    return $result
}
