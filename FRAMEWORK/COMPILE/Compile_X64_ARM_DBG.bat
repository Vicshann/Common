set CLANGPATH=C:\_TOOLS_\_DEVTOOLS_\LLVM\bin
set SRCPATH=%~dp0
set OUTDIR=%SRCPATH%/../BUILD/bin/Release/
rem cd %CLANGPATH%/bin
cd %SRCPATH%/../

%CLANGPATH%/clang.exe -o "%OUTDIR%/UBootMgr64.bin" --config "%SRCPATH%ReleaseX64.cfg" "%SRCPATH%../Main.cpp"

pause