@echo off
setlocal
cd /d "%~dp0"
if not defined D3_CC set "D3_CC=i686-w64-mingw32-gcc"
"%D3_CC%" -std=c99 -Os -Wall -Wextra -Wno-misleading-indentation -ffreestanding -fno-builtin -fno-stack-protector -fno-ident -fno-asynchronous-unwind-tables -fno-unwind-tables -march=i386 -mfpmath=387 -mno-sse -DWINVER=0x0400 -D_WIN32_WINNT=0x0400 -D_WIN32_IE=0x0300 -nostdlib -mwindows -s -Wl,--entry,_WinMainCRTStartup@0,--major-os-version,4,--minor-os-version,0 -Wl,--major-subsystem-version,4,--minor-subsystem-version,0 -Wl,--disable-dynamicbase,--disable-nxcompat,--stack,1048576 src\win98.c src\d3core.c src\d3files.c src\sha256.c -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lcomctl32 -lgcc -o D3EDIT98.EXE
if errorlevel 1 exit /b 1
echo D3EDIT98.EXE wurde erstellt.
endlocal
