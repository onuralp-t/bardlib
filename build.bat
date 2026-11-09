@echo off
if /i "%~1"=="clean" goto clean

set NOCRT=-Wl,/SUBSYSTEM:console,/ENTRY:entry_point,/OPT:REF -nostdlib -ffreestanding -nostartfiles -fno-builtin
set CFLAGS=-std=c23 -Wall -Wextra -Wpedantic -Os -lkernel32 -luser32 -lgdi32

@echo on
clang -o main.exe src/main.c %CFLAGS% %NOCRT% || exit /b 1
@echo off

rem start /wait "" main.exe
main.exe
exit /b %ERRORLEVEL%

rem clang -c lib.c -o lib.o -fPIC -O2
rem clang -c lib.h -o lib.o -fPIC -O2
rem llvm-ar rcs lib.a lib.o

:clean
del /f /q *.exe *.pdb *.rdi 2>nul
exit /b 0
