@echo off
setlocal
cd /d "%~dp0"
set "D3_CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%D3_CSC%" set "D3_CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe"
if not exist "%D3_CSC%" (
  echo Der .NET-Framework-C#-Compiler wurde nicht gefunden.
  exit /b 1
)
"%D3_CSC%" /nologo /target:winexe /platform:anycpu /optimize+ /win32manifest:src\app.manifest /out:D3SaveEditor.exe /reference:System.dll /reference:System.Core.dll /reference:System.Windows.Forms.dll /reference:System.Drawing.dll src\AssemblyInfo.cs src\SaveGame.cs src\ShipLimits.cs src\Program.cs
if errorlevel 1 exit /b 1
echo D3SaveEditor.exe wurde erstellt.
endlocal
