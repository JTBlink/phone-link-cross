@echo off
setlocal enabledelayedexpansion

:: Qt Windows Installation Script Wrapper
:: Calls Python script for cross-platform compatibility

echo ========================================
echo Qt Windows Installation Script
echo ========================================
echo.

:: Check if Python is available
python --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] Python is not installed or not in PATH
    echo [INFO] Please install Python 3.6 or higher
    echo [INFO] Download from: https://www.python.org/downloads/
    pause
    exit /b 1
)

:: Get script directory
set SCRIPT_DIR=%~dp0
set PYTHON_SCRIPT=%SCRIPT_DIR%install_qt.py

:: Check if Python script exists
if not exist "%PYTHON_SCRIPT%" (
    echo [ERROR] Python script not found: %PYTHON_SCRIPT%
    pause
    exit /b 1
)

:: Run Python script
echo [INFO] Starting Qt installation using Python...
python "%PYTHON_SCRIPT%"

:: Check result
if %errorLevel% equ 0 (
    echo [SUCCESS] Installation completed successfully
) else (
    echo [ERROR] Installation failed
)

pause
exit /b %errorLevel%