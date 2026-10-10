@echo off
setlocal enabledelayedexpansion

:: universal ps4 native runner for windows
cd /d "%~dp0"

:: check if native windows runner is present
if exist "%~dp0bin\windows\shadPS4.exe" (
    "%~dp0bin\windows\shadPS4.exe" %*
    exit /b %ERRORLEVEL%
)

:: check if wsl is available
where wsl >nul 2>&1
if %ERRORLEVEL% equ 0 (
    wsl -e bash ./run.sh %*
    exit /b %ERRORLEVEL%
)

:: check if python is available natively
where python >nul 2>&1
if %ERRORLEVEL% equ 0 (
    python play %*
    exit /b %ERRORLEVEL%
)

echo [error] native windows runner, wsl2, and python were not found.
echo to run ps4 games natively on windows, either:
echo 1. ensure bin\windows\shadPS4.exe is present in the release folder, or
echo 2. install wsl2 by opening powershell as administrator and running:
echo    wsl --install

echo.
pause
exit /b 1
