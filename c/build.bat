@echo off
setlocal

if not exist app.ico powershell -NoProfile -ExecutionPolicy Bypass -File create-icon.ps1
if errorlevel 1 exit /b 1

windres break-reminder.rc -O coff -o break-reminder.res
if errorlevel 1 exit /b 1

gcc main.c break-reminder.res -mwindows -O2 -s -Wall -Wextra -Wpedantic -o break-reminder.exe -lcomctl32 -lshell32 -lgdi32 -ladvapi32
if errorlevel 1 exit /b 1

echo Built break-reminder.exe
