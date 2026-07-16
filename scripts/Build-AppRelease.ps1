param(
    [Parameter(Mandatory = $true)]
    [string]$InstallerVersion,

    [string]$Configuration = 'Release',

    [string]$RuntimeIdentifier = 'win-x64'
)

$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSVersion.Major -ge 7) {
    $PSNativeCommandUseErrorActionPreference = $true
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$appProjectPath = Join-Path $repoRoot 'dotnet/BreakReminderDotNet10.csproj'
$installerProjectPath = Join-Path $repoRoot 'dotnet/installer/BreakReminderDotNet10.Installer.wixproj'
$bundleProjectPath = Join-Path $repoRoot 'dotnet/installer/BreakReminderDotNet10.Bundle.wixproj'
$runtimeDir = Join-Path $repoRoot 'dotnet/artifacts/prerequisites'
$dotNetRuntimePath = Join-Path $runtimeDir 'windowsdesktop-runtime-win-x64.exe'
$windowsAppRuntimePath = Join-Path $runtimeDir 'windowsappruntimeinstall-x64.exe'
$publishDir = Join-Path $repoRoot 'dotnet/artifacts/publish/win-x64-framework-dependent'
$installerOutputDir = Join-Path $repoRoot 'dotnet/installer/bin/x64/Release'
$installerObjDir = Join-Path $repoRoot 'dotnet/installer/obj'

foreach ($projectPath in @($appProjectPath, $installerProjectPath, $bundleProjectPath)) {
    if (-not (Test-Path $projectPath)) {
        throw "Required project file is missing: $projectPath"
    }
}

function Save-PrerequisiteIfMissing {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Uri
    )

    if (Test-Path $Path) {
        return
    }

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
    Invoke-WebRequest -Uri $Uri -OutFile $Path
}

foreach ($path in @($publishDir, $installerOutputDir, $installerObjDir)) {
    if (Test-Path $path) {
        Remove-Item -Recurse -Force $path
    }
}

dotnet restore $appProjectPath
dotnet restore $installerProjectPath
dotnet restore $bundleProjectPath

dotnet publish $appProjectPath `
    -c $Configuration `
    -r $RuntimeIdentifier `
    --self-contained false `
    -p:SelfContained=false `
    -p:WindowsAppSDKSelfContained=false `
    -p:PublishSingleFile=false `
    -p:DebugSymbols=false `
    -p:DebugType=None `
    -p:Version=$InstallerVersion `
    -o $publishDir

$appExe = Get-ChildItem -Path $publishDir -Filter '20-20 Break.exe' | Select-Object -First 1
if (-not $appExe) {
    throw 'No app executable was produced by dotnet publish.'
}

dotnet build $installerProjectPath `
    -c $Configuration `
    -p:InstallerVersion=$InstallerVersion `
    -p:AppPublishDir="$publishDir"

$msi = Get-ChildItem -Path $installerOutputDir -Filter *.msi |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if (-not $msi) {
    throw 'No MSI was produced by the WiX build.'
}
Save-PrerequisiteIfMissing `
    -Path $dotNetRuntimePath `
    -Uri 'https://aka.ms/dotnet/10.0/windowsdesktop-runtime-win-x64.exe'
Save-PrerequisiteIfMissing `
    -Path $windowsAppRuntimePath `
    -Uri 'https://aka.ms/windowsappsdk/2.1/2.1.3/windowsappruntimeinstall-x64.exe'

dotnet build $bundleProjectPath `
    -c $Configuration `
    -p:InstallerVersion=$InstallerVersion `
    -p:InstallerMsiPath="$($msi.FullName)" `
    -p:DotNetDesktopRuntimeExePath="$dotNetRuntimePath" `
    -p:WindowsAppRuntimeExePath="$windowsAppRuntimePath"

$setup = Get-ChildItem -Path $installerOutputDir -Filter '20-20 Break Setup-*.exe' |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if (-not $setup) {
    throw 'No installer bootstrapper executable was produced by the WiX bundle build.'
}

if ($env:GITHUB_OUTPUT) {
    "app_exe_path=$($appExe.FullName)" >> $env:GITHUB_OUTPUT
    "app_exe_name=$($appExe.Name)" >> $env:GITHUB_OUTPUT
    "app_publish_path=$publishDir" >> $env:GITHUB_OUTPUT
    "msi_path=$($msi.FullName)" >> $env:GITHUB_OUTPUT
    "msi_name=$($msi.Name)" >> $env:GITHUB_OUTPUT
    "setup_path=$($setup.FullName)" >> $env:GITHUB_OUTPUT
    "setup_name=$($setup.Name)" >> $env:GITHUB_OUTPUT
}

[pscustomobject]@{
    AppExePath = $appExe.FullName
    AppExeName = $appExe.Name
    AppPublishPath = $publishDir
    MsiPath = $msi.FullName
    MsiName = $msi.Name
    SetupPath = $setup.FullName
    SetupName = $setup.Name
}
