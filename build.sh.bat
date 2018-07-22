#/bin/bash 2>nul || goto :windows

# bash
set -e
mkdir -p bin
gcc src/*.c examples/$1/*.c -o bin/$1 -Iinclude $2
exit

:windows
@echo off

sh build.sh.bat %1 -lws2_32
exit /b
