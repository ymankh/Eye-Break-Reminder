param(
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$sourceDir = Join-Path $repoRoot 'c'
$buildDir = Join-Path $sourceDir 'build'

cmake -S $sourceDir -B $buildDir -DCMAKE_BUILD_TYPE=$Configuration
cmake --build $buildDir --config $Configuration

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
