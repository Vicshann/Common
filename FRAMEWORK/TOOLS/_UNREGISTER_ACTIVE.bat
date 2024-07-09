rem The project directory name must be unique
rem Do not remove such directory junctions manually!

setlocal EnableDelayedExpansion 
 
set WORKDIR=%~dp0
set CPRJPATH=%WORKDIR:~0,-1%

echo %WORKDIR%
echo %CPRJPATH%

for %%a in ("%CPRJPATH%") do set prj_name=%%~nxa

rem fsutil reparsepoint delete PATH

rmdir "%ACTPRJDIR%\%prj_name%"

pause