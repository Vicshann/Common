set CLANGPATH=C:/TEST/MyClang
set SRCPATH=%~dp0
cd %CLANGPATH%/bin

clang.exe -o "%SRCPATH%bin/ArmBoot.so" --config "%SRCPATH%Compile_X32_ARM_DBG.cfg" "%SRCPATH%Main.cpp"

pause