@echo off
setlocal enabledelayedexpansion

REM ======================================================================
REM                       Environment Setup and Initialization
REM ======================================================================
pushd %~dp0
cls

REM ======================================================================
REM                    Start Batch Process
REM ======================================================================
echo ===== Starting batch process =====
echo.

set pname=ecat_402_io.patch

REM Check if patch file exists
call :check_patch_file "%pname%" || goto end

REM Check source folder
call :check_source_folder || goto end

echo --- Starting patch process ---
patch -u -p0 --binary < "%pname%"
echo --- Patch process completed ---
echo.

REM Patch destination folders
echo --- Starting copy process ---
set DESTINATIONS[0]=..\..\..\project\CiA402\e2studio\src\ethercat\beckhoff

REM Copy patched folder to each destination
call :copy_patched_folder "!DESTINATIONS[0]!"
echo --- Copy process completed ---

REM ======================================================================
REM                       Cleanup
REM ======================================================================
echo.
REM echo --- Cleanup .\SSCconfig\Src files ---
rmdir /S /Q Src
rmdir /S /Q ..\SSCconfig\Src
REM echo --- Cleanup process completed ---

goto end

REM ======================================================================
REM                       Subroutines
REM ======================================================================

:check_patch_file
REM Check if patch file exists
echo --- Check if patch file exists ---
if not exist %~1 (
    echo [ERROR] Patch file %~1 not found
    exit /b 1
)
echo Using patch file %~1
exit /b 0

:check_source_folder
REM Check source folder
REM echo --- Check SSC source folder ---
if exist Src rmdir /S /Q Src

set SRCDIR=..\SSCconfig\Src
if not exist %SRCDIR% (
    echo [ERROR] SSC source folder %SRCDIR% not found
    exit /b 1
)

mkdir Src
copy /Y %SRCDIR% Src > NUL
REM echo SSC source folder prepared
echo.
exit /b 0

:copy_patched_folder
REM Copy patched folder to destination
set DSTDIR=%~1
if not exist "!DSTDIR!" (
    echo [ERROR] Destination folder "!DSTDIR!" does not exist
    exit /b 1
) else if exist "!DSTDIR!\Src" (
    echo Destination "!DSTDIR!\Src" already exists. Skipping copy.
    exit /b 0
) else (
    mkdir "!DSTDIR!\Src"
    copy Src "!DSTDIR!\Src" > NUL
    echo Patched Src folder copied to "!DSTDIR!\Src"
    exit /b 0
)

:end
echo.
echo ===== End batch process =====
pause
endlocal
