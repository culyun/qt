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

call :%*
exit /b %errorlevel%

:subroutines

rem Use this as a basis for making a custom qt build-target for your device
rem It needs a bit of work, but should be easy enough to massage into shape

:configure

  set TOOL_CHAIN=%1

  if "%TOOL_CHAIN%" neq "msvc2008" (
    echo.
    echo ##############
    echo #
    echo # Configure failed -- Invalid tool chain, Windows CE (WinCE) only supports msvc2008
    echo.
    set errorlevel=899
    goto :eof
  )

  rem Set QT_XPLATFORM_BASE to name the base name of your device's mkspec folder
  set QT_XPLATFORM_BASE=wince600Generic-armv4i

  set CONFIG_OPTIONS=-opensource -release -no-phonon-backend -no-phonon -no-multimedia -no-audio-backend -no-script -no-dbus -no-openssl -no-openvg -no-declarative -nomake demos -nomake examples -nomake tools
  rem set CONFIG_OPTIONS=%CONFIG_OPTIONS% -no-webkit
  set CONFIG_OPTIONS=%CONFIG_OPTIONS% -webkit
  set CONFIG_OPTIONS=%CONFIG_OPTIONS% -qt-freetype
  set CONFIG_OPTIONS=%CONFIG_OPTIONS% -D QT_NO_PRINTER -D QT_NO_PRINTDIALOG

  echo y | "%QTSRCDIR%\configure" -platform win32-%TOOL_CHAIN% -xplatform %QT_XPLATFORM_BASE%-%TOOL_CHAIN% %CONFIG_OPTIONS%

goto :eof

:build

  rem The libraries that MS ships with Windows CE are lame
  rem Many of the includes redefine macros and other such things
  rem The following PATH order was found by trial and error
  rem Your mileage may vary...

  rem Change TARGET_WINCE_VER to match the version of Windows CE that your device targets
  set TARGET_WINCE_VER=wce600

  rem Change OEM_SDK_NAME to match the name of the SDK that ships for your device
  set OEM_SDK_NAME=GenericWinceDevice

  set PATH=%ProgramFiles%\Microsoft SDKs\Windows\v6.0A\Bin;%PATH%
  set PATH=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\VCPackages;%PATH%
  set PATH=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\Common7\IDE;%PATH%
  set PATH=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\Common7\Tools;%PATH%
  set PATH=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\bin;%PATH%
  set PATH=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\ce\BIN\x86_arm;%PATH%

  set INCLUDE=%ProgramFiles(x86)%\Windows CE Tools\%TARGET_WINCE_VER%\%OEM_SDK_NAME%\Include\Armv4i;%INCLUDE%
  set INCLUDE=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\ce\include;%INCLUDE%
  set INCLUDE=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\ce\atlmfc\include;%INCLUDE%

  set LIB=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\ce\atlmfc\lib\armv4i;%LIB%
  set LIB=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\ce\lib\armv4i;%LIB%
  set LIB=%ProgramFiles(x86)%\Windows CE Tools\%TARGET_WINCE_VER%\%OEM_SDK_NAME%\Lib\Armv4i;%LIB%

  set LIBPATH=%LIB%

  jom /NOLOGO /D

goto :eof