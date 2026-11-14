@echo off
setlocal
dotnet test "%~dp0windows10\D3SaveEditor.sln" --configuration Release %*
exit /b %errorlevel%
