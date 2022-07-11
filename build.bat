@echo off
set app_name=sw-renderer
ctime -begin %app_name%.ctm

set CompilerFlags=-DAPP_NAME="\"%app_name%\"" -diagnostics:column -WL -nologo -fp:fast -fp:except- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7 -GS- -Gs9999999
set LinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib Winmm.lib kernel32.lib

if [%1]==[] goto MissingBuildFlag
if [%1]==[Debug] goto Debug
if [%1]==[Test] goto Test
if [%1]==[Release] goto Release

:MissingBuildFlag
echo Missing build flag (Debug, Test, Release), defaulting to debug.
goto Debug

rem todo set build params for each 
:Debug
echo Debug Build
set CompilerFlags=-DINTERNAL_BUILD=1 -DSLOW_BUILD=1 -Od %CompilerFlags%
goto Build

:Test
echo Test Build
set CompilerFlags=
goto Build

:Release
REM can set internal to 1 to enable profiling and developer debuggin tools in release mode
set CompilerFlags=-DINTERNAL_BUILD=1 -DSLOW_BUILD=0 -O2 %CompilerFlags%
echo Release Build
goto Build


:Build
if not defined DevEnvDir call vcvarsall x64
if not exist .\build mkdir .\build 
pushd .\build
echo -----------------
rem build the platform layer
cl  %CompilerFlags%  ..\code\win32_platform.cpp -Fewin32-%app_name%.exe /link /subsystem:windows %LinkerFlags%
popd
ctime -end %app_name%.ctm %LastError%d
