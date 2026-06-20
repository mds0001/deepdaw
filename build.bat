@echo off
setlocal enabledelayedexpansion

echo ========================================
echo DeepDAW Phase 0 - Automated Build
echo ========================================

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: CMake not found in PATH.
    echo Please install CMake 3.25+ from https://cmake.org/download/
    pause
    exit /b 1
)

where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio compiler (cl.exe) not found.
    echo Please run from a "Developer Command Prompt for VS 2022"
    echo or ensure Visual Studio 2022 with C++ workload is installed.
    pause
    exit /b 1
)

echo Configuring project...
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64

if %errorlevel% neq 0 (
    echo CMake configuration failed.
    pause
    exit /b 1
)

echo Building Release configuration...
cmake --build build --config Release --parallel

if %errorlevel% neq 0 (
    echo Build failed.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful!
echo Executable: build\Release\DeepDAW.exe
echo ========================================
echo.
echo To run: .\build\Release\DeepDAW.exe
echo.
pause