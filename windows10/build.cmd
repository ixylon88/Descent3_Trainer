@echo off
setlocal
cd /d "%~dp0"
dotnet build D3SaveEditor.Gui\D3SaveEditor.Gui.csproj --configuration Release %*
exit /b %errorlevel%
