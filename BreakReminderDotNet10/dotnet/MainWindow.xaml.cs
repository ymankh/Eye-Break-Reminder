using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using System.Runtime.InteropServices;
using Windows.Graphics;
using Microsoft.UI;
using WinRT.Interop;
using WinUIApplication = Microsoft.UI.Xaml.Application;

namespace BreakReminderDotNet10;

public sealed partial class MainWindow : Window
{
    private const int BreakIntervalSeconds = 20 * 60;
    private const int BreakDurationSeconds = 20;
    private const int InitialWindowWidth = 420;
    private const int InitialWindowHeight = 320;
    private const int MinimumWindowWidth = 360;
    private const int MinimumWindowHeight = 280;
    private const int SwRestore = 9;

    private readonly DispatcherTimer _timer = new() { Interval = TimeSpan.FromSeconds(1) };
    private bool _allowClose;
    private bool _windowInitialized;
    private int _secondsUntilBreak = BreakIntervalSeconds;
    private int _breakSecondsLeft;
    private bool _isBreakActive;
    private TrayIcon? _trayIcon;

    public MainWindow()
    {
        InitializeComponent();

        Title = "20-20 Break";
        SystemBackdrop = new MicaBackdrop();
        ExtendsContentIntoTitleBar = true;
        SetTitleBar(AppTitleBar);

        Closed += OnClosed;
        _timer.Tick += OnTick;
        _timer.Start();
        UpdateView();
    }

    public void InitializeBackgroundMode()
    {
        EnsureWindowInitialized();
        AppWindow.Hide();
    }

    public void ShowReminderWindow()
    {
        EnsureWindowInitialized();
        AppWindow.Show();
        Activate();
        BringToFront();
    }

    private void EnsureWindowInitialized()
    {
        if (_windowInitialized)
        {
            return;
        }

        _windowInitialized = true;
        Activate();
        ConfigureTitleBar();
        AppWindow.Resize(new SizeInt32(InitialWindowWidth, InitialWindowHeight));
        AppWindow.Closing += OnAppWindowClosing;

        if (AppWindow.Presenter is OverlappedPresenter presenter)
        {
            presenter.IsResizable = true;
            presenter.PreferredMinimumWidth = MinimumWindowWidth;
            presenter.PreferredMinimumHeight = MinimumWindowHeight;
        }

        _trayIcon = new TrayIcon(WindowNative.GetWindowHandle(this), "20-20 Break", ShowReminderWindow, StartBreak, ExitApplication);
    }

    private void ConfigureTitleBar()
    {
        if (!AppWindowTitleBar.IsCustomizationSupported())
        {
            return;
        }

        AppWindowTitleBar titleBar = AppWindow.TitleBar;
        titleBar.PreferredHeightOption = TitleBarHeightOption.Tall;
        titleBar.ButtonBackgroundColor = Colors.Transparent;
        titleBar.ButtonInactiveBackgroundColor = Colors.Transparent;
    }

    private void OnTick(object? sender, object e)
    {
        if (_isBreakActive)
        {
            _breakSecondsLeft--;
            if (_breakSecondsLeft <= 0)
            {
                FinishBreak();
                return;
            }

            UpdateView();
            return;
        }

        _secondsUntilBreak--;
        if (_secondsUntilBreak <= 0)
        {
            StartBreak();
            return;
        }

        UpdateView();
    }

    private void StartBreak()
    {
        if (_isBreakActive)
        {
            return;
        }

        _isBreakActive = true;
        _breakSecondsLeft = BreakDurationSeconds;
        ShowBalloon("20-20 Break", "Look away for 20 seconds.");
        ShowReminderWindow();
        UpdateView();
    }

    private void FinishBreak()
    {
        if (!_isBreakActive)
        {
            return;
        }

        _isBreakActive = false;
        _secondsUntilBreak = BreakIntervalSeconds;
        AppWindow.Hide();
        UpdateView();
    }

    private void UpdateView()
    {
        int current = _isBreakActive ? _breakSecondsLeft : _secondsUntilBreak;
        int total = _isBreakActive ? BreakDurationSeconds : BreakIntervalSeconds;
        string timeRemaining = TimeSpan.FromSeconds(current).ToString(@"mm\:ss");

        CountdownText.Text = timeRemaining;
        CountdownProgress.Value = Math.Clamp((double)current / total, 0, 1) * 100;

        if (_isBreakActive)
        {
            ModeText.Text = "Break";
            StatusText.Text = $"{timeRemaining} remaining. Look away from the screen.";
            ActionButton.Content = "Finish";
            return;
        }

        ModeText.Text = "Active";
        StatusText.Text = $"Next break in {timeRemaining}.";
        ActionButton.Content = "Start break";
    }

    private void ShowBalloon(string title, string message)
    {
        _trayIcon?.ShowBalloon(title, message);
    }

    private void ActionButton_Click(object sender, RoutedEventArgs e)
    {
        if (_isBreakActive)
        {
            FinishBreak();
            return;
        }

        StartBreak();
    }

    private void HideButton_Click(object sender, RoutedEventArgs e)
    {
        AppWindow.Hide();
    }

    private void OnAppWindowClosing(AppWindow sender, AppWindowClosingEventArgs args)
    {
        if (_allowClose)
        {
            return;
        }

        args.Cancel = true;
        sender.Hide();
    }

    private void ExitApplication()
    {
        _allowClose = true;
        _timer.Stop();
        DisposeTrayIcon();
        Close();
        WinUIApplication.Current.Exit();
    }

    private void OnClosed(object sender, WindowEventArgs args)
    {
        _timer.Stop();
        DisposeTrayIcon();
    }

    private void DisposeTrayIcon()
    {
        _trayIcon?.Dispose();
        _trayIcon = null;
    }

    private void BringToFront()
    {
        nint hwnd = WindowNative.GetWindowHandle(this);
        ShowWindow(hwnd, SwRestore);
        SetForegroundWindow(hwnd);
    }

    [DllImport("user32.dll")]
    private static extern bool ShowWindow(nint hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(nint hWnd);
}
