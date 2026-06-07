# 20-20 Break for .NET 10

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

## Publish a single-file executable

```powershell
dotnet publish -c Release -r win-x64 --self-contained true -p:WindowsAppSDKSelfContained=true -p:PublishSingleFile=true -p:DebugSymbols=false -p:DebugType=None -o publish/win-x64-single-file
```

Published output:

- `publish/win-x64-single-file/BreakReminderDotNet10.exe`

## Build the WiX installer locally

```powershell
dotnet build installer/BreakReminderDotNet10.Installer.wixproj -c Release -p:InstallerVersion=1.0.0 -p:AppPublishDir="$PWD\publish\win-x64-single-file"
```

Installer output:

- `installer/bin/x64/Release/BreakReminderDotNet10-1.0.0-x64.msi`

## GitHub release automation

The repository includes `.github/workflows/release-installer.yml`.

- Publishing a GitHub release builds the single-file app
- builds the WiX MSI
- uploads the MSI to the release assets
- uploads the MSI as a workflow artifact

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
- The published single-file executable self-extracts its required WinUI files at runtime.
- The WiX project uses `WixToolset.Sdk` `7.0.0` and accepts the `wix7` EULA in the installer project file.
