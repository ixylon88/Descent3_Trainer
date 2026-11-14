@echo off
setlocal
cd /d "%~dp0"
dotnet build D3SaveEditor.sln --configuration Release
exit /b %errorlevel%
