# Project-authored helper for the Phase 6C original-Psycle offline-render witness.
# It invokes only source-pinned Psycle 1.12.0 UI identities and returns evidence.
# It does not classify behavioral parity.

$phase6cAutomationReferences = @(
    [System.Windows.Automation.AutomationElement].Assembly.Location
    [System.Windows.Automation.ControlType].Assembly.Location
) | Select-Object -Unique

Add-Type -ReferencedAssemblies $phase6cAutomationReferences -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Automation;

public sealed class Phase6cRenderWindowOpenedObserver : IDisposable
{
    private const uint EVENT_OBJECT_SHOW = 0x8002;
    private const int OBJID_WINDOW = 0;
    private const int CHILDID_SELF = 0;
    private const uint WINEVENT_OUTOFCONTEXT = 0x0000;
    private const uint WM_QUIT = 0x0012;
    private const uint WM_APP_FLUSH = 0x8031;
    private const uint PM_NOREMOVE = 0x0000;
    private const uint GA_ROOT = 2;

    [StructLayout(LayoutKind.Sequential)]
    private struct POINT
    {
        public int X;
        public int Y;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct MSG
    {
        public IntPtr hwnd;
        public uint message;
        public IntPtr wParam;
        public IntPtr lParam;
        public uint time;
        public POINT point;
        public uint lPrivate;
    }

    private delegate void WinEventDelegate(
        IntPtr hook,
        uint eventType,
        IntPtr window,
        int objectId,
        int childId,
        uint eventThread,
        uint eventTime
    );

    [DllImport("user32.dll", SetLastError=true)]
    private static extern IntPtr SetWinEventHook(
        uint eventMin,
        uint eventMax,
        IntPtr module,
        WinEventDelegate callback,
        uint processId,
        uint threadId,
        uint flags
    );

    [DllImport("user32.dll", SetLastError=true)]
    private static extern bool UnhookWinEvent(IntPtr hook);

    [DllImport("user32.dll", SetLastError=true)]
    private static extern int GetMessage(
        out MSG message,
        IntPtr window,
        uint minFilter,
        uint maxFilter
    );

    [DllImport("user32.dll", SetLastError=true)]
    private static extern bool PeekMessage(
        out MSG message,
        IntPtr window,
        uint minFilter,
        uint maxFilter,
        uint removeMessage
    );

    [DllImport("user32.dll")]
    private static extern bool TranslateMessage(ref MSG message);

    [DllImport("user32.dll")]
    private static extern IntPtr DispatchMessage(ref MSG message);

    [DllImport("user32.dll", SetLastError=true)]
    private static extern bool PostThreadMessage(
        uint threadId,
        uint message,
        IntPtr wParam,
        IntPtr lParam
    );

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(
        IntPtr window,
        out uint processId
    );

    [DllImport("user32.dll")]
    private static extern IntPtr GetAncestor(IntPtr window, uint flags);

    [DllImport("user32.dll", CharSet=CharSet.Unicode)]
    private static extern int GetWindowText(
        IntPtr window,
        StringBuilder text,
        int count
    );

    [DllImport("kernel32.dll")]
    private static extern uint GetTickCount();

    [DllImport("kernel32.dll")]
    private static extern uint GetCurrentThreadId();

    private readonly uint processId;
    private readonly object gate = new object();
    private readonly Queue<IntPtr> opened = new Queue<IntPtr>();
    private readonly System.Threading.ManualResetEventSlim ready =
        new System.Threading.ManualResetEventSlim(false);
    private readonly System.Threading.ManualResetEventSlim stopped =
        new System.Threading.ManualResetEventSlim(false);
    private readonly System.Threading.ManualResetEventSlim flushed =
        new System.Threading.ManualResetEventSlim(false);
    private readonly System.Threading.Thread pumpThread;

    private WinEventDelegate handler;
    private IntPtr hook;
    private uint pumpThreadId;
    private uint dispatchBoundaryTick;
    private int postDispatchEventCount;
    private int postDispatchObservedWindowEventCount;
    private int unresolvedPostDispatchEventCount;
    private int hookError;
    private bool dispatchBoundarySet;
    private bool disposed;

    public Phase6cRenderWindowOpenedObserver(uint processId)
    {
        this.processId = processId;
        pumpThread = new System.Threading.Thread(Pump);
        pumpThread.IsBackground = true;
        pumpThread.Name = "Phase6cRenderWinEventPump";
        pumpThread.Start();

        if (!ready.Wait(5000))
            throw new InvalidOperationException(
                "render WinEvent message pump did not initialize"
            );
        if (hook == IntPtr.Zero)
            throw new InvalidOperationException(
                "could not install process-filtered EVENT_OBJECT_SHOW hook: " +
                hookError
            );
    }

    private void Pump()
    {
        pumpThreadId = GetCurrentThreadId();

        // PostThreadMessage fails until the target thread owns a message queue.
        // Force queue creation before publishing readiness to the pwsh thread.
        MSG bootstrapMessage;
        PeekMessage(
            out bootstrapMessage,
            IntPtr.Zero,
            0,
            0,
            PM_NOREMOVE
        );

        handler = new WinEventDelegate(OnWinEvent);
        hook = SetWinEventHook(
            EVENT_OBJECT_SHOW,
            EVENT_OBJECT_SHOW,
            IntPtr.Zero,
            handler,
            processId,
            0,
            WINEVENT_OUTOFCONTEXT
        );
        if (hook == IntPtr.Zero)
            hookError = Marshal.GetLastWin32Error();
        ready.Set();

        if (hook == IntPtr.Zero)
        {
            stopped.Set();
            return;
        }

        try
        {
            MSG message;
            int status;
            while ((status = GetMessage(
                out message,
                IntPtr.Zero,
                0,
                0
            )) > 0)
            {
                if (message.message == WM_APP_FLUSH)
                {
                    flushed.Set();
                    continue;
                }
                TranslateMessage(ref message);
                DispatchMessage(ref message);
            }
            if (status < 0)
            {
                lock (gate)
                {
                    unresolvedPostDispatchEventCount += 1;
                }
            }
        }
        finally
        {
            IntPtr current = hook;
            hook = IntPtr.Zero;
            if (current != IntPtr.Zero)
                UnhookWinEvent(current);
            stopped.Set();
        }
    }

    private static bool TickStrictlyAfter(uint value, uint boundary)
    {
        // Equal millisecond timestamps are intentionally inconclusive:
        // they cannot prove which side of the dispatch boundary occurred first.
        return unchecked((int)(value - boundary)) > 0;
    }

    private void OnWinEvent(
        IntPtr ignoredHook,
        uint eventType,
        IntPtr window,
        int objectId,
        int childId,
        uint ignoredThread,
        uint eventTime
    )
    {
        if (eventType != EVENT_OBJECT_SHOW ||
            objectId != OBJID_WINDOW ||
            childId != CHILDID_SELF ||
            window == IntPtr.Zero)
            return;

        lock (gate)
        {
            if (disposed ||
                !dispatchBoundarySet ||
                !TickStrictlyAfter(eventTime, dispatchBoundaryTick))
                return;

            // Count first. The process-filtered WinEvent hook already bound this
            // event to the observed Psycle process at generation time. A HWND
            // may be gone or reused before this out-of-context callback runs;
            // liveness must not erase evidence that another window was shown.
            postDispatchObservedWindowEventCount += 1;
        }

        uint owner;
        if (GetWindowThreadProcessId(window, out owner) == 0 ||
            owner != processId)
        {
            lock (gate)
            {
                unresolvedPostDispatchEventCount += 1;
            }
            return;
        }

        IntPtr root = GetAncestor(window, GA_ROOT);
        if (root == IntPtr.Zero)
        {
            lock (gate)
            {
                unresolvedPostDispatchEventCount += 1;
            }
            return;
        }
        if (root != window)
            return;

        var title = new StringBuilder(256);
        GetWindowText(window, title, title.Capacity);
        if (!string.Equals(
            title.ToString(),
            "Render as Wav File",
            StringComparison.Ordinal
        ))
            return;

        lock (gate)
        {
            if (disposed)
                return;
            opened.Enqueue(window);
            postDispatchEventCount += 1;
        }
    }

    private void FlushPump()
    {
        if (pumpThreadId == 0 ||
            GetCurrentThreadId() == pumpThreadId ||
            disposed)
            return;

        flushed.Reset();
        if (!PostThreadMessage(
            pumpThreadId,
            WM_APP_FLUSH,
            IntPtr.Zero,
            IntPtr.Zero
        ))
            throw new InvalidOperationException(
                "could not post render WinEvent flush message"
            );
        if (!flushed.Wait(2000))
            throw new InvalidOperationException(
                "render WinEvent message pump flush timed out"
            );
    }

    public void BeginDispatchBoundary()
    {
        FlushPump();
        lock (gate)
        {
            if (disposed)
                throw new ObjectDisposedException(
                    "Phase6cRenderWindowOpenedObserver"
                );
            opened.Clear();
            postDispatchEventCount = 0;
            postDispatchObservedWindowEventCount = 0;
            unresolvedPostDispatchEventCount = 0;
            dispatchBoundaryTick = GetTickCount();
            dispatchBoundarySet = true;
        }
    }

    public void CancelDispatchBoundary()
    {
        lock (gate)
        {
            dispatchBoundarySet = false;
            opened.Clear();
            postDispatchEventCount = 0;
            postDispatchObservedWindowEventCount = 0;
            unresolvedPostDispatchEventCount = 0;
        }
    }

    public bool MessagePumpStarted
    {
        get
        {
            return pumpThreadId != 0 &&
                hook != IntPtr.Zero &&
                !stopped.IsSet;
        }
    }

    public uint DispatchBoundaryTick
    {
        get
        {
            lock (gate)
            {
                return dispatchBoundaryTick;
            }
        }
    }

    public int PostDispatchObservedWindowEventCount
    {
        get
        {
            FlushPump();
            lock (gate)
            {
                return postDispatchObservedWindowEventCount;
            }
        }
    }

    public int UnresolvedPostDispatchEventCount
    {
        get
        {
            FlushPump();
            lock (gate)
            {
                return unresolvedPostDispatchEventCount;
            }
        }
    }

    public long TakeNextHandle()
    {
        FlushPump();
        lock (gate)
        {
            while (opened.Count > 0)
            {
                IntPtr window = opened.Dequeue();
                uint owner;
                if (window != IntPtr.Zero &&
                    GetWindowThreadProcessId(window, out owner) != 0 &&
                    owner == processId)
                    return window.ToInt64();

                unresolvedPostDispatchEventCount += 1;
            }
        }
        return 0;
    }

    public int PostDispatchEventCount
    {
        get
        {
            FlushPump();
            lock (gate)
            {
                return postDispatchEventCount;
            }
        }
    }

    public void ThrowIfAmbiguous()
    {
        FlushPump();
        lock (gate)
        {
            if (unresolvedPostDispatchEventCount > 0)
                throw new InvalidOperationException(
                    "unresolved post-dispatch Psycle window-show event observed"
                );
            if (postDispatchObservedWindowEventCount > 1)
                throw new InvalidOperationException(
                    "multiple post-dispatch Psycle window-show events observed"
                );
            if (postDispatchEventCount > 1)
                throw new InvalidOperationException(
                    "multiple post-dispatch Render as Wav File windows observed"
                );
        }
    }

    public int Seal()
    {
        FlushPump();

        int count;
        uint threadId;
        lock (gate)
        {
            count = postDispatchEventCount;
            if (disposed)
                return count;
            disposed = true;
            dispatchBoundarySet = false;
            threadId = pumpThreadId;
        }

        if (threadId != 0)
        {
            if (!PostThreadMessage(
                threadId,
                WM_QUIT,
                IntPtr.Zero,
                IntPtr.Zero
            ))
                throw new InvalidOperationException(
                    "could not stop render WinEvent message pump"
                );
            if (!stopped.Wait(5000))
                throw new InvalidOperationException(
                    "render WinEvent message pump did not stop"
                );
        }
        return count;
    }

    public void Dispose()
    {
        Seal();
    }
}

public static class Phase6cRenderNative
{
    private delegate bool EnumProc(IntPtr window, IntPtr ignored);

    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumProc callback, IntPtr ignored);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetClassName(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll")] private static extern IntPtr GetMenu(IntPtr window);
    [DllImport("user32.dll")] private static extern IntPtr GetSubMenu(IntPtr menu, int position);
    [DllImport("user32.dll")] private static extern int GetMenuItemCount(IntPtr menu);
    [DllImport("user32.dll")] private static extern uint GetMenuItemID(IntPtr menu, int position);
    [DllImport("user32.dll")] private static extern uint GetMenuState(IntPtr menu, uint position, uint flags);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetMenuString(IntPtr menu, uint position, StringBuilder text, int count, uint flags);
    [DllImport("user32.dll", SetLastError=true)] private static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] private static extern IntPtr GetParent(IntPtr window);
    [DllImport("user32.dll")] private static extern int GetDlgCtrlID(IntPtr window);
    [DllImport("user32.dll")] private static extern bool IsWindowEnabled(IntPtr window);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr window);

    [DllImport("user32.dll", EntryPoint="SendMessageTimeoutW", SetLastError=true)]
    private static extern IntPtr SendValue(
        IntPtr window, uint message, IntPtr wParam, IntPtr lParam,
        uint flags, uint timeout, out IntPtr result);

    [DllImport("user32.dll", EntryPoint="SendMessageTimeoutW", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern IntPtr SendText(
        IntPtr window, uint message, IntPtr wParam, string text,
        uint flags, uint timeout, out IntPtr result);

    [DllImport("user32.dll", EntryPoint="SendMessageTimeoutW", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern IntPtr ReadText(
        IntPtr window, uint message, IntPtr wParam, StringBuilder text,
        uint flags, uint timeout, out IntPtr result);

    public class Command
    {
        public long Window;
        public uint Process;
        public string WindowTitle;
        public string Menu;
        public string Label;
        public uint Id;
        public bool Enabled;
    }

    private static string Plain(string text)
    {
        return text.Split((char)9)[0].Replace("&", "").Trim();
    }

    private static string MenuLabel(IntPtr menu, int position)
    {
        var text = new StringBuilder(1024);
        if (GetMenuString(menu, (uint)position, text, text.Capacity, 0x400) == 0)
            return "";
        return text.ToString();
    }

    private static string Class(IntPtr window)
    {
        var text = new StringBuilder(128);
        GetClassName(window, text, text.Capacity);
        return text.ToString();
    }

    private static bool Owned(IntPtr window, uint process)
    {
        uint owner;
        return window != IntPtr.Zero &&
            GetWindowThreadProcessId(window, out owner) != 0 &&
            owner == process;
    }

    private static bool DescendsFrom(IntPtr child, IntPtr ancestor)
    {
        for (int depth = 0; depth < 64 && child != IntPtr.Zero; ++depth)
        {
            if (child == ancestor) return true;
            child = GetParent(child);
        }
        return false;
    }

    public static Command[] Inspect(uint process, string title)
    {
        var result = new List<Command>();
        EnumWindows(delegate(IntPtr window, IntPtr ignored)
        {
            uint owner;
            GetWindowThreadProcessId(window, out owner);
            if (owner != process) return true;
            var windowTitle = new StringBuilder(1024);
            GetWindowText(window, windowTitle, windowTitle.Capacity);
            if (!string.Equals(windowTitle.ToString(), title, StringComparison.Ordinal))
                return true;
            var menu = GetMenu(window);
            for (int i = 0; i < GetMenuItemCount(menu); ++i)
            {
                if (Plain(MenuLabel(menu, i)) != "File") continue;
                var fileMenu = GetSubMenu(menu, i);
                for (int j = 0; j < GetMenuItemCount(fileMenu); ++j)
                {
                    uint state = GetMenuState(fileMenu, (uint)j, 0x400);
                    result.Add(new Command {
                        Window = window.ToInt64(),
                        Process = owner,
                        WindowTitle = title,
                        Menu = "File",
                        Label = MenuLabel(fileMenu, j),
                        Id = GetMenuItemID(fileMenu, j),
                        Enabled = state != 0xffffffff && (state & 3) == 0
                    });
                }
            }
            return true;
        }, IntPtr.Zero);
        return result.ToArray();
    }

    public static bool Invoke(
        uint process,
        string title,
        Command expected,
        Phase6cRenderWindowOpenedObserver observer
    )
    {
        var matches = new List<Command>();
        foreach (var item in Inspect(process, title))
        {
            if (Plain(item.Label) == "Render as Wav..." && item.Id == 32894)
                matches.Add(item);
        }
        if (matches.Count != 1 || observer == null) return false;
        var command = matches[0];
        if (!command.Enabled || command.Window != expected.Window ||
            command.Id != expected.Id || command.Label != expected.Label)
            return false;

        observer.BeginDispatchBoundary();
        bool posted = PostMessage(
            new IntPtr(command.Window),
            0x0111,
            new IntPtr(command.Id),
            IntPtr.Zero
        );
        if (!posted)
            observer.CancelDispatchBoundary();
        return posted;
    }

    public static string SetText(
        uint process, IntPtr dialog, IntPtr edit, int expectedId, string value)
    {
        if (!Owned(dialog, process) || !Owned(edit, process) ||
            Class(edit) != "Edit" || !IsWindowEnabled(edit) || !IsWindowVisible(edit) ||
            GetDlgCtrlID(edit) != expectedId || !DescendsFrom(edit, dialog))
            return "render filename control identity mismatch";
        IntPtr result;
        if (SendText(edit, 0x000C, IntPtr.Zero, value, 2, 1000, out result) == IntPtr.Zero)
            return "render filename WM_SETTEXT failed";
        var actual = new StringBuilder(value.Length + 4);
        if (ReadText(edit, 0x000D, new IntPtr(actual.Capacity), actual, 2, 1000, out result) == IntPtr.Zero ||
            actual.ToString() != value)
            return "render filename WM_GETTEXT readback mismatch";
        return null;
    }

    public static string SetCombo(
        uint process, IntPtr dialog, IntPtr combo, int expectedId, int index)
    {
        if (!Owned(dialog, process) || !Owned(combo, process) ||
            Class(combo) != "ComboBox" || !IsWindowEnabled(combo) || !IsWindowVisible(combo) ||
            GetDlgCtrlID(combo) != expectedId || !DescendsFrom(combo, dialog))
            return "render combo identity mismatch";
        IntPtr result;
        if (SendValue(combo, 0x014E, new IntPtr(index), IntPtr.Zero, 2, 1000, out result) == IntPtr.Zero ||
            result.ToInt64() != index)
            return "render combo CB_SETCURSEL failed";
        long wparam = ((long)1 << 16) | ((long)expectedId & 0xffff);
        if (SendValue(dialog, 0x0111, new IntPtr(wparam), combo, 2, 1000, out result) == IntPtr.Zero)
            return "render combo CBN_SELCHANGE notification failed";
        return null;
    }

    public static bool ClickButton(
        uint process, IntPtr dialog, IntPtr button)
    {
        if (!Owned(dialog, process) || !Owned(button, process) ||
            Class(button) != "Button" || !IsWindowEnabled(button) ||
            !IsWindowVisible(button) || !DescendsFrom(button, dialog))
            return false;
        IntPtr result;
        return SendValue(button, 0x00F5, IntPtr.Zero, IntPtr.Zero, 2, 1000, out result) != IntPtr.Zero;
    }

    public static bool CloseDialog(uint process, IntPtr dialog)
    {
        if (!Owned(dialog, process) || !IsWindowVisible(dialog))
            return false;
        IntPtr result;
        return SendValue(dialog, 0x0010, IntPtr.Zero, IntPtr.Zero, 2, 1000, out result) != IntPtr.Zero;
    }
}
"@

function Get-Phase6cRenderDialogs(
    [System.Diagnostics.Process]$Process
) {
    $condition = [System.Windows.Automation.PropertyCondition]::new(
        [System.Windows.Automation.AutomationElement]::ProcessIdProperty,
        $Process.Id
    )
    $windows = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
        [System.Windows.Automation.TreeScope]::Children,
        $condition
    )
    $ownedWindows = [System.Collections.Generic.List[System.Windows.Automation.AutomationElement]]::new()
    foreach ($window in $windows) {
        try {
            [void]$ownedWindows.Add($window)
            $descendants = $window.FindAll(
                [System.Windows.Automation.TreeScope]::Descendants,
                $condition
            )
            foreach ($descendant in $descendants) {
                try {
                    if ($descendant.Current.ControlType -eq [System.Windows.Automation.ControlType]::Window) {
                        [void]$ownedWindows.Add($descendant)
                    }
                }
                catch [System.Windows.Automation.ElementNotAvailableException] {
                    continue
                }
            }
        }
        catch [System.Windows.Automation.ElementNotAvailableException] {
            continue
        }
    }

    $byHandle = @{}
    foreach ($window in $ownedWindows) {
        try {
            if ($window.Current.Name -cne "Render as Wav File") {
                continue
            }
            if ($window.Current.ProcessId -ne $Process.Id) {
                continue
            }
            $handle = [long]$window.Current.NativeWindowHandle
            if ($handle -ne 0 -and -not $byHandle.ContainsKey($handle)) {
                $byHandle[$handle] = $window
            }
        }
        catch [System.Windows.Automation.ElementNotAvailableException] {
            continue
        }
    }
    return @($byHandle.Values)
}

function Wait-Phase6cOpenedRenderDialog(
    [System.Diagnostics.Process]$Process,
    [Phase6cRenderWindowOpenedObserver]$Observer,
    [int]$Polls = 40
) {
    for ($poll = 0; $poll -lt $Polls; $poll++) {
        $Observer.ThrowIfAmbiguous()
        $handle = [long]$Observer.TakeNextHandle()
        if ($handle -ne 0) {
            try {
                $dialog = [System.Windows.Automation.AutomationElement]::FromHandle(
                    [IntPtr]$handle
                )
                if ($null -ne $dialog -and
                    $dialog.Current.ProcessId -eq $Process.Id -and
                    $dialog.Current.ControlType -eq [System.Windows.Automation.ControlType]::Window -and
                    $dialog.Current.Name -ceq "Render as Wav File") {
                    return $dialog
                }
            }
            catch [System.Windows.Automation.ElementNotAvailableException] {
                # The event was real but the window vanished before binding.
                # Continue within the existing bounded discovery window.
            }
        }
        if ($poll + 1 -lt $Polls) {
            Start-Sleep -Milliseconds 200
        }
    }
    return $null
}

function Get-Phase6cLiveRenderDialog(
    [System.Diagnostics.Process]$Process,
    [System.Windows.Automation.AutomationElement]$Dialog
) {
    if ($null -eq $Dialog) {
        return $null
    }
    try {
        if ($Dialog.Current.ProcessId -ne $Process.Id -or
            $Dialog.Current.ControlType -ne [System.Windows.Automation.ControlType]::Window -or
            $Dialog.Current.Name -cne "Render as Wav File") {
            return $null
        }
        return $Dialog
    }
    catch [System.Windows.Automation.ElementNotAvailableException] {
        return $null
    }
}

function Get-Phase6cRenderNode(
    [System.Windows.Automation.AutomationElement]$Dialog,
    [string]$AutomationId,
    $ControlType
) {
    $nodes = $Dialog.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition
    )
    $matches = @($nodes | Where-Object {
        $_.Current.AutomationId -ceq $AutomationId -and
        $_.Current.ControlType -eq $ControlType
    })
    if ($matches.Count -ne 1) {
        throw "render control id=$AutomationId is missing or ambiguous"
    }
    return $matches[0]
}

function Select-Phase6cRenderRadio(
    [System.Windows.Automation.AutomationElement]$Dialog,
    [string]$Name
) {
    $nodes = $Dialog.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition
    )
    $matches = @($nodes | Where-Object {
        $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::RadioButton -and
        $_.Current.Name -ceq $Name -and
        $_.Current.IsEnabled
    })
    if ($matches.Count -ne 1) {
        throw "render radio '$Name' is missing, disabled or ambiguous"
    }
    try {
        $selection = [System.Windows.Automation.SelectionItemPattern](
            $matches[0].GetCurrentPattern(
                [System.Windows.Automation.SelectionItemPattern]::Pattern
            )
        )
        $selection.Select()
    }
    catch {
        $invoke = [System.Windows.Automation.InvokePattern](
            $matches[0].GetCurrentPattern(
                [System.Windows.Automation.InvokePattern]::Pattern
            )
        )
        $invoke.Invoke()
    }
}

function Disable-Phase6cRenderDither(
    [System.Windows.Automation.AutomationElement]$Dialog
) {
    $node = Get-Phase6cRenderNode $Dialog "1773" ([System.Windows.Automation.ControlType]::CheckBox)
    $toggle = [System.Windows.Automation.TogglePattern](
        $node.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern)
    )
    if ($toggle.Current.ToggleState -eq [System.Windows.Automation.ToggleState]::On) {
        $toggle.Toggle()
        Start-Sleep -Milliseconds 100
        $node = Get-Phase6cRenderNode $Dialog "1773" ([System.Windows.Automation.ControlType]::CheckBox)
        $toggle = [System.Windows.Automation.TogglePattern](
            $node.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern)
        )
    }
    if ($toggle.Current.ToggleState -ne [System.Windows.Automation.ToggleState]::Off) {
        throw "could not disable render dither"
    }
}

function Disable-Phase6cSeparatedRenders(
    [System.Windows.Automation.AutomationElement]$Dialog
) {
    $nodes = $Dialog.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition
    )
    $boxes = @($nodes | Where-Object {
        $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::CheckBox -and
        $_.Current.Name -clike "Save each *"
    })
    if ($boxes.Count -ne 3) {
        throw "separated-render checkbox signature is missing or ambiguous"
    }
    foreach ($box in $boxes) {
        $toggle = [System.Windows.Automation.TogglePattern](
            $box.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern)
        )
        if ($toggle.Current.ToggleState -eq [System.Windows.Automation.ToggleState]::On) {
            $toggle.Toggle()
        }
    }
}

function Invoke-Phase6cAudioRender(
    [System.Diagnostics.Process]$Process,
    [string]$ExpectedTitle,
    [string]$OutputPath
) {
    $result = [ordered]@{
        schema_version = 1
        outcome = "inconclusive"
        command_verified = $false
        command_dispatched = $false
        dialog_verified = $false
        controls_configured = $false
        save_invoked = $false
        stable_output_polls = 0
        dialog_closed = $false
        close_control_seen = $false
        close_uia_invoked = $false
        close_native_fallback_invoked = $false
        close_wm_close_invoked = $false
        preexisting_render_dialog_count = 0
        render_dialog_native_event_hook_armed = $false
        render_dialog_event_message_pump_started = $false
        render_dialog_dispatch_boundary_set = $false
        render_dialog_dispatch_boundary_tick = $null
        render_dialog_post_dispatch_observed_window_event_count = 0
        render_dialog_unresolved_post_dispatch_event_count = 0
        render_dialog_post_dispatch_event_count = 0
        selected_render_dialog_native_handle = $null
        selected_render_dialog_runtime_id = @()
        dialog_discovery = $null
        requested_output = $null
        output = $null
        observed_output = $null
        process_exited = $false
        process_exit_code = $null
        menu_inventory = @()
        diagnostics = @()
    }
    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $dialogObserver = $null

    try {
        $OutputPath = [System.IO.Path]::GetFullPath($OutputPath)
        $result.requested_output = $OutputPath
        if (Test-Path -LiteralPath $OutputPath) {
            throw "offline render output already exists"
        }
        New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($OutputPath)) -Force | Out-Null

        $Process.Refresh()
        if ($Process.HasExited) {
            throw "reference exited before offline render observation"
        }
        if ([string]::IsNullOrWhiteSpace($ExpectedTitle)) {
            throw "missing verified loaded-fixture window title"
        }

        $result.preexisting_render_dialog_count = @(
            Get-Phase6cRenderDialogs $Process
        ).Count

        $commands = @([Phase6cRenderNative]::Inspect([uint32]$Process.Id, $ExpectedTitle))
        $result.menu_inventory = @($commands | ForEach-Object {
            [ordered]@{
                title = $_.WindowTitle
                menu = $_.Menu
                label = $_.Label
                id = $_.Id
                enabled = $_.Enabled
            }
        })
        $matches = @($commands | Where-Object {
            $_.Id -eq 32894 -and
            ($_.Label.Split([char]9)[0].Replace("&", "").Trim()) -ceq "Render as Wav..."
        })
        if ($matches.Count -ne 1 -or -not $matches[0].Enabled) {
            throw "source-pinned Render as Wav menu signature missing, disabled or ambiguous"
        }
        $result.command_verified = $true
        $dialogObserver = [Phase6cRenderWindowOpenedObserver]::new(
            [uint32]$Process.Id
        )
        $result.render_dialog_native_event_hook_armed = $true
        $result.render_dialog_event_message_pump_started = [bool](
            $dialogObserver.MessagePumpStarted
        )
        if (-not $result.render_dialog_event_message_pump_started) {
            throw "render WinEvent message pump is not running"
        }
        if (-not [Phase6cRenderNative]::Invoke(
            [uint32]$Process.Id,
            $ExpectedTitle,
            $matches[0],
            $dialogObserver
        )) {
            throw "verified Render as Wav command dispatch failed"
        }
        $result.command_dispatched = $true
        $result.render_dialog_dispatch_boundary_set = $true
        $result.render_dialog_dispatch_boundary_tick = [uint32](
            $dialogObserver.DispatchBoundaryTick
        )

        $dialog = Wait-Phase6cOpenedRenderDialog $Process $dialogObserver 40
        if ($null -eq $dialog) {
            throw "post-dispatch Render as Wav File EVENT_OBJECT_SHOW not observed"
        }
        $dialogObserver.ThrowIfAmbiguous()
        try {
            $result.selected_render_dialog_native_handle = [long]$dialog.Current.NativeWindowHandle
            $result.selected_render_dialog_runtime_id = @($dialog.GetRuntimeId())
            $result.dialog_discovery = "pumped-win-event-object-show-strictly-after-dispatch-tick"
        }
        catch [System.Windows.Automation.ElementNotAvailableException] {
            throw "newly opened Render as Wav File dialog became unavailable before binding"
        }

        $filename = Get-Phase6cRenderNode $dialog "1502" ([System.Windows.Automation.ControlType]::Edit)
        $rate = Get-Phase6cRenderNode $dialog "1528" ([System.Windows.Automation.ControlType]::ComboBox)
        $bits = Get-Phase6cRenderNode $dialog "1530" ([System.Windows.Automation.ControlType]::ComboBox)
        $channels = Get-Phase6cRenderNode $dialog "1531" ([System.Windows.Automation.ControlType]::ComboBox)
        $save = Get-Phase6cRenderNode $dialog "1205" ([System.Windows.Automation.ControlType]::Button)
        if ($save.Current.Name.Replace("&", "") -cne "Save Wave") {
            throw "render Save Wave control label mismatch"
        }
        $result.dialog_verified = $true

        Select-Phase6cRenderRadio $dialog "Output to file"
        Select-Phase6cRenderRadio $dialog "Record the entire song"
        Disable-Phase6cSeparatedRenders $dialog

        $dialogHandle = [IntPtr]$dialog.Current.NativeWindowHandle
        $textError = [Phase6cRenderNative]::SetText(
            [uint32]$Process.Id,
            $dialogHandle,
            [IntPtr]$filename.Current.NativeWindowHandle,
            1502,
            $OutputPath
        )
        if ($null -ne $textError) { throw $textError }

        $rateError = [Phase6cRenderNative]::SetCombo(
            [uint32]$Process.Id,
            $dialogHandle,
            [IntPtr]$rate.Current.NativeWindowHandle,
            1528,
            5
        )
        if ($null -ne $rateError) { throw $rateError }
        $bitsError = [Phase6cRenderNative]::SetCombo(
            [uint32]$Process.Id,
            $dialogHandle,
            [IntPtr]$bits.Current.NativeWindowHandle,
            1530,
            1
        )
        if ($null -ne $bitsError) { throw $bitsError }
        $channelsError = [Phase6cRenderNative]::SetCombo(
            [uint32]$Process.Id,
            $dialogHandle,
            [IntPtr]$channels.Current.NativeWindowHandle,
            1531,
            0
        )
        if ($null -ne $channelsError) { throw $channelsError }
        Disable-Phase6cRenderDither $dialog
        $result.controls_configured = $true
        $dialogObserver.ThrowIfAmbiguous()

        $invoke = [System.Windows.Automation.InvokePattern](
            $save.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
        )
        $invoke.Invoke()
        $result.save_invoked = $true

        $previousHash = $null
        for ($poll = 0; $poll -lt 120; $poll++) {
            Start-Sleep -Milliseconds 250
            $dialogObserver.ThrowIfAmbiguous()
            $Process.Refresh()
            if ($Process.HasExited) {
                throw "reference exited during offline render"
            }
            if (Test-Path -LiteralPath $OutputPath -PathType Leaf) {
                $item = Get-Item -LiteralPath $OutputPath
                if ($item.Length -gt 44) {
                    $hash = Get-Sha256 $OutputPath
                    if ($hash -eq $previousHash) {
                        $result.stable_output_polls += 1
                    } else {
                        $result.stable_output_polls = 1
                    }
                    $previousHash = $hash
                }
            } else {
                $result.stable_output_polls = 0
            }

            $currentDialog = Get-Phase6cLiveRenderDialog $Process $dialog
            if ($null -eq $currentDialog) {
                throw "Render as Wav File dialog disappeared before completion"
            }
            $nodes = $currentDialog.FindAll(
                [System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition
            )
            $closeButtons = @($nodes | Where-Object {
                $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and
                $_.Current.Name.Replace("&", "") -ceq "Close" -and
                $_.Current.IsEnabled
            })
            if ($result.stable_output_polls -ge 4 -and $closeButtons.Count -eq 1) {
                $result.close_control_seen = $true
                $closeElement = $closeButtons[0]
                $close = [System.Windows.Automation.InvokePattern](
                    $closeElement.GetCurrentPattern(
                        [System.Windows.Automation.InvokePattern]::Pattern
                    )
                )
                $close.Invoke()
                $result.close_uia_invoked = $true
                for ($closePoll = 0; $closePoll -lt 20; $closePoll++) {
                    Start-Sleep -Milliseconds 100
                    if ($null -eq (Get-Phase6cLiveRenderDialog $Process $dialog)) {
                        $result.dialog_closed = $true
                        break
                    }
                }
                if (-not $result.dialog_closed) {
                    $dialogHandle = [IntPtr]$currentDialog.Current.NativeWindowHandle
                    $buttonHandle = [IntPtr]$closeElement.Current.NativeWindowHandle
                    if ([Phase6cRenderNative]::ClickButton(
                        [uint32]$Process.Id,
                        $dialogHandle,
                        $buttonHandle
                    )) {
                        $result.close_native_fallback_invoked = $true
                        for ($closePoll = 0; $closePoll -lt 20; $closePoll++) {
                            Start-Sleep -Milliseconds 100
                            if ($null -eq (Get-Phase6cLiveRenderDialog $Process $dialog)) {
                                $result.dialog_closed = $true
                                break
                            }
                        }
                    }
                }
                if (-not $result.dialog_closed) {
                    $dialogHandle = [IntPtr]$currentDialog.Current.NativeWindowHandle
                    if ([Phase6cRenderNative]::CloseDialog(
                        [uint32]$Process.Id,
                        $dialogHandle
                    )) {
                        $result.close_wm_close_invoked = $true
                        for ($closePoll = 0; $closePoll -lt 20; $closePoll++) {
                            Start-Sleep -Milliseconds 100
                            if ($null -eq (Get-Phase6cLiveRenderDialog $Process $dialog)) {
                                $result.dialog_closed = $true
                                break
                            }
                        }
                    }
                }
                break
            }
        }

        if ($result.stable_output_polls -lt 4 -or -not $result.close_control_seen) {
            throw "offline render did not reach a stable completed output"
        }
        if (-not $result.dialog_closed) {
            $diagnostics.Add(
                "render output finalized and Close control was verified, but dialog teardown did not complete"
            )
        }
        if (-not (Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
            throw "offline render output missing after completion"
        }
        $dialogObserver.ThrowIfAmbiguous()
        $result.render_dialog_post_dispatch_observed_window_event_count = [int](
            $dialogObserver.PostDispatchObservedWindowEventCount
        )
        $result.render_dialog_unresolved_post_dispatch_event_count = [int](
            $dialogObserver.UnresolvedPostDispatchEventCount
        )
        $result.render_dialog_post_dispatch_event_count = $dialogObserver.Seal()
        $dialogObserver = $null
        if ($result.render_dialog_post_dispatch_event_count -ne 1 -or
            $result.render_dialog_unresolved_post_dispatch_event_count -ne 0) {
            throw "offline render requires exactly one post-dispatch Render as Wav File event"
        }
        $result.output = [ordered]@{
            path = [System.IO.Path]::GetFileName($OutputPath)
            sha256 = Get-Sha256 $OutputPath
        }
        $result.outcome = "rendered"
    }
    catch {
        $diagnostics.Add($_.Exception.Message)
    }
    finally {
        if ($null -ne $dialogObserver) {
            try {
                $result.render_dialog_post_dispatch_observed_window_event_count = [int](
                    $dialogObserver.PostDispatchObservedWindowEventCount
                )
                $result.render_dialog_unresolved_post_dispatch_event_count = [int](
                    $dialogObserver.UnresolvedPostDispatchEventCount
                )
                $result.render_dialog_post_dispatch_event_count = $dialogObserver.Seal()
            }
            catch {
                $diagnostics.Add(
                    "could not seal render-dialog observer: $($_.Exception.Message)"
                )
            }
            $dialogObserver = $null
        }
    }

    try {
        $Process.Refresh()
        $result.process_exited = [bool]$Process.HasExited
        if ($result.process_exited) {
            $result.process_exit_code = [int64]$Process.ExitCode
        }
    }
    catch {
        $diagnostics.Add("could not inspect reference process after render attempt: $($_.Exception.Message)")
    }

    if ($null -ne $result.requested_output -and
        (Test-Path -LiteralPath $result.requested_output -PathType Leaf)) {
        $observed = Get-Item -LiteralPath $result.requested_output
        $result.observed_output = [ordered]@{
            path = [System.IO.Path]::GetFileName($result.requested_output)
            size_bytes = [int64]$observed.Length
            sha256 = Get-Sha256 $result.requested_output
        }
    }

    $result.diagnostics = @($diagnostics.ToArray())
    return $result
}
