@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "ROOT=%%~fI"
cd /d "%ROOT%"

if not exist "CMakeLists.txt" (
    echo [Bass_amp] CMakeLists.txt was not found in:
    echo   %ROOT%
    exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo [Bass_amp] CMake was not found in PATH.
    echo Install CMake 3.22 or newer, then try again.
    exit /b 1
)

where git >nul 2>nul
if errorlevel 1 (
    echo [Bass_amp] Git was not found in PATH.
    echo Git is required because CMake fetches JUCE automatically.
    exit /b 1
)

echo [Bass_amp] Configuring...
cmake -S . -B build
if errorlevel 1 (
    echo [Bass_amp] CMake configure failed.
    echo Make sure Visual Studio has the Desktop development with C++ workload installed.
    exit /b 1
)

echo.
echo [Bass_amp] Building Release...
cmake --build build --config Release --parallel 2
if errorlevel 1 (
    echo [Bass_amp] Release build failed.
    exit /b 1
)

echo.
echo [Bass_amp] Build completed successfully.
exit /b 0
