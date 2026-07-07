@echo off
setlocal enabledelayedexpansion
set "DIST_VERSION=8.2.1"
set "DIST_NAME=gradle-%DIST_VERSION%-bin.zip"
set "DIST_URL=https://services.gradle.org/distributions/%DIST_NAME%"
set "ROOT=%~dp0"
set "DEST=%ROOT%\.gradle\wrapper\dists"

rem If system Gradle is available, use it to run the build instead of downloading distribution.
where gradle >nul 2>&1
if %errorlevel%==0 (
  echo Found system Gradle, delegating to it.
  gradle %*
  exit /b %ERRORLEVEL%
)

if not exist "%DEST%\gradle-%DIST_VERSION%" (
  echo Downloading %DIST_URL% ...
  rem Try curl first (Windows 10+), then fallback to PowerShell WebClient with TLS1.2, then certutil.
  where curl >nul 2>&1
  if %errorlevel%==0 (
    curl -L -o "%TEMP%\%DIST_NAME%" "%DIST_URL%"
    if exist "%TEMP%\%DIST_NAME%" (
      powershell -NoProfile -Command "Expand-Archive -LiteralPath \"$env:TEMP\\%DIST_NAME%\" -DestinationPath \"%DEST%\"; Remove-Item \"$env:TEMP\\%DIST_NAME%\" -Force"
    )
  ) else (
    powershell -NoProfile -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; $zip = Join-Path $env:TEMP '%DIST_NAME%'; $wc = New-Object System.Net.WebClient; $wc.DownloadFile('%DIST_URL%', $zip); Expand-Archive -LiteralPath $zip -DestinationPath '%DEST%'; Remove-Item $zip -Force" || (
      echo PowerShell download failed, trying certutil...
      certutil -urlcache -split -f "%DIST_URL%" "%TEMP%\%DIST_NAME%" >nul 2>&1 && powershell -NoProfile -Command "Expand-Archive -LiteralPath \"$env:TEMP\\%DIST_NAME%\" -DestinationPath \"%DEST%\"; Remove-Item \"$env:TEMP\\%DIST_NAME%\" -Force"
    )
  )
)

set "GRADLE_HOME=%DEST%\gradle-%DIST_VERSION%"
if not exist "%GRADLE_HOME%\bin\gradle.bat" (
  echo Failed to prepare Gradle distribution in %DEST%\gradle-%DIST_VERSION%
  exit /b 1
)

"%GRADLE_HOME%\bin\gradle.bat" %*
