rem DO NOT RUN THIS FROM A NETWORK DRIVE!
rem For a new versions of VisualStudio which place their special files in ".vs" directory
rem Every solution must have an unique name
rem Do not forget to define PRJBUILDDIR and COMMONSRCDIR environment variales
rem Please do not make a copy of the Common folder. Always keep it in one place. 
rem Link each project to it and make a backup with an entire project folder to have a working copy.

rem if not defined PRJBUILDDIR goto NoBuild
if not defined COMMONSRCDIR goto NoCommon

rem for /f "delims=" %%F in ('dir "%~dp0*.sln" /b /o-n') do set sln_name=%%F
rem if not defined sln_name goto NoSlnFile
rem ECHO sln_name

rem SET  sln_fldr=%sln_name:~0,-4%
rem ECHO sln_fldr

rem set build_dir=%PRJBUILDDIR%\%sln_fldr%
rem ECHO build_dir

rem set vs_dir=%PRJBUILDDIR%\.vs
rem ECHO vs_dir

rem mkdir %vs_dir%
rem mkdir %build_dir%

rem mklink /J ".\.vs" "%vs_dir%"
rem mklink /J ".\BUILD" "%build_dir%"
mklink /J ".\COMMON" "%COMMONSRCDIR%"

if defined BACKUPSRCDIR (
mklink /J "%BACKUPSRCDIR%\%sln_fldr%" "%~dp0"
)

ECHO "Success!"
goto Exit

:NoSlnFile
ECHO "No solution file!"
goto Exit

:NoCommon
ECHO "No common folder EVAR!"
goto Exit

:NoBuild
ECHO "No build folder EVAR!"

:Exit
pause