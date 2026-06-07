using System.Runtime.InteropServices;

namespace BreakReminderDotNet10;

internal sealed class TrayIcon : IDisposable
{
    private const uint MfSeparator = 0x00000800;
    private const uint MfString = 0x00000000;
    private const uint NiifInfo = 0x00000001;
    private const uint NifIcon = 0x00000002;
    private const uint NifInfo = 0x00000010;
    private const uint NifMessage = 0x00000001;
    private const uint NifShowTip = 0x00000080;
    private const uint NifTip = 0x00000004;
    private const uint NimAdd = 0x00000000;
    private const uint NimDelete = 0x00000002;
    private const uint NimModify = 0x00000001;
    private const uint NimSetVersion = 0x00000004;
    private const uint NotifyIconVersion4 = 4;
    private const uint TpmBottomAlign = 0x0020;
    private const uint TpmLeftAlign = 0x0000;
    private const uint TpmReturnCmd = 0x0100;
    private const uint TpmRightButton = 0x0002;
    private const int GwlWndProc = -4;
    private const int MenuCommandOpen = 1001;
    private const int MenuCommandStartBreak = 1002;
    private const int MenuCommandExit = 1003;
    private const int WmApp = 0x8000;
    private const int WmContextMenu = 0x007B;
    private const int WmLButtonDblClk = 0x0203;
    private const int WmLButtonUp = 0x0202;
    private const int WmNull = 0x0000;
    private const int WmTrayIcon = WmApp + 1;

    private readonly Action _exitRequested;
    private readonly nint _hwnd;
    private readonly nint _iconHandle;
    private readonly Action _openRequested;
    private readonly Action _startBreakRequested;
    private readonly WndProcDelegate _wndProc;
    private bool _disposed;
    private nint _previousWndProc;

    public TrayIcon(nint hwnd, string toolTip, Action openRequested, Action startBreakRequested, Action exitRequested)
    {
        _hwnd = hwnd;
        _openRequested = openRequested;
        _startBreakRequested = startBreakRequested;
        _exitRequested = exitRequested;
        _wndProc = HandleWindowMessage;
        _iconHandle = LoadApplicationIcon();

        _previousWndProc = SetWindowLongPtr(_hwnd, GwlWndProc, Marshal.GetFunctionPointerForDelegate(_wndProc));
        AddIcon(toolTip);
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        DeleteIcon();
        if (_previousWndProc != 0)
        {
            _ = SetWindowLongPtr(_hwnd, GwlWndProc, _previousWndProc);
            _previousWndProc = 0;
        }

        if (_iconHandle != 0)
        {
            _ = DestroyIcon(_iconHandle);
        }

        GC.SuppressFinalize(this);
    }

    public void ShowBalloon(string title, string message)
    {
        NOTIFYICONDATA data = CreateData(NifInfo);
        data.szInfoTitle = title;
        data.szInfo = message;
        data.dwInfoFlags = NiifInfo;
        _ = Shell_NotifyIcon(NimModify, ref data);
    }

    private void AddIcon(string toolTip)
    {
        NOTIFYICONDATA data = CreateData(NifMessage | NifIcon | NifTip | NifShowTip);
        data.uCallbackMessage = WmTrayIcon;
        data.hIcon = _iconHandle;
        data.szTip = toolTip;
        data.uVersion = NotifyIconVersion4;

        if (!Shell_NotifyIcon(NimAdd, ref data))
        {
            throw new InvalidOperationException("Failed to add tray icon.");
        }

        _ = Shell_NotifyIcon(NimSetVersion, ref data);
    }

    private void DeleteIcon()
    {
        NOTIFYICONDATA data = CreateData(0);
        _ = Shell_NotifyIcon(NimDelete, ref data);
    }

    private nint HandleWindowMessage(nint hwnd, uint message, nint wParam, nint lParam)
    {
        if (message == WmTrayIcon)
        {
            switch (LowWord(lParam))
            {
                case WmContextMenu:
                    ShowContextMenu(PointFromPackedValue(wParam));
                    return 0;
                case WmLButtonUp:
                case WmLButtonDblClk:
                    _openRequested();
                    return 0;
            }
        }

        return CallWindowProc(_previousWndProc, hwnd, message, wParam, lParam);
    }

    private static nint LoadApplicationIcon()
    {
        string? processPath = Environment.ProcessPath;
        if (string.IsNullOrWhiteSpace(processPath))
        {
            throw new InvalidOperationException("Failed to resolve the application executable path.");
        }

        uint extracted = ExtractIconEx(processPath, 0, out nint largeIcon, out _, 1);
        if (extracted == 0 || largeIcon == 0)
        {
            throw new InvalidOperationException("Failed to load the application icon.");
        }

        return largeIcon;
    }

    private NOTIFYICONDATA CreateData(uint flags)
    {
        return new NOTIFYICONDATA
        {
            cbSize = (uint)Marshal.SizeOf<NOTIFYICONDATA>(),
            hWnd = _hwnd,
            uID = 1,
            uFlags = flags
        };
    }

    private void ShowContextMenu((int X, int Y) screenPoint)
    {
        nint menuHandle = CreatePopupMenu();
        if (menuHandle == 0)
        {
            return;
        }

        try
        {
            _ = AppendMenu(menuHandle, MfString, MenuCommandOpen, "Open");
            _ = AppendMenu(menuHandle, MfString, MenuCommandStartBreak, "Start break now");
            _ = AppendMenu(menuHandle, MfSeparator, 0, string.Empty);
            _ = AppendMenu(menuHandle, MfString, MenuCommandExit, "Exit");

            _ = SetForegroundWindow(_hwnd);
            uint command = TrackPopupMenuEx(menuHandle, TpmLeftAlign | TpmBottomAlign | TpmRightButton | TpmReturnCmd, screenPoint.X, screenPoint.Y, _hwnd, nint.Zero);
            _ = PostMessage(_hwnd, WmNull, 0, 0);
            ExecuteCommand(command);
        }
        finally
        {
            _ = DestroyMenu(menuHandle);
        }
    }

    private void ExecuteCommand(uint command)
    {
        switch (command)
        {
            case MenuCommandOpen:
                _openRequested();
                break;
            case MenuCommandStartBreak:
                _startBreakRequested();
                break;
            case MenuCommandExit:
                _exitRequested();
                break;
        }
    }

    private static int LowWord(nint value)
    {
        return unchecked((short)((long)value & 0xFFFF));
    }

    private static (int X, int Y) PointFromPackedValue(nint value)
    {
        int x = unchecked((short)((long)value & 0xFFFF));
        int y = unchecked((short)(((long)value >> 16) & 0xFFFF));
        return (x, y);
    }

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool AppendMenu(nint hMenu, uint uFlags, uint uIDNewItem, string lpNewItem);

    [DllImport("user32.dll")]
    private static extern nint CallWindowProc(nint lpPrevWndFunc, nint hWnd, uint msg, nint wParam, nint lParam);

    [DllImport("user32.dll")]
    private static extern nint CreatePopupMenu();

    [DllImport("user32.dll")]
    private static extern bool DestroyIcon(nint hIcon);

    [DllImport("user32.dll")]
    private static extern bool DestroyMenu(nint hMenu);

    [DllImport("shell32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern uint ExtractIconEx(string lpszFile, int nIconIndex, out nint phiconLarge, out nint phiconSmall, uint nIcons);

    [DllImport("user32.dll")]
    private static extern bool PostMessage(nint hWnd, uint msg, nint wParam, nint lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern nint SetWindowLongPtr(nint hWnd, int nIndex, nint dwNewLong);

    [DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(nint hWnd);

    [DllImport("shell32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool Shell_NotifyIcon(uint dwMessage, ref NOTIFYICONDATA lpData);

    [DllImport("user32.dll")]
    private static extern uint TrackPopupMenuEx(nint hmenu, uint fuFlags, int x, int y, nint hwnd, nint lptpm);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct NOTIFYICONDATA
    {
        public uint cbSize;
        public nint hWnd;
        public uint uID;
        public uint uFlags;
        public uint uCallbackMessage;
        public nint hIcon;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string szTip;

        public uint dwState;
        public uint dwStateMask;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string szInfo;

        public uint uVersion;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string szInfoTitle;

        public uint dwInfoFlags;
        public Guid guidItem;
        public nint hBalloonIcon;
    }

    private delegate nint WndProcDelegate(nint hwnd, uint message, nint wParam, nint lParam);
}
