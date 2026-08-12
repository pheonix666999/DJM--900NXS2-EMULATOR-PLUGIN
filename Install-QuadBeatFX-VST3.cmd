@echo off
setlocal
title QuadBeat FX VST3 Installer
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-QuadBeatFX-VST3.ps1"
set "INSTALL_RESULT=%ERRORLEVEL%"
echo.
if not "%INSTALL_RESULT%"=="0" echo Installation failed. Review the message above.
pause
exit /b %INSTALL_RESULT%
