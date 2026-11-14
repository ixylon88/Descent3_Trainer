@echo off
setlocal
call "%~dp0windows10\build.cmd" %*
exit /b %errorlevel%
