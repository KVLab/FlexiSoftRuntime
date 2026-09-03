@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem FlexiSoft Runtime post-build helper.
rem
rem This helper is convenience-only. It must NEVER make a successful
rem compiler/linker build fail. Missing optional files/tools are reported
rem as INFO/WARNING and the script always returns exit code 0.
rem
rem Arguments from Visual Studio:
rem   1 = solution/root directory
rem   2 = output directory
rem   3 = target path
rem   4 = configuration

set "ROOT=%~1"
set "OUTDIR=%~2"
set "TARGET=%~3"
set "CONFIG=%~4"

if "%ROOT%"=="" (
  echo [post_build_runtime] WARNING: missing ROOT argument - skipping post-build.
  exit /b 0
)

if "%OUTDIR%"=="" (
  echo [post_build_runtime] WARNING: missing OUTDIR argument - skipping post-build.
  exit /b 0
)

for %%I in ("%ROOT%") do set "ROOT=%%~fI"
for %%I in ("%OUTDIR%") do set "OUTDIR=%%~fI"
if not "%TARGET%"=="" for %%I in ("%TARGET%") do set "TARGET=%%~fI"
for %%I in ("%ROOT%\..") do set "PARENT=%%~fI"

echo [post_build_runtime] ROOT=%ROOT%
echo [post_build_runtime] OUTDIR=%OUTDIR%
echo [post_build_runtime] TARGET=%TARGET%
echo [post_build_runtime] CONFIG=%CONFIG%

if not exist "%OUTDIR%" (
  mkdir "%OUTDIR%" >nul 2>nul
  if errorlevel 1 (
    echo [post_build_runtime] WARNING: cannot create output directory:
    echo [post_build_runtime]          %OUTDIR%
    exit /b 0
  )
)

rem ---------------------------------------------------------------------------
rem Runtime payload.
rem ---------------------------------------------------------------------------

call :CopyDir "conf" "%ROOT%\conf" "%OUTDIR%\conf" "/XF runtime_state.json"
call :CopyDir "docs" "%ROOT%\docs" "%OUTDIR%\docs" ""
call :CopyDir "fonts" "%ROOT%\fonts" "%OUTDIR%\fonts" ""

if exist "%OUTDIR%\conf\runtime_state.json" (
  del /f /q "%OUTDIR%\conf\runtime_state.json" >nul 2>nul
)

if exist "%ROOT%\LICENSE" (
  copy /y "%ROOT%\LICENSE" "%OUTDIR%\LICENSE" >nul 2>nul
  if errorlevel 1 (
    echo [post_build_runtime] WARNING: failed to copy LICENSE.
  ) else (
    echo [post_build_runtime] copied LICENSE
  )
) else (
  echo [post_build_runtime] INFO: root LICENSE not found, skipping.
)

rem ---------------------------------------------------------------------------
rem Optional sibling FlexiSoftMdReader.
rem A clean Runtime clone must build even when the Reader repo is absent.
rem ---------------------------------------------------------------------------

set "READER_EXE=%PARENT%\FlexiSoftMdReader\Release\FlexiSoftMdReader.exe"

if exist "%READER_EXE%" (
  copy /y "%READER_EXE%" "%OUTDIR%\FlexiSoftMdReader.exe" >nul 2>nul
  if errorlevel 1 (
    echo [post_build_runtime] WARNING: failed to copy FlexiSoftMdReader.exe.
  ) else (
    echo [post_build_runtime] copied FlexiSoftMdReader.exe
  )
) else (
  echo [post_build_runtime] INFO: optional Reader not found, skipping:
  echo [post_build_runtime]       %READER_EXE%
)

rem ---------------------------------------------------------------------------
rem Optional local-only installer-stage sync.
rem Same principle as FlexiSoftMdReader: this helper lives above the public
rem repository and must never be required for a successful clean-clone build.
rem ---------------------------------------------------------------------------

if /I "%CONFIG%"=="Release" (
  set "LOCAL_SYNC=%PARENT%\sync_installer_stage.bat"

  if exist "!LOCAL_SYNC!" (
    echo [post_build_runtime] running optional local sync:
    echo [post_build_runtime]   !LOCAL_SYNC! /runtime
    call "!LOCAL_SYNC!" /runtime
    set "SYNC_RC=!ERRORLEVEL!"

    if not "!SYNC_RC!"=="0" (
      echo [post_build_runtime] WARNING: optional sync returned errorlevel !SYNC_RC!.
      echo [post_build_runtime]          Build continues.
    )
  ) else (
    echo [post_build_runtime] INFO: optional local sync not found, skipping:
    echo [post_build_runtime]       !LOCAL_SYNC!
  )
) else (
  echo [post_build_runtime] INFO: installer-stage sync skipped for %CONFIG%.
)

echo [post_build_runtime] done
exit /b 0

:CopyDir
set "LABEL=%~1"
set "SRC=%~2"
set "DST=%~3"
set "EXTRA=%~4"

if not exist "%SRC%\" (
  echo [post_build_runtime] INFO: %LABEL% not found, skipping:
  echo [post_build_runtime]       %SRC%
  exit /b 0
)

if not exist "%DST%" mkdir "%DST%" >nul 2>nul

if "%EXTRA%"=="" (
  robocopy "%SRC%" "%DST%" /MIR /R:1 /W:1 /NFL /NDL /NJH /NJS /NP >nul
) else (
  robocopy "%SRC%" "%DST%" /MIR %EXTRA% /R:1 /W:1 /NFL /NDL /NJH /NJS /NP >nul
)

set "COPY_RC=%ERRORLEVEL%"
if %COPY_RC% GEQ 8 (
  echo [post_build_runtime] WARNING: robocopy failed for %LABEL%, exit code %COPY_RC%.
) else (
  echo [post_build_runtime] synced %LABEL%
)

exit /b 0
