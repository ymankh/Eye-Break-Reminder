param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [ValidateSet('x64')]
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw 'Visual Studio Installer was not found. Install Visual Studio 2026 with C++ WinUI app development tools.'
}

$installationPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
if (-not $installationPath) { throw 'Visual Studio 2026 with MSBuild was not found.' }

$msbuild = Join-Path $installationPath 'MSBuild\Current\Bin\MSBuild.exe'
$project = Join-Path $PSScriptRoot 'BreakReminderWinUI.vcxproj'
$outputDirectory = Join-Path $PSScriptRoot "artifacts\$Configuration\$Platform"
$resolvedRoot = [System.IO.Path]::GetFullPath($PSScriptRoot) + [System.IO.Path]::DirectorySeparatorChar
$resolvedOutput = [System.IO.Path]::GetFullPath($outputDirectory)
if (-not $resolvedOutput.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean an output directory outside the project: $resolvedOutput"
}
if (Test-Path $resolvedOutput) {
    Remove-Item -LiteralPath $resolvedOutput -Recurse -Force
}

& $msbuild $project -restore -m -p:Configuration=$Configuration -p:Platform=$Platform
if ($LASTEXITCODE -ne 0) {
    throw "WinUI build failed with exit code $LASTEXITCODE."
}

$exe = Join-Path $outputDirectory 'BreakReminderWinUI.exe'
if (-not (Test-Path $exe)) {
    throw "The build completed without producing the expected executable: $exe"
}

$outputFiles = Get-ChildItem -LiteralPath $outputDirectory -File -Recurse
if ($Configuration -eq 'Release') {
    # WinUI's transitive package copies WebView2 even when the app has no WebView2 control.
    # Linker outputs are development artifacts, not runtime dependencies.
    foreach ($unusedFile in @(
        'Microsoft.Web.WebView2.Core.dll',
        'Microsoft.Web.WebView2.Core.winmd',
        'BreakReminderWinUI.lib',
        'BreakReminderWinUI.exp',
        'BreakReminderWinUI.pdb'
    )) {
        Remove-Item -LiteralPath (Join-Path $outputDirectory $unusedFile) -ErrorAction SilentlyContinue
    }

    $outputFiles = Get-ChildItem -LiteralPath $outputDirectory -File -Recurse
    $totalBytes = ($outputFiles | Measure-Object -Property Length -Sum).Sum
    if ($totalBytes -gt 1MB) {
        throw "The framework-dependent output is larger than 1 MB: $totalBytes bytes."
    }
} else {
    $totalBytes = ($outputFiles | Measure-Object -Property Length -Sum).Sum
}

[pscustomobject]@{
    ExePath = $exe
    Configuration = $Configuration
    Platform = $Platform
    FileCount = $outputFiles.Count
    TotalBytes = $totalBytes
}
