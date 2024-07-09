rem The project directory name must be unique
rem 'syncthing' can follow only directory junctions 'mklink J'

setlocal EnableDelayedExpansion 
 
set WORKDIR=%~dp0
set CPRJPATH=%WORKDIR:~0,-1%

echo %WORKDIR%
echo %CPRJPATH%

for %%a in ("%CPRJPATH%") do set prj_name=%%~nxa

mklink /J "%ACTPRJDIR%\%prj_name%" "%CPRJPATH%"

pause