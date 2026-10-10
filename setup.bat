@echo off
setlocal

:: universal ps4 native windows setup
cd /d "%~dp0"
echo ==========================================================
echo universal ps4 native - windows setup
echo ==========================================================

where wsl >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [1/2] wsl2 found on system
    echo [2/2] initializing linux runtime environment...
    wsl -e bash ./setup.sh
    echo.
    echo ==========================================================
    echo setup complete. you can now launch games with:
    echo   play.exe       (graphical launcher)
    echo   run.bat        (command-line launcher)
    echo ==========================================================
    exit /b 0
)

echo [warning] wsl2 is not installed.
echo to run native ps4 titles with direct vulkan acceleration on windows,
echo wsl2 with wslg is required.
echo.
echo to install wsl2, open powershell as administrator and run:
echo   wsl --install
echo.
pause
exit /b 1
