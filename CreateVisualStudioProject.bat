@REM Create JKG projects for Visual Studio using CMake
@echo off
for %%X in (cmake.exe) do (set FOUND=%%~$PATH:X)
if not defined FOUND (
	echo CMake was not found on your system. Please make sure you have installed CMake
	echo from http://www.cmake.org/ and cmake.exe is installed to your system's PATH
	echo environment variable.
	echo.
	pause
	exit /b 1
) else (
	echo Found CMake!
	cmake --version
)

:: Use the latest visual studio installed, we only support 2017 - 2022 right now.
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\" (
   echo Found MSVS2017
   set "VS_TAG=Visual Studio 15 2017"
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\" (
   echo Found MSVS2019
   set "VS_TAG=Visual Studio 16 2019"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\" (
   echo Found MSVS2022
   set "VS_TAG=Visual Studio 17 2022"
)
::requires cmake v4.3.0+, remove if using older cmake
::if exist "C:\Program Files\Microsoft Visual Studio\18\" (
::   echo Found MSVS2026
::   set "VS_TAG=Visual Studio 18 2026"
::)

::insert custom install location here (if not in c:\Program Files\)
::if exist "C:\" (
::    echo Found MSVS2022
::    set "VS_TAG=Visual Studio 17 2022"
::)

::No MSVS!
if not defined VS_TAG (
   echo Visual Studio is not installed, cannot setup install!
   echo.
   pause
   exit /b 1
)

::Success, proceed!
if not exist build\nul (mkdir build)
pushd build
cmake -G "%VS_TAG%" -A Win32 -D CMAKE_INSTALL_PREFIX=../install ..
popd
pause