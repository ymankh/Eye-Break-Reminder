using Microsoft.UI.Xaml;
using Microsoft.Windows.AppLifecycle;
using System.Diagnostics;

namespace BreakReminderDotNet10;

public partial class App : Application
{
    private const string SingleInstanceKey = "BreakReminderDotNet10.SingleInstance";

    private AppInstance? _mainInstance;
    private MainWindow? _mainWindow;

    public App()
    {
        InitializeComponent();
    }

    protected override async void OnLaunched(LaunchActivatedEventArgs args)
    {
        _mainInstance = AppInstance.FindOrRegisterForKey(SingleInstanceKey);
        if (!_mainInstance.IsCurrent)
        {
            AppActivationArguments activatedArgs = AppInstance.GetCurrent().GetActivatedEventArgs();
            await _mainInstance.RedirectActivationToAsync(activatedArgs);
            Process.GetCurrentProcess().Kill();
            return;
        }

        _mainInstance.Activated += OnMainInstanceActivated;

        _mainWindow = new MainWindow();
        _mainWindow.InitializeBackgroundMode();
    }

    private void OnMainInstanceActivated(object? sender, AppActivationArguments args)
    {
        _mainWindow?.DispatcherQueue.TryEnqueue(() => _mainWindow.ShowReminderWindow());
    }
}
