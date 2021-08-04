@if "%ECHOON%" == "" (@echo off) else (@echo %ECHOON%)&:: set ECHOON=on if you want to debug this script
@::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
@::  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  ::
@::  Read the zproject/README.md for information about making permanent changes. ::
@::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
SET solution=zyre.sln
SET version=11
SET log=build.log
SET tools=Microsoft Visual Studio %version%.0\VC\vcvarsall.bat
SET environment="%programfiles(x86)%\%tools%"
IF NOT EXIST %environment% SET environment="%programfiles%\%tools%"
IF NOT EXIST %environment% GOTO no_tools

ECHO Building %solution%...

CALL %environment% x86 > nul
@if "%ECHOON%" == "" (@echo off) else (@echo %ECHOON%)&:: set ECHOON=on if you want to debug this script
ECHO Platform=x86

ECHO Configuration=DynDebug
msbuild /m /v:n /p:Configuration=DynDebug /p:Platform=Win32 %solution% > %log%
IF errorlevel 1 GOTO error
ECHO Configuration=DynRelease
msbuild /m /v:n /p:Configuration=DynRelease /p:Platform=Win32 %solution% >> %log%
IF errorlevel 1 GOTO error

CALL %environment% x86_amd64 > nul
@if "%ECHOON%" == "" (@echo off) else (@echo %ECHOON%)&:: set ECHOON=on if you want to debug this script
ECHO Platform=x64

ECHO Configuration=DynDebug
msbuild /m /v:n /p:Configuration=DynDebug /p:Platform=x64 %solution% > %log%
IF errorlevel 1 GOTO error
ECHO Configuration=DynRelease
msbuild /m /v:n /p:Configuration=DynRelease /p:Platform=x64 %solution% >> %log%
IF errorlevel 1 GOTO error

ECHO Complete: %solution%
GOTO end

:error
ECHO *** ERROR, build terminated early: see %log%
GOTO end

:no_tools
ECHO *** ERROR, build tools not found: %tools%

:end
