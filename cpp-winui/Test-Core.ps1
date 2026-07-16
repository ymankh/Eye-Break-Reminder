$ErrorActionPreference = 'Stop'
$gcc = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gcc) { throw 'GCC is required for the lightweight C core test.' }

$testOutput = Join-Path $PSScriptRoot 'artifacts\tests\reminder_core_tests.exe'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $testOutput) | Out-Null
& $gcc.Source -std=c11 -Wall -Wextra -Werror (Join-Path $PSScriptRoot 'core\reminder_core.c') (Join-Path $PSScriptRoot 'tests\reminder_core_tests.c') -o $testOutput
if ($LASTEXITCODE -ne 0) { throw "C core compilation failed with exit code $LASTEXITCODE." }
& $testOutput
if ($LASTEXITCODE -ne 0) { throw "C core tests failed with exit code $LASTEXITCODE." }
Write-Host 'Reminder core tests passed.'
