@echo off
setlocal enabledelayedexpansion

:: universal ps4 native runner for windows
cd /d "%~dp0"

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

echo [error] neither wsl2 nor python was found on this system.
echo to run ps4 games natively on windows, install wsl2 by opening
echo powershell as administrator and running:
echo   wsl --install
echo.
pause
exit /b 1
