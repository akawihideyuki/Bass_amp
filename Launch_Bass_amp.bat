@echo off
setlocal EnableExtensions

cd /d "%~dp0"
set "ROOT=%CD%"

call :find_app
if defined APP_EXE goto launch

echo [Bass_amp] Release executable was not found.
echo [Bass_amp] Building the Release version now...
echo.
call "%ROOT%\scripts\build_windows.bat"
if errorlevel 1 goto build_failed

call :find_app
if not defined APP_EXE goto exe_missing

:launch
echo [Bass_amp] Starting:
echo   %APP_EXE%
start "" "%APP_EXE%"
exit /b 0

:find_app
set "APP_EXE="
set "KNOWN_EXE=%ROOT%\build\BassAmp_artefacts\Release\Bass_amp.exe"

if exist "%KNOWN_EXE%" (
    set "APP_EXE=%KNOWN_EXE%"
    exit /b 0
)

if exist "%ROOT%\build" (
    for /r "%ROOT%\build" %%F in (Bass_amp.exe) do (
        if not defined APP_EXE set "APP_EXE=%%~fF"
    )
)
exit /b 0

:build_failed
echo.
echo [Bass_amp] Build failed.
echo Make sure Visual Studio C++ tools, CMake, Git, and an internet connection are available.
echo See README.md for the build requirements.
echo.
pause
exit /b 1

:exe_missing
echo.
echo [Bass_amp] The build finished, but Bass_amp.exe could not be found.
echo Check the build output above for details.
echo.
pause
exit /b 1
