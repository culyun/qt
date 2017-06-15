@echo off
setlocal ENABLEEXTENSIONS
setlocal ENABLEDELAYEDEXPANSION

set SCRIPT_NAME=%~nx0
set SCRIPT_PATH=%~dp0
set SCRIPT_PATH=%SCRIPT_PATH:~0,-1%
for %%* in (%SCRIPT_PATH%) do set SCRIPT_FOLDER=%%~nx*
set SCRIPT_ARGS=%*

rem You can use tt as a temporary file for stashing stuff
set tt=%Time: =0%
set tt=%tt::=%
set tt=%tt:.=%
set tt=%tt:,=%
set tt=%TEMP%\%tt%.%SCRIPT_NAME%

set PWD=%CD%

:start_here

call :main %*
cd "%PWD%"
echo.
exit /b %errorlevel%

:subroutines

:setup

  echo.
  echo ##############
  echo #
  echo # Setting up Qt %BUILD_ALIAS% build environment
  echo.

  set QTSRCDIR=%SCRIPT_PATH%

  pushd "%QTSRCDIR%\.."
  set QT_INSTALL_ROOT=%CD%
  popd

  if "%TOOL_CHAIN%" == "msvc2008" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
    goto :eof
  )

  if "%TOOL_CHAIN%" == "msvc2010" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
    goto :eof
  )

  rem msvc2012 through msvc2015 correspond to Visual Studio versions 11 through 14,
  rem which is why they're install paths appear be one version behind, eg. msvc2012 is installed in the 11.0 folder

  if "%TOOL_CHAIN%" == "msvc2012" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
    goto :eof
  )

  if "%TOOL_CHAIN%" == "msvc2013" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
    goto :eof
  )

  if "%TOOL_CHAIN%" == "msvc2015" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
    goto :eof
  )

  rem Microsoft's tool layout changed in msvc2017 onwards.  See the following:
  rem   https://blogs.msdn.microsoft.com/vcblog/2016/10/07/compiler-tools-layout-in-visual-studio-15/

  rem VSEDITION can be Community | Professional | Enterprise
  set VSEDITION=Community

  if "%TOOL_CHAIN%" == "msvc2017" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\%VSEDITION%\VC\Auxiliary\Build\vcvarsall.bat" amd64_x86
    goto :eof
  )

goto :eof

:configure

  echo.
  echo ##############
  echo #
  echo # Configuring Qt %BUILD_ALIAS%
  echo.

  if not exist "%QT_INSTALL_ROOT%\%BUILD_ALIAS%" mkdir "%QT_INSTALL_ROOT%\%BUILD_ALIAS%"
  copy "%SCRIPT_PATH%\jom.exe" "%QT_INSTALL_ROOT%\%BUILD_ALIAS%\jom.exe"
  cd "%QT_INSTALL_ROOT%\%BUILD_ALIAS%"

  call "%SCRIPT_PATH%\build-targets\%BUILD_TARGET%.cmd" :configure %TOOL_CHAIN%

  if %errorlevel% neq 0 (
    echo.
    echo ##############
    echo #
    echo # ^^!^^! Fatal error %errorlevel% while configuring Qt %BUILD_ALIAS%^^!^^!
    echo.
    popd
    goto :eof
  )

  echo.
  echo ##############
  echo #
  echo # Qt %BUILD_ALIAS% configuration complete
  echo.

goto :eof

:build

  echo.
  echo ##############
  echo #
  echo # Building Qt %BUILD_ALIAS%
  echo.

  if not exist "%QT_INSTALL_ROOT%\%BUILD_ALIAS%" (
    echo.
    echo ##############
    echo #
    echo # Build failed -- Call "%SCRIPTNAME% configure" first
    echo.
    set errorlevel=996
    goto :eof
  )

  pushd "%QT_INSTALL_ROOT%\%BUILD_ALIAS%"
  set PATH=%cd%\bin;%PATH%

  call "%SCRIPT_PATH%\build-targets\%BUILD_TARGET%.cmd" :build

  if %errorlevel% neq 0 (
    echo.
    echo ##############
    echo #
    echo # ^^!^^! Fatal error %errorlevel% while building Qt %BUILD_ALIAS%^^!^^!
    echo.
    popd
    goto :eof
  )

  popd

  echo.
  echo ##############
  echo #
  echo # Qt %BUILD_ALIAS% build complete
  echo.

goto :eof

:distill

  echo.
  echo ##############
  echo #
  echo # Distilling Qt %BUILD_ALIAS%
  echo.

  if not exist "%QT_INSTALL_ROOT%\%BUILD_ALIAS%" (
    echo.
    echo ##############
    echo #
    echo # Distillation failed -- "%QT_INSTALL_ROOT%\%BUILD_ALIAS%" doesn't exist
    echo.
    set errorlevel=995
    goto :eof
  )

  pushd "%QT_INSTALL_ROOT%\%BUILD_ALIAS%"
  set PATH=%cd%\bin;%PATH%

  jom /NOLOGO clean

  if %errorlevel% neq 0 (
    echo.
    echo ##############
    echo #
    echo # ^^!^^! Fatal error %errorlevel% while distilling Qt %BUILD_ALIAS% ^^!^^!
    echo.
    popd
    goto :eof
  )

  rem It look's like the only thing that's needed in .\src is some headers in .\src\corelib\global
  rem So let's grab those and delete the 100's of MB of intermediates

  xcopy /i /e .\src\corelib\global .\src-tt\corelib\global
  rmdir /q /s .\src
  move .\src-tt .\src

  popd

  echo.
  echo ##############
  echo #
  echo # Qt %BUILD_ALIAS% distillation complete
  echo.

goto :eof

:show_help

  echo.
  echo ############################################################
  echo.##
  echo ## %SCRIPT_NAME% is used to build Qt for 32-bit Windows based targets
  echo.##
  echo ## Usage: %SCRIPT_NAME% target [build_action] [tool_chain]
  echo.##
  echo ##  where [] indicates an optional parameter, and:
  echo.##
  echo ##   - target is one of {%BUILD_TARGETS%}.
  echo ##   - [build_action] is one of {configure, build, distill, all}.
  echo ##     Default is all, ie. perform configure, build, and distill in sequence
  echo ##   - tool_chain is one of {msvc2008, msvc2010, msvc2012, msvc2013, msvc2015, msvc2017}.
  echo ##     Default is msvc2008
  echo.##
  echo.## --------------------------------------------------------
  echo.##
  echo ## You called %SCRIPT_NAME% from "%PWD%" with
  echo.##
  echo ##   %SCRIPT_NAME% %SCRIPT_ARGS%
  echo.##
if "%~1" neq "" (
  echo.## which caused error, "%~1"
  echo.##
)
  echo ############################################################
  echo.

goto :eof

:main

  git branch > tt
  set /p GIT_BRANCH= <tt
  set GIT_BRANCH=%GIT_BRANCH:* =%

  git rev-parse HEAD > tt
  set /p GIT_ID= <tt
  set GIT_ID=%GIT_ID:~0,6%

  set QT_VERSION=%GIT_BRANCH%-%GIT_ID%
  del tt

  for /r "%SCRIPT_PATH%\build-targets" %%f in (*.cmd) do (
    if not defined BUILD_TARGETS (
      call set "BUILD_TARGETS=%%~nf"
    ) else (
      call set "BUILD_TARGETS=%%BUILD_TARGETS%%, %%~nf"
    )
  )

  set BUILD_TARGET=%~1

  if not defined BUILD_TARGET (
    call :show_help "target not specified"
    set errorlevel=999
    goto :finish
  )

  for /r "%SCRIPT_PATH%\build-targets" %%f in (*.cmd) do (
    if %BUILD_TARGET%==%%~nf set VALID_BUILD_TARGET=true
  )

  for /f %%a in ('echo -h ^&echo --help') do (
    if %~1==%%a (
      call :show_help
      goto :eof
    )
  )

  if not defined VALID_BUILD_TARGET (
    call :show_help "unknown target"
    set errorlevel=998
    goto :finish
  )

  set BUILD_TASK=%~2
  if "%2" == "" set BUILD_TASK=all

  for /f %%a in ('echo configure ^&echo build ^&echo distill ^&echo all') do (
    if %BUILD_TASK%==%%a set VALID_BUILD_TASK=true
  )

  if not defined VALID_BUILD_TASK (
    call :show_help "unknown build task"
    set errorlevel=997
    goto :finish
  )

  set TOOL_CHAIN=%~3
  if "%3" == "" set TOOL_CHAIN=msvc2008

  for /f %%a in ('echo msvc2008 ^&echo msvc2010 ^&echo msvc2012 ^&echo msvc2013 ^&echo msvc2015 ^&echo msvc2017') do (
    if %TOOL_CHAIN%==%%a set KNOWN_TOOL_CHAIN=true
  )

  if not defined KNOWN_TOOL_CHAIN (
    call :show_help "unknown tool chain"
    set errorlevel=994
    goto :finish
  )

  set BUILD_ALIAS=%QT_VERSION%-%BUILD_TARGET%-%TOOL_CHAIN%

  :start

  echo.
  echo ##############
  echo #
  echo # Starting Qt %BUILD_ALIAS% %BUILD_TASK% at %Date% %Time%
  echo.

  call :setup

  if "%BUILD_TASK%" neq "all" (
    call :%BUILD_TASK%
    goto :finish
  )

  call :configure
  if %errorlevel% neq 0 goto :finish
  call :build
  if %errorlevel% neq 0 goto :finish
  call :distill
  if %errorlevel% neq 0 goto :finish

  :finish

  if %errorlevel% neq 0 (
    echo. Failed with error %errorlevel% :-^(
  ) else (
    echo. All done :-^) at %Date% %Time%
    call :warmFuzzy
  )

goto :eof

:warmFuzzy
  echo.   __    __    __      ____  _____  _  _  ____    ##     ___-___     ##
  echo.  /__\  (  )  (  )    (  _ \(  _  )( \( )( ___)  o##=ooO-  _~~  -Ooo=##o
  echo. /(__)\  )(__  )(__    )(_) ))(_)(  )  (  )__)    ## \\    - +    // ##
  echo.(__)(__)(____)(____)  (____/(_____)(_)\_)(____)        \\__\0/__//
goto :eof
