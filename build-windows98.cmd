@echo off
setlocal
call "%~dp0windows98\build.cmd"
exit /b %errorlevel%
