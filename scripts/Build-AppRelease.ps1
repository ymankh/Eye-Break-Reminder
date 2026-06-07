param(
    [Parameter(Mandatory = $true)]
    [string]$InstallerVersion,

    [string]$Configuration = 'Release',

    [string]$RuntimeIdentifier = 'win-x64'
)

$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$publishDir = Join-Path $repoRoot 'dotnet/artifacts/publish/win-x64-single-file'
$runtimeDir = Join-Path $repoRoot 'dotnet/artifacts/prerequisites'
$runtimePath = Join-Path $runtimeDir 'windowsdesktop-runtime-win-x64.exe'

dotnet restore (Join-Path $repoRoot 'dotnet/BreakReminderDotNet10.csproj')
dotnet restore (Join-Path $repoRoot 'dotnet/installer/BreakReminderDotNet10.Installer.wixproj')
dotnet restore (Join-Path $repoRoot 'dotnet/installer/BreakReminderDotNet10.Bundle.wixproj')

dotnet publish (Join-Path $repoRoot 'dotnet/BreakReminderDotNet10.csproj') `
    -c $Configuration `
    -r $RuntimeIdentifier `
    --self-contained false `
    -p:SelfContained=false `
    -p:WindowsAppSDKSelfContained=true `
    -p:PublishSingleFile=true `
    -p:DebugSymbols=false `
    -p:DebugType=None `
    -p:Version=$InstallerVersion `
    -o $publishDir

$appExe = Get-ChildItem -Path $publishDir -Filter '20-20 Break.exe' | Select-Object -First 1
if (-not $appExe) {
    throw 'No app executable was produced by dotnet publish.'
}

dotnet build (Join-Path $repoRoot 'dotnet/installer/BreakReminderDotNet10.Installer.wixproj') `
    -c $Configuration `
    -p:InstallerVersion=$InstallerVersion `
    -p:AppPublishDir="$publishDir"

$msi = Get-ChildItem -Path (Join-Path $repoRoot 'dotnet/installer/bin/x64/Release') -Filter *.msi |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if (-not $msi) {
    throw 'No MSI was produced by the WiX build.'
}

New-Item -ItemType Directory -Force -Path $runtimeDir | Out-Null
Invoke-WebRequest `
    -Uri 'https://aka.ms/dotnet/10.0/windowsdesktop-runtime-win-x64.exe' `
    -OutFile $runtimePath

dotnet build (Join-Path $repoRoot 'dotnet/installer/BreakReminderDotNet10.Bundle.wixproj') `
    -c $Configuration `
    -p:InstallerVersion=$InstallerVersion `
    -p:InstallerMsiPath="$($msi.FullName)" `
    -p:DotNetDesktopRuntimeExePath="$runtimePath"

$setup = Get-ChildItem -Path (Join-Path $repoRoot 'dotnet/installer/bin/x64/Release') -Filter *.exe |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if (-not $setup) {
    throw 'No installer bootstrapper executable was produced by the WiX bundle build.'
}

if ($env:GITHUB_OUTPUT) {
    "app_exe_path=$($appExe.FullName)" >> $env:GITHUB_OUTPUT
    "app_exe_name=$($appExe.Name)" >> $env:GITHUB_OUTPUT
    "msi_path=$($msi.FullName)" >> $env:GITHUB_OUTPUT
    "msi_name=$($msi.Name)" >> $env:GITHUB_OUTPUT
    "setup_path=$($setup.FullName)" >> $env:GITHUB_OUTPUT
    "setup_name=$($setup.Name)" >> $env:GITHUB_OUTPUT
}

[pscustomobject]@{
    AppExePath = $appExe.FullName
    AppExeName = $appExe.Name
    MsiPath = $msi.FullName
    MsiName = $msi.Name
    SetupPath = $setup.FullName
    SetupName = $setup.Name
}
