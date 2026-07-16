#pragma once

#include "MainWindow.g.h"
#include "core/reminder_core.h"

namespace winrt::BreakReminderWinUI::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        ~MainWindow();

        void PrimaryButton_Click(
            winrt::Windows::Foundation::IInspectable const&,
            Microsoft::UI::Xaml::RoutedEventArgs const&);

    private:
        ReminderState m_state{};
        Microsoft::UI::Xaml::DispatcherTimer m_timer{};
        HWND m_hwnd{};
        NOTIFYICONDATAW m_trayIcon{};
        UINT m_taskbarCreatedMessage{};
        bool m_subclassInstalled{ false };
        bool m_trayIconAdded{ false };
        bool m_exitRequested{ false };

        void OnTimerTick(
            winrt::Windows::Foundation::IInspectable const&,
            winrt::Windows::Foundation::IInspectable const&);
        void UpdateView();
        void ShowBreakWindow();
        void ConfigureWindow();
        bool AddTrayIcon();
        void RemoveTrayIcon();
        void ShowTrayMenu();
        void ShowWindowFromTray();
        void OnWindowClosing(
            Microsoft::UI::Windowing::AppWindow const&,
            Microsoft::UI::Windowing::AppWindowClosingEventArgs const& args);
        static LRESULT CALLBACK WindowSubclassProc(
            HWND hwnd,
            UINT message,
            WPARAM wparam,
            LPARAM lparam,
            UINT_PTR subclassId,
            DWORD_PTR referenceData);
    };
}

namespace winrt::BreakReminderWinUI::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
