@echo off
setlocal
title Sandtrix - C++ Qt Build & Run
cd /d "%~dp0"

echo ========================================================
echo        SANDTRIX - Sand Tetris for Qt (C++)
echo ========================================================
echo.

set "QT_DIR=C:\Qt\6.12.0\mingw_64"
set "MINGW_DIR=C:\Qt\Tools\mingw1310_64"
set "CMAKE_DIR=C:\Qt\Tools\CMake_64"
set "NINJA_DIR=C:\Qt\Tools\Ninja"

if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [ERROR] Qt 6 installation not found at %QT_DIR%
    pause
    exit /b 1
)

set "PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%CMAKE_DIR%\bin;%NINJA_DIR%;%PATH%"

echo [1/3] Configuring CMake project...
if not exist "build" mkdir "build"
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_C_COMPILER="%MINGW_DIR%\bin\gcc.exe" -DCMAKE_CXX_COMPILER="%MINGW_DIR%\bin\g++.exe" -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

echo.
echo [2/3] Compiling C++ Sandtrix...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo [3/3] Launching Sandtrix...
start "" "build\sandtrix.exe"

echo Done!
