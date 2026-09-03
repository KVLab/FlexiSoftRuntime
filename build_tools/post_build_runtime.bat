@echo off
setlocal EnableExtensions

set "ROOT=%~1"
set "OUTDIR=%~2"
set "TARGET=%~3"
set "CONFIG=%~4"

if "%ROOT%"=="" (
  echo [post_build_runtime] ERROR: missing ROOT argument
  exit /b 0
)

if "%OUTDIR%"=="" (
  echo [post_build_runtime] ERROR: missing OUTDIR argument
  exit /b 0
)

if not "%ROOT:~-1%"=="\" set "ROOT=%ROOT%\"
if not "%OUTDIR:~-1%"=="\" set "OUTDIR=%OUTDIR%\"

for %%I in ("%ROOT%..") do set "PARENT=%%~fI"
if not "%PARENT:~-1%"=="\" set "PARENT=%PARENT%\"

echo [post_build_runtime] ROOT=%ROOT%
echo [post_build_runtime] OUTDIR=%OUTDIR%
echo [post_build_runtime] TARGET=%TARGET%
echo [post_build_runtime] CONFIG=%CONFIG%
echo [post_build_runtime] PARENT=%PARENT%

if not exist "%OUTDIR%" mkdir "%OUTDIR%" >nul 2>nul

call :copy_dir "conf" "%ROOT%conf" "%OUTDIR%conf" "/XF runtime_state.json"
call :copy_dir "docs" "%ROOT%docs" "%OUTDIR%docs" ""
call :copy_dir "fonts" "%ROOT%fonts" "%OUTDIR%fonts" ""

if exist "%OUTDIR%conf\runtime_state.json" (
  del /f /q "%OUTDIR%conf\runtime_state.json" >nul 2>nul
)

set "MDREADER=%PARENT%FlexiSoftMdReader\Release\FlexiSoftMdReader.exe"

if exist "%MDREADER%" (
  copy /y "%MDREADER%" "%OUTDIR%FlexiSoftMdReader.exe" >nul
  if errorlevel 1 (
    echo [post_build_runtime] WARNING: failed to copy FlexiSoftMdReader.exe
  ) else (
    echo [post_build_runtime] copied FlexiSoftMdReader.exe
  )
) else (
  echo [post_build_runtime] WARNING: FlexiSoftMdReader.exe not found:
  echo [post_build_runtime]          %MDREADER%
  echo [post_build_runtime]          Runtime build continues without reader.
)

set "SYNC=%PARENT%sync_installer_stage.bat"

if /I "%CONFIG%"=="Release" (
  if exist "%SYNC%" (
    echo [post_build_runtime] running sync_installer_stage.bat /runtime
    call "%SYNC%" /runtime
    if errorlevel 1 (
      echo [post_build_runtime] WARNING: sync_installer_stage.bat returned error
    )
  ) else (
    echo [post_build_runtime] INFO: sync_installer_stage.bat not found, skipping:
    echo [post_build_runtime]       %SYNC%
  )
) else (
  echo [post_build_runtime] INFO: installer stage sync skipped for %CONFIG%
)

echo [post_build_runtime] done
exit /b 0

:copy_dir
set "LABEL=%~1"
set "SRC=%~2"
set "DST=%~3"
set "EXTRA=%~4"

if not exist "%SRC%\" (
  echo [post_build_runtime] INFO: %LABEL% not found, skipping: %SRC%
  exit /b 0
)

if not exist "%DST%" mkdir "%DST%" >nul 2>nul

if "%EXTRA%"=="" (
  robocopy "%SRC%" "%DST%" /E >nul
) else (
  robocopy "%SRC%" "%DST%" /E %EXTRA% >nul
)

if errorlevel 8 (
  echo [post_build_runtime] WARNING: robocopy failed for %LABEL%
) else (
  echo [post_build_runtime] copied %LABEL%
)

exit /b 0
