#define UNICODE
#define _UNICODE
#define _WIN32_IE 0x0500

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <wchar.h>

#define APP_NAME L"20-20 Break"
#define ARRAY_COUNT(value) (sizeof(value) / sizeof((value)[0]))
#define WM_TRAYICON (WM_APP + 1)
#define ID_TRAY_SHOW 1001
#define ID_TRAY_EXIT 1002
#define ID_TIMER_TICK 2001
#define IDI_APP_ICON 3001
#define BREAK_INTERVAL_SECONDS (20 * 60)
#define BREAK_DURATION_SECONDS 20
#define WINDOW_CLASS_NAME L"TwentyTwentyBreakWindow"
#define INSTANCE_MUTEX_NAME L"Local\\TwentyTwentyBreakReminderSingleInstance"

static HWND g_hwnd;
static HWND g_title;
static HWND g_countdown;
static HWND g_status;
static HWND g_skip_button;
static NOTIFYICONDATAW g_nid;
static HICON g_app_icon;
static HANDLE g_instance_mutex;
static int g_seconds_until_break = BREAK_INTERVAL_SECONDS;
static int g_break_seconds_left = 0;
static BOOL g_in_break = FALSE;
static BOOL g_exit_requested = FALSE;

static HFONT CreateUiFont(int point_size, int weight)
{
    HDC hdc = GetDC(NULL);
    int height = -MulDiv(point_size, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    return CreateFontW(
        height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

static void FormatTime(int seconds, wchar_t *buffer, size_t buffer_count)
{
    int minutes = seconds / 60;
    int remaining_seconds = seconds % 60;
    swprintf(buffer, buffer_count, L"%02d:%02d", minutes, remaining_seconds);
}

static void UpdateStatusText(void)
{
    wchar_t text[128];
    wchar_t time_text[16];

    if (g_in_break) {
        FormatTime(g_break_seconds_left, time_text, ARRAY_COUNT(time_text));
        SetWindowTextW(g_title, L"Time for a 20 second break");
        SetWindowTextW(g_countdown, time_text);
        SetWindowTextW(g_status, L"Look away, stretch, and relax your eyes.");
        SetWindowTextW(g_skip_button, L"Finish Break");
        return;
    }

    FormatTime(g_seconds_until_break, time_text, ARRAY_COUNT(time_text));
    swprintf(text, ARRAY_COUNT(text), L"Next break in %s", time_text);
    SetWindowTextW(g_title, L"Break reminder is running");
    SetWindowTextW(g_countdown, time_text);
    SetWindowTextW(g_status, text);
    SetWindowTextW(g_skip_button, L"Start Break Now");
}

static void ShowTrayNotification(const wchar_t *title, const wchar_t *message)
{
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
    ShowWindow(g_hwnd, SW_SHOWNORMAL);
    SetForegroundWindow(g_hwnd);
    BringWindowToTop(g_hwnd);
}

static void ShowExistingInstance(void)
{
    HWND existing = FindWindowW(WINDOW_CLASS_NAME, NULL);
    if (existing) {
        ShowWindow(existing, SW_SHOWNORMAL);
        SetForegroundWindow(existing);
        BringWindowToTop(existing);
    }
}

static void StartBreak(void)
{
    g_in_break = TRUE;
    g_break_seconds_left = BREAK_DURATION_SECONDS;
    UpdateStatusText();
    ShowTrayNotification(APP_NAME, L"Take a 20 second break now.");
    InvalidateRect(g_hwnd, NULL, FALSE);
    ShowReminderWindow();
}

static void FinishBreak(void)
{
    g_in_break = FALSE;
    g_break_seconds_left = 0;
    g_seconds_until_break = BREAK_INTERVAL_SECONDS;
    UpdateStatusText();
    InvalidateRect(g_hwnd, NULL, FALSE);
    ShowWindow(g_hwnd, SW_HIDE);
}

static void AddTrayIcon(HWND hwnd)
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
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void RemoveTrayIcon(void)
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

static void ShowTrayMenu(HWND hwnd)
{
    POINT cursor;
    HMENU menu = CreatePopupMenu();

    AppendMenuW(menu, MF_STRING, ID_TRAY_SHOW, L"Open");
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd, NULL);
    DestroyMenu(menu);
}

static void PaintBackground(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rect;
    HDC hdc = BeginPaint(hwnd, &ps);
    HDC memory_dc = CreateCompatibleDC(hdc);
    HBITMAP memory_bitmap;
    HBITMAP old_bitmap;
    HBRUSH background = CreateSolidBrush(RGB(245, 247, 250));
    HBRUSH panel = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH accent = CreateSolidBrush(g_in_break ? RGB(28, 132, 87) : RGB(36, 99, 185));
    HBRUSH icon_fill = CreateSolidBrush(g_in_break ? RGB(228, 247, 238) : RGB(230, 239, 255));
    HBRUSH progress_fill = CreateSolidBrush(g_in_break ? RGB(28, 132, 87) : RGB(36, 99, 185));
    HBRUSH progress_track = CreateSolidBrush(RGB(228, 233, 239));
    HPEN border = CreatePen(PS_SOLID, 1, RGB(215, 222, 230));
    HPEN accent_pen = CreatePen(PS_SOLID, 4, g_in_break ? RGB(28, 132, 87) : RGB(36, 99, 185));
    HFONT small_font = CreateUiFont(9, FW_SEMIBOLD);
    HFONT old_font;
    wchar_t label[64];
    int progress_width;
    int total_seconds = g_in_break ? BREAK_DURATION_SECONDS : BREAK_INTERVAL_SECONDS;
    int current_seconds = g_in_break ? g_break_seconds_left : g_seconds_until_break;

    GetClientRect(hwnd, &rect);
    memory_bitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
    old_bitmap = (HBITMAP)SelectObject(memory_dc, memory_bitmap);

    FillRect(memory_dc, &rect, background);

    RECT panel_rect = { 18, 18, rect.right - 18, rect.bottom - 18 };
    SelectObject(memory_dc, panel);
    SelectObject(memory_dc, border);
    RoundRect(memory_dc, panel_rect.left, panel_rect.top, panel_rect.right, panel_rect.bottom, 14, 14);

    RECT accent_rect = { panel_rect.left, panel_rect.top, panel_rect.right, panel_rect.top + 7 };
    FillRect(memory_dc, &accent_rect, accent);

    SelectObject(memory_dc, icon_fill);
    SelectObject(memory_dc, accent_pen);
    Ellipse(memory_dc, 42, 42, 92, 92);
    DrawIconEx(memory_dc, 55, 55, g_app_icon, 24, 24, 0, NULL, DI_NORMAL);

    RECT progress_track_rect = { 50, 223, rect.right - 50, 231 };
    FillRect(memory_dc, &progress_track_rect, progress_track);
    progress_width = (progress_track_rect.right - progress_track_rect.left) * current_seconds / total_seconds;
    if (progress_width < 0) {
        progress_width = 0;
    }
    RECT progress_fill_rect = progress_track_rect;
    progress_fill_rect.right = progress_track_rect.left + progress_width;
    FillRect(memory_dc, &progress_fill_rect, progress_fill);

    SetBkMode(memory_dc, TRANSPARENT);
    SetTextColor(memory_dc, g_in_break ? RGB(28, 132, 87) : RGB(36, 99, 185));
    old_font = (HFONT)SelectObject(memory_dc, small_font);
    swprintf(label, ARRAY_COUNT(label), L"%s", g_in_break ? L"BREAK IN PROGRESS" : L"BACKGROUND REMINDER");
    RECT label_rect = { 104, 48, rect.right - 42, 68 };
    DrawTextW(memory_dc, label, -1, &label_rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    SelectObject(memory_dc, old_font);

    BitBlt(hdc, 0, 0, rect.right, rect.bottom, memory_dc, 0, 0, SRCCOPY);

    DeleteObject(background);
    DeleteObject(panel);
    DeleteObject(accent);
    DeleteObject(icon_fill);
    DeleteObject(progress_fill);
    DeleteObject(progress_track);
    DeleteObject(border);
    DeleteObject(accent_pen);
    DeleteObject(small_font);
    SelectObject(memory_dc, old_bitmap);
    DeleteObject(memory_bitmap);
    DeleteDC(memory_dc);
    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_CREATE: {
        HFONT title_font = CreateUiFont(18, FW_SEMIBOLD);
        HFONT timer_font = CreateUiFont(48, FW_BOLD);
        HFONT body_font = CreateUiFont(10, FW_NORMAL);

        g_title = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
            104, 69, 270, 32, hwnd, NULL, NULL, NULL);
        g_countdown = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
            38, 104, 354, 76, hwnd, NULL, NULL, NULL);
        g_status = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
            44, 181, 342, 28, hwnd, NULL, NULL, NULL);
        g_skip_button = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            132, 248, 166, 36, hwnd, (HMENU)ID_TRAY_SHOW, NULL, NULL);

        SendMessageW(g_title, WM_SETFONT, (WPARAM)title_font, TRUE);
        SendMessageW(g_countdown, WM_SETFONT, (WPARAM)timer_font, TRUE);
        SendMessageW(g_status, WM_SETFONT, (WPARAM)body_font, TRUE);
        SendMessageW(g_skip_button, WM_SETFONT, (WPARAM)body_font, TRUE);

        AddTrayIcon(hwnd);
        SetTimer(hwnd, ID_TIMER_TICK, 1000, NULL);
        UpdateStatusText();
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == ID_TRAY_EXIT) {
            g_exit_requested = TRUE;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wparam) == ID_TRAY_SHOW) {
            if (g_in_break) {
                FinishBreak();
            } else {
                StartBreak();
            }
            return 0;
        }
        break;
    case WM_TIMER:
        if (wparam == ID_TIMER_TICK) {
            if (g_in_break) {
                g_break_seconds_left--;
                if (g_break_seconds_left <= 0) {
                    FinishBreak();
                } else {
                    UpdateStatusText();
                }
            } else {
                g_seconds_until_break--;
                if (g_seconds_until_break <= 0) {
                    StartBreak();
                } else {
                    UpdateStatusText();
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        break;
    case WM_TRAYICON:
        if (lparam == WM_LBUTTONDBLCLK) {
            ShowReminderWindow();
            return 0;
        }
        if (lparam == WM_RBUTTONUP) {
            ShowTrayMenu(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        if (g_exit_requested) {
            DestroyWindow(hwnd);
        } else {
            ShowWindow(hwnd, SW_HIDE);
        }
        return 0;
    case WM_PAINT:
        PaintBackground(hwnd);
        return 0;
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wparam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(23, 32, 42));
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_TICK);
        RemoveTrayIcon();
        if (g_app_icon) {
            DestroyIcon(g_app_icon);
        }
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
    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    WNDCLASSW wc;
    MSG msg;

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
        g_app_icon = LoadIcon(NULL, IDI_INFORMATION);
    }

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = g_app_icon;

    if (!RegisterClassW(&wc)) {
        return 1;
    }

    g_hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName,
        APP_NAME,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 430, 345,
        NULL, NULL, instance, NULL);

    if (!g_hwnd) {
        return 1;
    }

    ShowWindow(g_hwnd, SW_HIDE);

    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
