#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0600
#define _WIN32_IE 0x0600
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <commctrl.h>
#include <wchar.h>
#include <stdio.h>

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef ODS_HOTLIGHT
#define ODS_HOTLIGHT 0x0040
#endif

#define APP_NAME L"20-20 Break"
#define ARRAY_COUNT(value) (sizeof(value) / sizeof((value)[0]))
#define WM_TRAYICON (WM_APP + 1)
#define ID_ACTION_BUTTON 1001
#define ID_TRAY_OPEN 1002
#define ID_TRAY_START_BREAK 1003
#define ID_TRAY_EXIT 1004
#define ID_TIMER_TICK 2001
#define IDI_APP_ICON 3001
#define BREAK_INTERVAL_SECONDS (20 * 60)
#define BREAK_DURATION_SECONDS 20
#define WINDOW_CLASS_NAME L"TwentyTwentyBreakWindow"
#define INSTANCE_MUTEX_NAME L"Local\\TwentyTwentyBreakReminderSingleInstance"

typedef struct AppTheme {
    COLORREF background;
    COLORREF surface;
    COLORREF text;
    COLORREF muted_text;
    COLORREF accent;
    COLORREF progress_track;
    BOOL dark;
} AppTheme;

typedef BOOL (WINAPI *SetProcessDpiAwarenessContextProc)(HANDLE);
typedef BOOL (WINAPI *SetProcessDPIAwareProc)(void);
typedef UINT (WINAPI *GetDpiForWindowProc)(HWND);
typedef BOOL (WINAPI *AdjustWindowRectExForDpiProc)(LPRECT, DWORD, BOOL, DWORD, UINT);
typedef HRESULT (WINAPI *DwmSetWindowAttributeProc)(HWND, DWORD, LPCVOID, DWORD);

static HWND g_hwnd;
static HWND g_action_button;
static NOTIFYICONDATAW g_nid;
static HICON g_app_icon;
static HANDLE g_instance_mutex;
static HFONT g_label_font;
static HFONT g_heading_font;
static HFONT g_countdown_font;
static HFONT g_body_font;
static HFONT g_button_font;
static AppTheme g_theme;
static UINT g_dpi = 96;
static UINT g_taskbar_created_message;
static int g_seconds_until_break = BREAK_INTERVAL_SECONDS;
static int g_break_seconds_left;
static BOOL g_in_break;
static BOOL g_exit_requested;
static BOOL g_tray_icon_added;

static int Scale(int value)
{
    return MulDiv(value, (int)g_dpi, 96);
}

static UINT WindowDpi(HWND hwnd)
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    GetDpiForWindowProc get_dpi = (GetDpiForWindowProc)GetProcAddress(user32, "GetDpiForWindow");
    if (get_dpi) {
        return get_dpi(hwnd);
    }
    {
        HDC hdc = GetDC(hwnd);
        UINT dpi = (UINT)GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(hwnd, hdc);
        return dpi ? dpi : 96;
    }
}

static UINT SystemDpi(void)
{
    HDC hdc = GetDC(NULL);
    UINT dpi = (UINT)GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);
    return dpi ? dpi : 96;
}

static void EnableBestDpiAwareness(void)
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    SetProcessDpiAwarenessContextProc set_context =
        (SetProcessDpiAwarenessContextProc)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
    if (set_context) {
        set_context((HANDLE)-4);
    } else {
        SetProcessDPIAwareProc set_aware =
            (SetProcessDPIAwareProc)GetProcAddress(user32, "SetProcessDPIAware");
        if (set_aware) {
            set_aware();
        }
    }
}

static void AdjustInitialWindowRect(RECT *rect, DWORD style, UINT dpi)
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    AdjustWindowRectExForDpiProc adjust_for_dpi =
        (AdjustWindowRectExForDpiProc)GetProcAddress(user32, "AdjustWindowRectExForDpi");
    if (adjust_for_dpi) {
        adjust_for_dpi(rect, style, FALSE, 0, dpi);
    } else {
        AdjustWindowRectEx(rect, style, FALSE, 0);
    }
}

static COLORREF Color(BYTE red, BYTE green, BYTE blue)
{
    return RGB(red, green, blue);
}

static BOOL IsLightTheme(void)
{
    HKEY key;
    DWORD value = 1;
    DWORD size = sizeof(value);
    if (RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0,
        KEY_QUERY_VALUE,
        &key) == ERROR_SUCCESS) {
        RegQueryValueExW(key, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(key);
    }
    return value != 0;
}

static void LoadTheme(void)
{
    if (IsLightTheme()) {
        g_theme = (AppTheme) {
            Color(247, 248, 250), Color(255, 255, 255), Color(28, 32, 38),
            Color(102, 109, 120), Color(37, 99, 235), Color(229, 232, 238), FALSE
        };
    } else {
        g_theme = (AppTheme) {
            Color(24, 26, 30), Color(32, 35, 40), Color(242, 244, 247),
            Color(166, 172, 181), Color(96, 140, 255), Color(58, 62, 69), TRUE
        };
    }
}

static HFONT CreateUiFont(int point_size, int weight, const wchar_t *face)
{
    int height = -MulDiv(point_size, (int)g_dpi, 72);
    return CreateFontW(
        height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, face);
}

static void DeleteFonts(void)
{
    DeleteObject(g_label_font);
    DeleteObject(g_heading_font);
    DeleteObject(g_countdown_font);
    DeleteObject(g_body_font);
    DeleteObject(g_button_font);
}

static void CreateFonts(void)
{
    DeleteFonts();
    g_label_font = CreateUiFont(9, FW_SEMIBOLD, L"Segoe UI Variable Text");
    g_heading_font = CreateUiFont(20, FW_SEMIBOLD, L"Segoe UI Variable Display");
    g_countdown_font = CreateUiFont(52, FW_SEMIBOLD, L"Segoe UI Variable Display");
    g_body_font = CreateUiFont(10, FW_NORMAL, L"Segoe UI Variable Text");
    g_button_font = CreateUiFont(10, FW_SEMIBOLD, L"Segoe UI");
    if (g_action_button) {
        SendMessageW(g_action_button, WM_SETFONT, (WPARAM)g_button_font, TRUE);
    }
}

static void FormatTime(int seconds, wchar_t *buffer, size_t buffer_count)
{
    int minutes = seconds / 60;
    int remaining_seconds = seconds % 60;
    _snwprintf(buffer, buffer_count, L"%02d:%02d", minutes, remaining_seconds);
    buffer[buffer_count - 1] = L'\0';
}

static void UpdateActionText(void)
{
    SetWindowTextW(g_action_button, g_in_break ? L"End break" : L"Take a break");
}

static void ShowTrayNotification(const wchar_t *title, const wchar_t *message)
{
    if (!g_tray_icon_added) {
        return;
    }
    g_nid.uFlags = NIF_INFO;
    wcsncpy(g_nid.szInfoTitle, title, ARRAY_COUNT(g_nid.szInfoTitle) - 1);
    wcsncpy(g_nid.szInfo, message, ARRAY_COUNT(g_nid.szInfo) - 1);
    g_nid.szInfoTitle[ARRAY_COUNT(g_nid.szInfoTitle) - 1] = L'\0';
    g_nid.szInfo[ARRAY_COUNT(g_nid.szInfo) - 1] = L'\0';
    g_nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

static void ShowReminderWindow(void)
{
    ShowWindow(g_hwnd, SW_RESTORE);
    SetForegroundWindow(g_hwnd);
}

static void ShowExistingInstance(void)
{
    HWND existing = FindWindowW(WINDOW_CLASS_NAME, NULL);
    if (existing) {
        ShowWindow(existing, SW_RESTORE);
        SetForegroundWindow(existing);
    }
}

static void StartBreak(void)
{
    g_in_break = TRUE;
    g_break_seconds_left = BREAK_DURATION_SECONDS;
    UpdateActionText();
    ShowTrayNotification(APP_NAME, L"Look away for 20 seconds and let your eyes relax.");
    InvalidateRect(g_hwnd, NULL, FALSE);
    ShowReminderWindow();
}

static void FinishBreak(void)
{
    g_in_break = FALSE;
    g_break_seconds_left = 0;
    g_seconds_until_break = BREAK_INTERVAL_SECONDS;
    UpdateActionText();
    InvalidateRect(g_hwnd, NULL, FALSE);
    ShowWindow(g_hwnd, SW_HIDE);
}

static BOOL AddTrayIcon(HWND hwnd)
{
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_app_icon;
    wcsncpy(g_nid.szTip, APP_NAME, ARRAY_COUNT(g_nid.szTip) - 1);
    g_nid.szTip[ARRAY_COUNT(g_nid.szTip) - 1] = L'\0';
    g_tray_icon_added = Shell_NotifyIconW(NIM_ADD, &g_nid);
    if (g_tray_icon_added) {
        g_nid.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &g_nid);
    }
    return g_tray_icon_added;
}

static void RemoveTrayIcon(void)
{
    if (g_tray_icon_added) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_tray_icon_added = FALSE;
    }
}

static void ShowTrayMenu(HWND hwnd)
{
    POINT cursor;
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | MF_DEFAULT, ID_TRAY_OPEN, L"Open");
    AppendMenuW(menu, MF_STRING, ID_TRAY_START_BREAK, L"Start break now");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd, NULL);
    DestroyMenu(menu);
}

static void SetTextStyle(HDC hdc, HFONT font, COLORREF color)
{
    SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
}

static void DrawTextLine(HDC hdc, const wchar_t *text, RECT rect, UINT format)
{
    DrawTextW(hdc, text, -1, &rect, format | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

static void LayoutControls(HWND hwnd)
{
    RECT client;
    GetClientRect(hwnd, &client);
    MoveWindow(
        g_action_button,
        client.right - Scale(160),
        client.bottom - Scale(62),
        Scale(132),
        Scale(36),
        TRUE);
}

static void PaintBackground(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT client;
    HDC hdc = BeginPaint(hwnd, &ps);
    HDC buffer_dc;
    HBITMAP buffer_bitmap;
    HBITMAP old_bitmap;
    HBRUSH brush;
    HBRUSH old_brush;
    HPEN old_pen;
    wchar_t time_text[16];
    RECT rect;
    int total_seconds = g_in_break ? BREAK_DURATION_SECONDS : BREAK_INTERVAL_SECONDS;
    int current_seconds = g_in_break ? g_break_seconds_left : g_seconds_until_break;
    int track_width;
    int progress_width;

    GetClientRect(hwnd, &client);
    buffer_dc = CreateCompatibleDC(hdc);
    buffer_bitmap = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    old_bitmap = (HBITMAP)SelectObject(buffer_dc, buffer_bitmap);

    brush = CreateSolidBrush(g_theme.background);
    FillRect(buffer_dc, &client, brush);
    DeleteObject(brush);

    SetTextStyle(buffer_dc, g_label_font, g_theme.accent);
    rect = (RECT) { Scale(28), Scale(18), client.right - Scale(28), Scale(44) };
    DrawTextLine(buffer_dc, g_in_break ? L"BREAK IN PROGRESS" : L"NEXT EYE BREAK", rect, DT_LEFT);

    SetTextStyle(buffer_dc, g_heading_font, g_theme.text);
    rect = (RECT) { Scale(28), Scale(44), client.right - Scale(28), Scale(80) };
    DrawTextLine(buffer_dc, g_in_break ? L"Rest your eyes" : L"Look away in", rect, DT_LEFT);

    FormatTime(current_seconds, time_text, ARRAY_COUNT(time_text));
    SetTextStyle(buffer_dc, g_countdown_font, g_theme.text);
    rect = (RECT) { Scale(24), Scale(78), client.right - Scale(24), Scale(156) };
    DrawTextLine(buffer_dc, time_text, rect, DT_LEFT);

    SetTextStyle(buffer_dc, g_body_font, g_theme.muted_text);
    rect = (RECT) { Scale(28), Scale(156), client.right - Scale(28), Scale(190) };
    DrawTextLine(
        buffer_dc,
        g_in_break ? L"Focus on something distant and breathe normally." : L"Look 20 feet away for 20 seconds every 20 minutes.",
        rect,
        DT_LEFT);

    rect = (RECT) { Scale(28), Scale(205), client.right - Scale(28), Scale(211) };
    track_width = rect.right - rect.left;
    progress_width = total_seconds > 0 ? track_width * current_seconds / total_seconds : 0;
    brush = CreateSolidBrush(g_theme.progress_track);
    old_brush = (HBRUSH)SelectObject(buffer_dc, brush);
    old_pen = (HPEN)SelectObject(buffer_dc, GetStockObject(NULL_PEN));
    RoundRect(buffer_dc, rect.left, rect.top, rect.right, rect.bottom, Scale(6), Scale(6));
    SelectObject(buffer_dc, old_brush);
    DeleteObject(brush);
    if (progress_width > 0) {
        brush = CreateSolidBrush(g_theme.accent);
        old_brush = (HBRUSH)SelectObject(buffer_dc, brush);
        RoundRect(buffer_dc, rect.left, rect.top, rect.left + progress_width, rect.bottom, Scale(6), Scale(6));
        SelectObject(buffer_dc, old_brush);
        DeleteObject(brush);
    }
    SelectObject(buffer_dc, old_pen);

    BitBlt(hdc, 0, 0, client.right, client.bottom, buffer_dc, 0, 0, SRCCOPY);
    SelectObject(buffer_dc, old_bitmap);
    DeleteObject(buffer_bitmap);
    DeleteDC(buffer_dc);
    EndPaint(hwnd, &ps);
}

static void DrawActionButton(const DRAWITEMSTRUCT *draw)
{
    RECT rect = draw->rcItem;
    wchar_t text[64];
    COLORREF fill = g_theme.surface;
    HBRUSH brush;
    HBRUSH old_brush;
    HPEN pen;
    HPEN old_pen;
    HFONT old_font;

    if (draw->itemState & ODS_SELECTED) {
        fill = g_theme.progress_track;
    } else if (draw->itemState & ODS_HOTLIGHT) {
        fill = g_theme.progress_track;
    }

    brush = CreateSolidBrush(g_theme.background);
    FillRect(draw->hDC, &rect, brush);
    DeleteObject(brush);

    brush = CreateSolidBrush(fill);
    pen = CreatePen(PS_SOLID, (draw->itemState & ODS_FOCUS) ? Scale(2) : 1, g_theme.accent);
    old_brush = (HBRUSH)SelectObject(draw->hDC, brush);
    old_pen = (HPEN)SelectObject(draw->hDC, pen);
    RoundRect(draw->hDC, rect.left, rect.top, rect.right, rect.bottom, Scale(8), Scale(8));
    SelectObject(draw->hDC, old_brush);
    SelectObject(draw->hDC, old_pen);
    DeleteObject(pen);
    DeleteObject(brush);

    GetWindowTextW(draw->hwndItem, text, ARRAY_COUNT(text));
    old_font = (HFONT)SelectObject(draw->hDC, g_button_font);
    SetTextColor(draw->hDC, g_theme.accent);
    SetBkMode(draw->hDC, TRANSPARENT);
    DrawTextLine(draw->hDC, text, rect, DT_CENTER);
    SelectObject(draw->hDC, old_font);

}

static void RefreshAppearance(HWND hwnd)
{
    BOOL use_dark_mode;
    HMODULE dwmapi;
    DwmSetWindowAttributeProc set_window_attribute;
    LoadTheme();
    use_dark_mode = g_theme.dark;
    dwmapi = LoadLibraryW(L"dwmapi.dll");
    if (dwmapi) {
        set_window_attribute =
            (DwmSetWindowAttributeProc)GetProcAddress(dwmapi, "DwmSetWindowAttribute");
        if (set_window_attribute) {
            set_window_attribute(hwnd, 20, &use_dark_mode, sizeof(use_dark_mode));
        }
        FreeLibrary(dwmapi);
    }
    InvalidateRect(hwnd, NULL, TRUE);
    InvalidateRect(g_action_button, NULL, TRUE);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (g_taskbar_created_message && msg == g_taskbar_created_message) {
        g_tray_icon_added = FALSE;
        AddTrayIcon(hwnd);
        return 0;
    }

    switch (msg) {
    case WM_CREATE:
        g_dpi = WindowDpi(hwnd);
        LoadTheme();
        CreateFonts();
        g_action_button = CreateWindowW(
            L"BUTTON", L"Take a break",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            0, 0, 0, 0, hwnd, (HMENU)ID_ACTION_BUTTON, NULL, NULL);
        SendMessageW(g_action_button, WM_SETFONT, (WPARAM)g_button_font, TRUE);
        LayoutControls(hwnd);
        AddTrayIcon(hwnd);
        SetTimer(hwnd, ID_TIMER_TICK, 1000, NULL);
        RefreshAppearance(hwnd);
        return 0;
    case WM_SIZE:
        LayoutControls(hwnd);
        return 0;
    case WM_DPICHANGED: {
        RECT *suggested = (RECT *)lparam;
        g_dpi = HIWORD(wparam);
        CreateFonts();
        SetWindowPos(
            hwnd, NULL, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        LayoutControls(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    case WM_SETTINGCHANGE:
        RefreshAppearance(hwnd);
        return 0;
    case WM_DRAWITEM:
        if (wparam == ID_ACTION_BUTTON) {
            DrawActionButton((const DRAWITEMSTRUCT *)lparam);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case ID_ACTION_BUTTON:
            if (g_in_break) {
                FinishBreak();
            } else {
                StartBreak();
            }
            return 0;
        case ID_TRAY_OPEN:
            ShowReminderWindow();
            return 0;
        case ID_TRAY_START_BREAK:
            StartBreak();
            return 0;
        case ID_TRAY_EXIT:
            g_exit_requested = TRUE;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_TIMER:
        if (wparam == ID_TIMER_TICK) {
            if (g_in_break) {
                if (--g_break_seconds_left <= 0) {
                    FinishBreak();
                }
            } else if (--g_seconds_until_break <= 0) {
                StartBreak();
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        break;
    case WM_TRAYICON:
        if (LOWORD(lparam) == WM_LBUTTONDBLCLK) {
            ShowReminderWindow();
            return 0;
        }
        if (LOWORD(lparam) == WM_CONTEXTMENU || LOWORD(lparam) == WM_RBUTTONUP) {
            ShowTrayMenu(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        if (g_exit_requested || !g_tray_icon_added) {
            DestroyWindow(hwnd);
        } else {
            ShowWindow(hwnd, SW_HIDE);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        PaintBackground(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_TICK);
        RemoveTrayIcon();
        DeleteFonts();
        if (g_instance_mutex) {
            CloseHandle(g_instance_mutex);
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPSTR command_line, int show_command)
{
    WNDCLASSW wc;
    MSG msg;
    RECT window_rect;
    UINT initial_dpi;
    int message_result;

    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    EnableBestDpiAwareness();
    initial_dpi = SystemDpi();
    window_rect = (RECT) {
        0, 0,
        MulDiv(440, (int)initial_dpi, 96),
        MulDiv(310, (int)initial_dpi, 96)
    };
    g_instance_mutex = CreateMutexW(NULL, TRUE, INSTANCE_MUTEX_NAME);
    if (!g_instance_mutex) {
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        ShowExistingInstance();
        CloseHandle(g_instance_mutex);
        return 0;
    }

    g_app_icon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    if (!g_app_icon) {
        g_app_icon = LoadIconW(NULL, IDI_INFORMATION);
    }
    g_taskbar_created_message = RegisterWindowMessageW(L"TaskbarCreated");

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = g_app_icon;

    if (!RegisterClassW(&wc)) {
        return 1;
    }

    AdjustInitialWindowRect(
        &window_rect,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        initial_dpi);
    g_hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        APP_NAME,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_rect.right - window_rect.left,
        window_rect.bottom - window_rect.top,
        NULL, NULL, instance, NULL);

    if (!g_hwnd) {
        return 1;
    }

    ShowWindow(g_hwnd, SW_SHOWNORMAL);
    UpdateWindow(g_hwnd);

    while ((message_result = GetMessageW(&msg, NULL, 0, 0)) > 0) {
        if (!IsDialogMessageW(g_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return message_result == -1 ? 1 : (int)msg.wParam;
}
