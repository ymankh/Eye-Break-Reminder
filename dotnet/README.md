# 20-20 Break

A small native Windows break reminder built with .NET 10, WinUI 3, and Windows App SDK.

Run the commands below from the `dotnet/` folder.

## What it does

- runs as a single instance
- starts in the system tray
- reminds you every 20 minutes
- shows a 20 second break countdown
- lets you start a break immediately
- keeps running when the window is hidden or closed

## Requirements

- Windows 10 version 19041 or newer
- .NET 10 SDK for local development

## Build the app

```powershell
dotnet build
```

## Run during development

```powershell
dotnet run
```

## Publish the app files

```powershell
dotnet publish -c Release -r win-x64 --self-contained false -p:SelfContained=false -p:WindowsAppSDKSelfContained=false -p:PublishSingleFile=false -p:DebugSymbols=false -p:DebugType=None -o artifacts/publish/win-x64-framework-dependent
```

Published output:

- `artifacts/publish/win-x64-framework-dependent/`
- `20-20 Break.exe` plus its adjacent `.dll`, `.json`, and WinUI runtime bridge files. Keep the folder together; the executable is not a standalone app.

## Build the WiX installer locally

```powershell
dotnet build installer/BreakReminderDotNet10.Installer.wixproj -c Release -p:InstallerVersion=1.0.0 -p:AppPublishDir="$PWD\artifacts\publish\win-x64-framework-dependent"
```

Installer output:

- `installer/bin/x64/Release/20-20 Break-1.0.0-x64.msi`

## Build the setup bootstrapper locally

Build the MSI first, make sure the prerequisite installers exist under `artifacts/prerequisites/`, then run:

```powershell
dotnet build installer/BreakReminderDotNet10.Bundle.wixproj -c Release -p:InstallerVersion=1.0.0 -p:InstallerMsiPath="$PWD\installer\bin\x64\Release\20-20 Break-1.0.0-x64.msi" -p:DotNetDesktopRuntimeExePath="$PWD\artifacts\prerequisites\windowsdesktop-runtime-win-x64.exe" -p:WindowsAppRuntimeExePath="$PWD\artifacts\prerequisites\windowsappruntimeinstall-x64.exe"
```

Setup output:

- `installer/bin/x64/Release/20-20 Break Setup-1.0.0-x64.exe`

The same full .NET release build used by GitHub Actions can be run from the repository root:

```powershell
./scripts/Build-AppRelease.ps1 -InstallerVersion 1.0.0
```

The MSI installer:

- checks for 64-bit Windows 10 or newer
- shows feature selection
- lets the user choose the installation folder
- installs to `C:\Program Files\20-20 Break\` by default
- lets the user choose the Start Menu shortcut
- lets the user choose the Desktop shortcut
- lets the user choose whether the app starts automatically when Windows launches

Users should run the setup bootstrapper `.exe`. It downloads the .NET Desktop Runtime when missing, runs the Windows App Runtime prerequisite installer, then launches the MSI. The MSI installs a small framework-dependent app folder instead of a self-contained WinUI payload.

## GitHub release automation

The repository includes `.github/workflows/release-installer.yml`.

- Publishing a GitHub release builds the framework-dependent app folder
- builds the WiX MSI
- builds the setup bootstrapper `.exe`
- uploads the MSI and setup `.exe` to the release assets
- uploads the MSI and setup `.exe` as workflow artifacts

The workflow delegates build work to `scripts/Resolve-ReleaseVersion.ps1` and `scripts/Build-AppRelease.ps1` so the same commands can be run manually.

Use release tags in `vMajor.Minor.Patch` form, for example `v1.2.3`.

You can also run the workflow manually with the same `Major.Minor.Patch` version format.

## How to use the app

1. Launch the app.
2. It starts hidden in the system tray.
3. Double-click the tray icon to open the window.
4. Wait for the reminder, or press **Start break** to trigger one immediately.
5. Press **Hide** to return the app to the tray.
6. Right-click the tray icon and choose **Exit** to close the app completely.
7. Launching the app again brings the existing instance back instead of starting a second copy.

## Notes

- The window uses native WinUI controls and the active Windows theme.
- The installer uses external .NET Desktop Runtime and Windows App Runtime prerequisites to keep the MSI and installed app folder small.
- The WiX project uses `WixToolset.Sdk` `7.0.0` and accepts the `wix7` EULA in the installer project file.
