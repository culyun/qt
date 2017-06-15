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

:configure

  set TOOL_CHAIN=%1
  set CONFIG_OPTIONS=-opensource
  set CONFIG_OPTIONS=%CONFIG_OPTIONS% -webkit
  set CONFIG_OPTIONS=%CONFIG_OPTIONS% -qt-freetype
  set CONFIG_OPTIONS=%CONFIG_OPTIONS% -make nmake
  set CONFIG_OPTIONS=%CONFIG_OPTIONS% -no-incredibuild-xge
  
  rem TODO: Add mkspecs etc for msvc2017
  rem win32-msvc2015 is good enough to build with for the moment

  if "%TOOL_CHAIN%" == "msvc2017" (
    echo y | "%QTSRCDIR%\configure" -platform win32-msvc2015 %CONFIG_OPTIONS%
  ) else (
    echo y | "%QTSRCDIR%\configure" -platform win32-%TOOL_CHAIN% %CONFIG_OPTIONS%
  )

goto :eof

:build

  jom /NOLOGO /D

goto :eof