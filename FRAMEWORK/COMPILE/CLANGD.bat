set SRCPATH=%~dp0

cd "%SRCPATH%"

C:\_TOOLS_\_DEVTOOLS_\LLVM\bin\clangd.exe --compile-commands-dir=%SRCPATH% --path-mappings="\\192.168.105.50\workfolder\_COMMONSRC_\SimpleCommon\"="C:\WORKFOLDER\_PUBLIC_\FrameworkTestApp\COMMON\" --check="%SRCPATH%../AppMain.cpp"

rem --clang-tidy
rem :\clang-tidy HelloWorld.c -checks=* --

pause