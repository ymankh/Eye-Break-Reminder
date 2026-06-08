param(
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
function ConvertTo-CMakePath {
    param([string]$Path)

    $Path -replace '\\', '/'
}


$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$sourceDir = Join-Path $repoRoot 'c'
$buildDir = Join-Path $sourceDir 'build'

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
$cmakePath = if ($cmake) { $cmake.Source } else { $null }

if (-not $cmakePath) {
    $cmakeCandidates = @()
    if ($env:ProgramFiles) {
        $cmakeCandidates += Join-Path $env:ProgramFiles 'CMake/bin/cmake.exe'
    }
    if (${env:ProgramFiles(x86)}) {
        $cmakeCandidates += Join-Path ${env:ProgramFiles(x86)} 'CMake/bin/cmake.exe'
    }

    $cmakePath = $cmakeCandidates |
        Where-Object { Test-Path $_ } |
        Select-Object -First 1
}

if (-not $cmakePath) {
    throw @'
CMake is required to build the native C app, but `cmake` was not found on PATH.
Install CMake, open a new terminal, then rerun this script:
    winget install --id Kitware.CMake --exact --source winget
'@
}

$cmakeConfigureArgs = @(
    '-S', $sourceDir,
    '-B', $buildDir,
    "-DCMAKE_BUILD_TYPE=$Configuration"
)

$cl = Get-Command cl -ErrorAction SilentlyContinue
$nmake = Get-Command nmake -ErrorAction SilentlyContinue
$gcc = Get-Command gcc -ErrorAction SilentlyContinue
$windres = Get-Command windres -ErrorAction SilentlyContinue
$make = Get-Command mingw32-make -ErrorAction SilentlyContinue
if (-not $make) {
    $make = Get-Command make -ErrorAction SilentlyContinue
}

if (-not ($cl -and $nmake) -and $gcc -and $windres -and $make) {
    $cmakeConfigureArgs += @(
        '-G', 'MinGW Makefiles',
        "-DCMAKE_MAKE_PROGRAM=$(ConvertTo-CMakePath $make.Source)",
        "-DCMAKE_C_COMPILER=$(ConvertTo-CMakePath $gcc.Source)",
        "-DCMAKE_RC_COMPILER=$(ConvertTo-CMakePath $windres.Source)"
    )
}

if (Test-Path $buildDir) {
    Remove-Item -Path $buildDir -Recurse -Force
}

& $cmakePath @cmakeConfigureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

& $cmakePath --build $buildDir --config $Configuration
if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed with exit code $LASTEXITCODE."
}

$exe = Get-ChildItem -Path $buildDir -Recurse -Filter break_reminder_c.exe |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if (-not $exe) {
    throw 'No C executable was produced by the CMake build.'
}

if ($env:GITHUB_OUTPUT) {
    "exe_path=$($exe.FullName)" >> $env:GITHUB_OUTPUT
    "exe_name=$($exe.Name)" >> $env:GITHUB_OUTPUT
}

[pscustomobject]@{
    ExePath = $exe.FullName
    ExeName = $exe.Name
}
