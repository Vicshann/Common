set CLANGPATH=C:\_TOOLS_\_DEVTOOLS_\LLVM\bin
set OSSLPATH=C:\_TOOLS_\_DEVTOOLS_\Git\usr\bin
set SRCPATH=%~dp0


:: Build final binary
%CLANGPATH%\clang.exe -S -emit-llvm --config "%SRCPATH%COMPILE\REL_LIN_ARM_X32.cfg" -D_RELEASE -D_CRYPTINIT "%SRCPATH%Main.cpp"



pause