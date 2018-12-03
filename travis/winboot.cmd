@echo off
SETLOCAL EnableExtensions
REM Script to build the Open Watcom bootstrap tools
REM By Microsoft Visual Studio
REM ...
REM Remove NMAKE macros wrongly defined by Travis
echo CC=%CC%
echo CFLAGS=%CFLAGS%
echo CPP=%CPP%
echo CPPFLAGS=%CPPFLAGS%
echo CXX=%CXX%
echo CXXFLAGS=%CXXFLAGS%
set CC=
set CFLAGS=
set CPP=
set CPPFLAGS=
set CXX=
set CXXFLAGS=
REM ...
set OWROOT=%CD%
REM ...
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
REM ...
call cmnvars.bat
REM ...
cd %OWSRCDIR%\wmake
mkdir %OWOBJDIR%
cd %OWOBJDIR%
nmake -f ..\nmake clean
nmake -f ..\nmake
if not errorlevel == 1 (
    cd %OWSRCDIR%\builder
    mkdir %OWOBJDIR%
    cd %OWOBJDIR%
    %OWBINDIR%\wmake -f ..\binmake clean
    %OWBINDIR%\wmake -f ..\binmake bootstrap=1 builder.exe
    if not errorlevel == 1 (
	cd %OWSRCDIR%
        if "%TRAVIS_EVENT_TYPE%" == "pull_request" (
            builder boot
        ) else (
            builder -q boot
        )
    )
)
