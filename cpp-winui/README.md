# Native WinUI 3 variant

This independent native version keeps the reminder state machine in C under `core/`. The window uses C++/WinRT, WinUI 3, Fluent controls, theme resources, and a Mica backdrop.

Closing the window keeps the timer running in the system tray. Double-click the tray icon to reopen the window, or use its context menu to start a break or exit.

## Requirements

- Windows 10 version 1809 or later; Windows 11 is recommended.
- Visual Studio 2026 with **WinUI application development**, **C++ WinUI app development tools**, and **C++ Universal Windows Platform tools (Latest MSVC)**.
- Developer Mode enabled in Windows.
- Windows App Runtime 2.2 x64 installed on the target computer.

Visual Studio can import `.vsconfig` from this folder to install the three required components.

## Build

```powershell
./cpp-winui/Build-Release.ps1
```

Output: `cpp-winui/artifacts/Release/x64/BreakReminderWinUI.exe`.

The output is framework-dependent to keep the complete app folder under 1 MB. Keep all generated files beside the executable when distributing it, and install Windows App Runtime 2.2 x64 separately on target computers.

After building, launch the executable and verify that the window opens. Close the window, confirm the tray icon remains available, reopen it from the icon, and choose **Exit** from the tray menu.

## Test the C reminder core

```powershell
./cpp-winui/Test-Core.ps1
```
