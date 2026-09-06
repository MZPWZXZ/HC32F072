@echo off
setlocal
rem ============================================================================
rem  build.bat - one-click build helper for HC32F072KA
rem  (CMake + MinGW Makefiles + arm-none-eabi-gcc)
rem
rem  Usage:
rem    build.bat             configure + build Debug   -> build\hc32f072ka.elf
rem    build.bat release     configure + build Release (-O2)
rem    build.bat minsize     configure + build MinSizeRel (-Os)
rem    build.bat clean       remove build directory and exit
rem
rem  Tool auto-detection order:
rem    1) environment override: HC32F072_CMAKE / HC32F072_MAKE
rem    2) PATH lookup
rem    3) well-known local install dirs under C:\Program Files\UserApp
rem
rem  The script is ASCII-only on purpose so it works on any system codepage.
rem ============================================================================

set "SCRIPT_DIR=%~dp0"
rem strip the trailing backslash of %~dp0 so quoted paths do not end with \"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "BUILD_DIR=%SCRIPT_DIR%\build"
set "BUILD_TYPE=Debug"
set "TOOLCHAIN_FILE=%SCRIPT_DIR%\cmake\arm-none-eabi-gcc.cmake"
set "CMAKE_EXE="
set "MAKE_EXE="
set "GCC_DIR="

rem ------------------------- parse command line args -------------------------
if /I "%~1"=="clean" (
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo [build] removed %BUILD_DIR%
    exit /b 0
)
if /I "%~1"=="release" set "BUILD_TYPE=Release"
if /I "%~1"=="minsize" set "BUILD_TYPE=MinSizeRel"

rem ------------------------------ locate cmake -------------------------------
if defined HC32F072_CMAKE set "CMAKE_EXE=%HC32F072_CMAKE%"
if not defined CMAKE_EXE for /f "delims=" %%i in ('where cmake 2^>nul') do if not defined CMAKE_EXE set "CMAKE_EXE=%%i"
if not defined CMAKE_EXE for /d %%d in ("C:\Program Files\UserApp\cmake-*") do if exist "%%d\bin\cmake.exe" if not defined CMAKE_EXE set "CMAKE_EXE=%%d\bin\cmake.exe"
if not defined CMAKE_EXE (
    echo [ERROR] cmake not found. Put cmake on PATH or set HC32F072_CMAKE to cmake.exe
    exit /b 1
)

rem ----------------------- locate arm-none-eabi-gcc ---------------------------
if not defined GCC_DIR for /f "delims=" %%i in ('where arm-none-eabi-gcc 2^>nul') do if not defined GCC_DIR set "GCC_DIR=%%~dpi"
if not defined GCC_DIR for /d %%d in ("C:\Program Files\UserApp\arm-gnu-toolchain-*") do if exist "%%d\bin\arm-none-eabi-gcc.exe" if not defined GCC_DIR set "GCC_DIR=%%d\bin\"
if not defined GCC_DIR (
    echo [ERROR] arm-none-eabi-gcc not found. Put it on PATH or install the Arm GNU toolchain
    exit /b 1
)

rem ------------------------ locate mingw32-make -------------------------------
if defined HC32F072_MAKE set "MAKE_EXE=%HC32F072_MAKE%"
if not defined MAKE_EXE for /f "delims=" %%i in ('where mingw32-make 2^>nul') do if not defined MAKE_EXE set "MAKE_EXE=%%i"
if not defined MAKE_EXE if exist "C:\Program Files\UserApp\mingw64\bin\mingw32-make.exe" set "MAKE_EXE=C:\Program Files\UserApp\mingw64\bin\mingw32-make.exe"
if not defined MAKE_EXE (
    echo [ERROR] mingw32-make not found. Needed by the MinGW Makefiles generator.
    echo         Put it on PATH or set HC32F072_MAKE to mingw32-make.exe
    exit /b 1
)

rem make sure the cross toolchain directory is reachable by cmake
if defined GCC_DIR set "PATH=%GCC_DIR%%PATH%"

echo [build] cmake   : %CMAKE_EXE%
echo [build] make    : %MAKE_EXE%
echo [build] gcc dir : %GCC_DIR%
echo [build] type    : %BUILD_TYPE%
echo.

rem ------------------------------ configure ----------------------------------
"%CMAKE_EXE%" -S "%SCRIPT_DIR%" -B "%BUILD_DIR%" -G "MinGW Makefiles" "-DCMAKE_MAKE_PROGRAM=%MAKE_EXE%" "-DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN_FILE%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo [ERROR] cmake configure failed
    exit /b 1
)

rem -------------------------------- build ------------------------------------
"%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel
if errorlevel 1 (
    echo [ERROR] build failed
    exit /b 1
)

echo.
echo [build] OK. Artifacts in %BUILD_DIR%:
echo         %BUILD_DIR%\hc32f072ka.elf
echo         %BUILD_DIR%\hc32f072ka.hex
echo         %BUILD_DIR%\hc32f072ka.bin
echo         %BUILD_DIR%\hc32f072ka.map
exit /b 0
