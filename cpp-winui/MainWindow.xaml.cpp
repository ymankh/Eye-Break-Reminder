#include "pch.h"
#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "microsoft.ui.xaml.window.h"

namespace
{
    constexpr UINT TrayIconMessage = WM_APP + 1;
    constexpr UINT TrayCommandOpen = 4001;
    constexpr UINT TrayCommandStartBreak = 4002;
    constexpr UINT TrayCommandExit = 4003;
}

namespace winrt::BreakReminderWinUI::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        Title(L"20-20 Break");
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(TitleBarDragRegion());
        SystemBackdrop(Microsoft::UI::Xaml::Media::MicaBackdrop{});

        ReminderCore_Initialize(&m_state);
        ConfigureWindow();
        if (m_subclassInstalled)
        {
            m_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
            AddTrayIcon();
        }
        AppWindow().Closing({ this, &MainWindow::OnWindowClosing });
        UpdateView();

        m_timer = Microsoft::UI::Xaml::DispatcherTimer{};
        m_timer.Interval(std::chrono::seconds(1));
        m_timer.Tick({ this, &MainWindow::OnTimerTick });
        m_timer.Start();
    }

    MainWindow::~MainWindow()
    {
        RemoveTrayIcon();
        if (m_subclassInstalled)
        {
            RemoveWindowSubclass(m_hwnd, WindowSubclassProc, 1);
        }
    }

    void MainWindow::PrimaryButton_Click(
        winrt::Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        if (ReminderCore_IsBreakActive(&m_state))
        {
            ReminderCore_FinishBreak(&m_state);
        }
        else
        {
            ReminderCore_StartBreak(&m_state);
        }
        UpdateView();
    }

    void MainWindow::OnTimerTick(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Windows::Foundation::IInspectable const&)
    {
        ReminderEvent const event = ReminderCore_Tick(&m_state);
        UpdateView();
        if (event == REMINDER_EVENT_BREAK_STARTED)
        {
            ShowBreakWindow();
        }
    }

    void MainWindow::UpdateView()
    {
        wchar_t timeText[16]{};
        ReminderCore_FormatTime(ReminderCore_SecondsRemaining(&m_state), timeText, _countof(timeText));
        CountdownText().Text(timeText);
        CountdownProgress().Value(ReminderCore_ProgressPercent(&m_state));

        if (ReminderCore_IsBreakActive(&m_state))
        {
            ModeLabel().Text(L"BREAK IN PROGRESS");
            HeadingText().Text(L"Give your eyes a reset");
            BodyText().Text(L"Look about 20 feet away, relax your shoulders, and blink naturally.");
            DetailText().Text(L"The reminder automatically resets when the countdown ends.");
            PrimaryButton().Content(winrt::box_value(L"Finish break"));
        }
        else
        {
            ModeLabel().Text(L"REMINDER ACTIVE");
            HeadingText().Text(L"Your next eye break");
            BodyText().Text(L"Every 20 minutes, look at something 20 feet away for 20 seconds.");
            DetailText().Text(L"The timer keeps counting while this window is open.");
            PrimaryButton().Content(winrt::box_value(L"Start break now"));
        }
    }

    void MainWindow::ShowBreakWindow()
    {
        ShowWindowFromTray();
    }

    void MainWindow::ConfigureWindow()
    {
        Microsoft::UI::Xaml::Window window = *this;
        window.as<::IWindowNative>()->get_WindowHandle(&m_hwnd);
        UINT const dpi = GetDpiForWindow(m_hwnd);
        int const width = MulDiv(720, static_cast<int>(dpi), 96);
        int const height = MulDiv(560, static_cast<int>(dpi), 96);
        SetWindowPos(m_hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        m_subclassInstalled = SetWindowSubclass(
            m_hwnd,
            WindowSubclassProc,
            1,
            reinterpret_cast<DWORD_PTR>(this)) != FALSE;
    }

    bool MainWindow::AddTrayIcon()
    {
        m_trayIcon.cbSize = sizeof(m_trayIcon);
        m_trayIcon.hWnd = m_hwnd;
        m_trayIcon.uID = 1;
        m_trayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
        m_trayIcon.uCallbackMessage = TrayIconMessage;
        m_trayIcon.hIcon = reinterpret_cast<HICON>(SendMessageW(m_hwnd, WM_GETICON, ICON_SMALL2, 0));
        if (m_trayIcon.hIcon == nullptr)
        {
            m_trayIcon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        }
        wcscpy_s(m_trayIcon.szTip, L"20-20 Break");
        m_trayIconAdded = Shell_NotifyIconW(NIM_ADD, &m_trayIcon) != FALSE;
        if (!m_trayIconAdded)
        {
            return false;
        }

        m_trayIcon.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &m_trayIcon);
        return true;
    }

    void MainWindow::RemoveTrayIcon()
    {
        if (m_trayIconAdded)
        {
            Shell_NotifyIconW(NIM_DELETE, &m_trayIcon);
            m_trayIconAdded = false;
        }
    }

    void MainWindow::ShowTrayMenu()
    {
        POINT cursor{};
        GetCursorPos(&cursor);
        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, TrayCommandOpen, L"Open");
        AppendMenuW(menu, MF_STRING, TrayCommandStartBreak, L"Start break now");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, TrayCommandExit, L"Exit");
        SetForegroundWindow(m_hwnd);
        TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, m_hwnd, nullptr);
        DestroyMenu(menu);
    }

    void MainWindow::ShowWindowFromTray()
    {
        ShowWindow(m_hwnd, SW_RESTORE);
        Activate();
        SetForegroundWindow(m_hwnd);
    }

    void MainWindow::OnWindowClosing(
        Microsoft::UI::Windowing::AppWindow const&,
        Microsoft::UI::Windowing::AppWindowClosingEventArgs const& args)
    {
        if (!m_exitRequested && m_trayIconAdded)
        {
            args.Cancel(true);
            ShowWindow(m_hwnd, SW_HIDE);
        }
    }

    LRESULT CALLBACK MainWindow::WindowSubclassProc(
        HWND hwnd,
        UINT message,
        WPARAM wparam,
        LPARAM lparam,
        UINT_PTR,
        DWORD_PTR referenceData)
    {
        auto* self = reinterpret_cast<MainWindow*>(referenceData);
        if (self == nullptr)
        {
            return DefSubclassProc(hwnd, message, wparam, lparam);
        }

        if (self->m_taskbarCreatedMessage != 0 && message == self->m_taskbarCreatedMessage)
        {
            self->m_trayIconAdded = false;
            self->AddTrayIcon();
            return 0;
        }

        if (message == TrayIconMessage)
        {
            UINT const trayEvent = LOWORD(lparam);
            if (trayEvent == WM_LBUTTONDBLCLK || trayEvent == NIN_SELECT)
            {
                self->ShowWindowFromTray();
                return 0;
            }
            if (trayEvent == WM_CONTEXTMENU || trayEvent == WM_RBUTTONUP)
            {
                self->ShowTrayMenu();
                return 0;
            }
        }

        if (message == WM_COMMAND)
        {
            switch (LOWORD(wparam))
            {
            case TrayCommandOpen:
                self->ShowWindowFromTray();
                return 0;
            case TrayCommandStartBreak:
                ReminderCore_StartBreak(&self->m_state);
                self->UpdateView();
                self->ShowWindowFromTray();
                return 0;
            case TrayCommandExit:
                self->m_exitRequested = true;
                self->RemoveTrayIcon();
                self->Close();
                return 0;
            default:
                break;
            }
        }

        return DefSubclassProc(hwnd, message, wparam, lparam);
    }
}
