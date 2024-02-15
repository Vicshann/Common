set SRCPATH=%~dp0
set BUILD_NAME=%1
set APP_NAME=%2
set APP_EXT=%3

cd %CLANGDIR%/bin

if [%APP_NAME%]==[] set APP_NAME=app
if [%APP_EXT%]==[] set APP_EXT=.exe

rem NOTE - The Framework should find out to which project it belongs
rem TODO - Check if build path is valid and find it deeper if not

clang.exe -o "%SRCPATH%../../../BUILD/%BUILD_NAME%/%APP_NAME%%APP_EXT%" --config "%SRCPATH%CFG/%BUILD_NAME%.cfg" "%SRCPATH%../AppMain.cpp"

pause