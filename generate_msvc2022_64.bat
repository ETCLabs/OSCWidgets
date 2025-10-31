@echo off
cd /D "%~dp0"
rmdir /S /Q build
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
pause