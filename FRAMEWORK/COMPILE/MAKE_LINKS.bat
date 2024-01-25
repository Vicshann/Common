rem This file must be copied into root folder of every project (its name is looked up by all compile scripts)
 
setlocal EnableDelayedExpansion 
 
set WORKDIR=%~dp0
set CMNPATH=%CMNSRCDIR% 
rem set COMPATH=%CLANGDIR%

mklink /D ".\COMMON" "%CMNPATH%"

set WORKDIR=%WORKDIR:~0,-1%
for %%i in (%WORKDIR%) do set prjname="%%~ni"
echo Found the project name - %prjname%

if defined COMPATH (

 for /r "%COMPATH%\" %%f in (*.h) do (
    if "%%~nxf"=="intrin.h" set fwp=%%~dpf
 )

 set INCPATH=!fwp:~0,-1!
 echo Found include directory - !INCPATH!

 mkdir ".\COMPILER"
 mklink /D ".\COMPILER\bin" "%COMPATH%bin"
 mklink /D ".\COMPILER\include" "!INCPATH!"
)

if defined PRJBUILDDIR (
 mkdir "%PRJBUILDDIR%"
 for /f "delims=" %%F in ('dir "%~dp0*.sln" /b /o-n') do set sln_name=%%F
 if defined sln_name (
  set vs_dir=%PRJBUILDDIR%\.vs
  mkdir !vs_dir!
  mklink /J ".\.vs" "!vs_dir!"
 )
 set build_dir=%PRJBUILDDIR%\%prjname%
 mkdir !build_dir!
 mklink /J ".\BUILD" "!build_dir!"
)

if defined BACKUPSRCDIR (
 mkdir "%BACKUPSRCDIR%"
 mklink /J "%BACKUPSRCDIR%\%prjname%" "%~dp0"
)

pause