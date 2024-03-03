set SRCPATH=%~dp0
set BUILD_NAME=%1
set APP_NAME=%2
set APP_EXT=%3
set APP_MODE=%4

cd %CLANGDIR%/bin

rem Use . to skip args
if [%APP_NAME%]==[] set APP_NAME=app
if [%APP_EXT%]==[] set APP_EXT=
if not [%APP_MODE%]==[] set APP_MODE=-D%APP_EXT%

set OUTPATH=%SRCPATH%../../../BUILD/%BUILD_NAME%

mkdir "%OUTPATH%"

rem AppMode is a preprocessor switch to use in appcfg
rem NOTE - The Framework should find out to which project it belongs
rem TODO - Check if build path is valid and find it deeper if not

clang.exe -o "%OUTPATH%/%APP_NAME%%APP_EXT%" --config "%SRCPATH%CFG/%BUILD_NAME%.cfg" %APP_MODE% "%SRCPATH%../AppMain.cpp"

pause