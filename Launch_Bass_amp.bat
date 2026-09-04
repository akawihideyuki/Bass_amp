@echo off
setlocal EnableExtensions

cd /d "%~dp0"
set "ROOT=%CD%"
set "APP_DIR=%ROOT%\app"
set "APP_EXE=%APP_DIR%\Bass_amp.exe"
set "BASSAMP_DOWNLOAD_URL=https://github.com/akawihideyuki/Bass_amp/releases/latest/download/Bass_amp-windows.zip"
set "BASSAMP_ZIP_FILE=%TEMP%\Bass_amp-windows-%RANDOM%-%RANDOM%.zip"
set "BASSAMP_APP_DIR=%APP_DIR%"

if exist "%APP_EXE%" goto launch

echo [Bass_amp] Bass_amp.exe was not found locally.
echo [Bass_amp] Downloading the latest Windows release from GitHub...
echo.

where powershell.exe >nul 2>nul
if errorlevel 1 goto powershell_missing

if not exist "%APP_DIR%" mkdir "%APP_DIR%" >nul 2>nul
if not exist "%APP_DIR%" goto app_dir_failed

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "$ProgressPreference='SilentlyContinue'; try { Invoke-WebRequest -UseBasicParsing -Uri $env:BASSAMP_DOWNLOAD_URL -OutFile $env:BASSAMP_ZIP_FILE -ErrorAction Stop; Expand-Archive -LiteralPath $env:BASSAMP_ZIP_FILE -DestinationPath $env:BASSAMP_APP_DIR -Force -ErrorAction Stop; exit 0 } catch { Write-Host ('[Bass_amp] Download/extract failed: ' + $_.Exception.Message); exit 1 }"
set "DOWNLOAD_RESULT=%ERRORLEVEL%"

if exist "%BASSAMP_ZIP_FILE%" del /q "%BASSAMP_ZIP_FILE%" >nul 2>nul

if not "%DOWNLOAD_RESULT%"=="0" goto download_failed
if not exist "%APP_EXE%" goto exe_missing

:launch
echo [Bass_amp] Starting Bass_amp...
start "" "%APP_EXE%"
exit /b 0

:powershell_missing
echo.
echo [Bass_amp] Windows PowerShell was not found.
echo Bass_amp's launcher requires the PowerShell included with Windows 11.
goto failure_pause

:app_dir_failed
echo.
echo [Bass_amp] Could not create:
echo   %APP_DIR%
echo Check folder permissions and try again.
goto failure_pause

:download_failed
echo.
echo [Bass_amp] Could not download the Windows release.
echo Check your internet connection and try again.
echo If a new release is still being built on GitHub, try again shortly.
goto failure_pause

:exe_missing
echo.
echo [Bass_amp] The release was downloaded, but Bass_amp.exe was not found.
echo Delete the "app" folder and run this launcher again.

:failure_pause
echo.
pause
exit /b 1
