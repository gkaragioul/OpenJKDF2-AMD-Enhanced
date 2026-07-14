@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch-OpenJKDF2.ps1" %*
exit /b %ERRORLEVEL%
