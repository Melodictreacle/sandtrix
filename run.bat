@echo off
title Sandtrix - Sand Tetris for Qt
cd /d "%~dp0"

set "PYTHON_PATH="

if exist "C:\msys64\ucrt64\bin\python.exe" (
    set "PYTHON_PATH=C:\msys64\ucrt64\bin\python.exe"
) else (
    where python >nul 2>nul
    if %errorlevel% equ 0 (
        set "PYTHON_PATH=python"
    )
)

if "%PYTHON_PATH%"=="" (
    echo [ERROR] Python not found. Please ensure Python and PyQt6 are installed.
    pause
    exit /b 1
)

"%PYTHON_PATH%" main.py
if %errorlevel% neq 0 (
    echo.
    echo Game exited with error code %errorlevel%.
    pause
)
