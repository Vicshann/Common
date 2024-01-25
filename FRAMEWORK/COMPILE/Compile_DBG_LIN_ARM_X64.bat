set SRCPATH=%~dp0
cd %CLANGDIR%/bin

clang.exe -o "%SRCPATH%../../../BUILD/app.so" --config "%SRCPATH%CFG/DBG_LIN_ARM_X64.cfg" "%SRCPATH%../AppMain.cpp"

pause