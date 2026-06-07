# Break Reminder

Break Reminder is a small Windows desktop app that reminds you to take a 20 second screen break every 20 minutes. The main implementation lives in `dotnet/` and uses .NET 10, WinUI 3, and Windows App SDK. A small `c/` project is also included as a native C scaffold.

## Features

- Runs as a single-instance Windows app.
- Starts hidden in the system tray.
- Shows a reminder every 20 minutes.
- Runs a 20 second break countdown.
- Lets you start or finish a break manually.
- Keeps running when the window is hidden or closed.
- Provides tray actions for opening the window, starting a break, and exiting.
- Supports single-file publishing and WiX MSI packaging.

## Repository layout

```text
.
├── dotnet/
│   ├── App.xaml
│   ├── MainWindow.xaml
│   ├── TrayIcon.cs
│   ├── BreakReminderDotNet10.csproj
│   ├── installer/
│   │   ├── BreakReminderDotNet10.Installer.wixproj
│   │   └── Package.wxs
│   └── README.md
├── c/
│   ├── CMakeLists.txt
│   ├── include/
│   ├── tests/
│   ├── main.c
│   ├── resource.h
│   ├── break-reminder.rc
│   └── app.ico
└── .github/workflows/
    ├── release-installer.yml
    └── release-c.yml
```

## Requirements

- Windows 10 version 19041 or newer for the WinUI app.
- .NET 10 SDK for building and running the .NET project.
- CMake 3.21 or newer for the C scaffold.
- A C compiler such as MSVC, Clang, or GCC for the C scaffold.

## Build the .NET app

Run these commands from the repository root:

```powershell
cd dotnet
dotnet build
```

To run the app during development:

```powershell
dotnet run
```

The app starts in the tray. Double-click the tray icon to open the window, or right-click it for the context menu.

## Publish the .NET app

From the `dotnet/` folder:

```powershell
dotnet publish -c Release -r win-x64 --self-contained true -p:WindowsAppSDKSelfContained=true -p:PublishSingleFile=true -p:DebugSymbols=false -p:DebugType=None -o publish/win-x64-single-file
```

Published executable:

```text
dotnet/publish/win-x64-single-file/BreakReminderDotNet10.exe
```

## Build the installer

Publish the app first, then build the WiX installer from the `dotnet/` folder:

```powershell
dotnet build installer/BreakReminderDotNet10.Installer.wixproj -c Release -p:InstallerVersion=1.0.0 -p:AppPublishDir="$PWD\publish\win-x64-single-file"
```

Installer output:

```text
dotnet/installer/bin/x64/Release/BreakReminderDotNet10-1.0.0-x64.msi
```

## Build the C app

The native C app can be built independently through the `c/` CMake project:

```powershell
cd c
cmake -S . -B build
cmake --build build
```

Generated executable:

```text
c/build/Release/break_reminder_c.exe
```

## GitHub release workflow

The repository includes two release workflows:

- `.github/workflows/release-installer.yml`
- `.github/workflows/release-c.yml`

When a GitHub release is published, the workflow:

- builds the .NET app as a single-file Windows executable,
- builds the WiX MSI installer,
- uploads the MSI to the release assets,
- uploads the MSI as a workflow artifact,
- builds the C executable,
- uploads the C executable to the release assets,
- uploads the C executable as a workflow artifact.

Use release tags in `vMajor.Minor.Patch` form, such as `v1.2.3`. The .NET installer workflow can also be run manually with a `Major.Minor.Patch` version value. The C workflow can also be run manually without inputs.

## Installer behavior

The .NET MSI installer:

- checks for 64-bit Windows 10 or newer,
- lets the user choose the installation folder,
- installs the app under `C:\Program Files\20-20 Break\` by default,
- creates Start Menu and Desktop shortcuts,
- registers the app to start when Windows launches.

The release workflow publishes the .NET app with `--self-contained true` and `WindowsAppSDKSelfContained=true`, so users do not need to install the .NET runtime or Windows App SDK runtime separately.

## Generated files

Generated outputs are ignored by `.gitignore` and `.ignore`, including:

- `dotnet/bin/`
- `dotnet/obj/`
- `dotnet/publish/`
- `dotnet/installer/bin/`
- `dotnet/installer/obj/`
- `c/build/`
- binary and packaging outputs such as `*.exe`, `*.dll`, `*.pdb`, `*.msi`, and `*.wixpdb`

## More details

The .NET project has its own focused notes in `dotnet/README.md`, including the app behavior, publishing command, installer command, and release automation details.
