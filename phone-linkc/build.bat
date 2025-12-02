@echo off
setlocal enabledelayedexpansion

echo =========================================
echo iOS Device Manager - Build Script
echo =========================================
echo.

REM Configure Qt environment
if exist "D:\Qt\6.10.1\msvc2022_64\bin\qmake.exe" (
    set "PATH=D:\Qt\6.10.1\msvc2022_64\bin;!PATH!"
    echo [INFO] Qt 6.10.1 MSVC2022 64-bit configured
) else if exist "D:\Qt\6.10.1\mingw_64\bin\qmake.exe" (
    set "PATH=D:\Qt\6.10.1\mingw_64\bin;!PATH!"
    echo [INFO] Qt 6.10.1 MinGW 64-bit configured
) else if exist "D:\Qt\6.9.3\msvc2022_64\bin\qmake.exe" (
    set "PATH=D:\Qt\6.9.3\msvc2022_64\bin;!PATH!"
    echo [INFO] Qt 6.9.3 MSVC2022 64-bit configured
)

REM Configure Qt CMake environment (prioritize Qt's CMake)
if exist "D:\Qt\Tools\CMake_64\bin\cmake.exe" (
    set "PATH=D:\Qt\Tools\CMake_64\bin;!PATH!"
    echo [INFO] Qt CMake configured
)

REM Configure MinGW compiler environment
if exist "D:\Qt\Tools\mingw1310_64\bin\g++.exe" (
    set "PATH=D:\Qt\Tools\mingw1310_64\bin;!PATH!"
    echo [INFO] MinGW 13.1.0 64-bit compiler configured
) else if exist "D:\Qt\Tools\mingw1120_64\bin\g++.exe" (
    set "PATH=D:\Qt\Tools\mingw1120_64\bin;!PATH!"
    echo [INFO] MinGW 11.2.0 64-bit compiler configured
) else if exist "D:\Qt\Tools\mingw900_64\bin\g++.exe" (
    set "PATH=D:\Qt\Tools\mingw900_64\bin;!PATH!"
    echo [INFO] MinGW 9.0.0 64-bit compiler configured
)

echo.

if "%1"=="check-deps" goto check_deps
if "%1"=="help" goto show_help
if "%1"=="" goto full_build
goto show_help

:check_deps
echo Checking dependencies...
echo.

REM Check Qt
where qmake >nul 2>&1
if !errorlevel!==0 (
    echo [OK] Qt found
    for /f "tokens=*" %%i in ('qmake -version ^| findstr "Qt version"') do echo     %%i
) else (
    echo [ERROR] Qt not found
)

echo.

REM Check CMake  
where cmake >nul 2>&1
if !errorlevel!==0 (
    echo [OK] CMake found
    for /f "tokens=*" %%i in ('cmake --version ^| findstr "cmake version"') do echo     %%i
) else (
    echo [ERROR] CMake not found - Please install from https://cmake.org/download/
)

echo.

REM Check compiler
where cl >nul 2>&1
if !errorlevel!==0 (
    echo [OK] MSVC compiler found
) else (
    where g++ >nul 2>&1
    if !errorlevel!==0 (
        echo [OK] MinGW compiler found
        for /f "tokens=*" %%i in ('g++ --version ^| findstr "gcc version"') do echo     %%i
    ) else (
        echo [ERROR] No compiler found
        echo     Please ensure MinGW or Visual Studio is properly installed
    )
)

echo.
goto end

:show_help
echo Usage: build.bat [option]
echo.
echo Options:
echo   check-deps  - Check dependencies
echo   help        - Show this help
echo   (no args)   - Run full build
echo.
goto end

:full_build
echo Starting full build process...
echo.
goto check_deps_inline

:check_deps_inline
echo Checking dependencies...
echo.

REM Check Qt
where qmake >nul 2>&1
if !errorlevel!==0 (
    echo [OK] Qt found
) else (
    echo [ERROR] Qt not found - Cannot proceed
    goto end
)

REM Check CMake
where cmake >nul 2>&1
if !errorlevel!==0 (
    echo [OK] CMake found
) else (
    echo [ERROR] CMake not found - Cannot proceed
    goto end
)

REM Check compiler
where cl >nul 2>&1
if !errorlevel!==0 (
    echo [OK] MSVC compiler found
    set "COMPILER_FOUND=1"
) else (
    where g++ >nul 2>&1
    if !errorlevel!==0 (
        echo [OK] MinGW compiler found
        set "COMPILER_FOUND=1"
    ) else (
        echo [ERROR] No compiler found - Cannot proceed
        goto end
    )
)

if not defined COMPILER_FOUND goto end

echo.
echo All dependencies satisfied!
echo.
set /p choice="Continue with build? (y/n): "
if /i not "!choice!"=="y" goto end

echo.
echo Building project...
echo.

if exist build (
    echo Cleaning previous build...
    rmdir /s /q build
)
mkdir build
cd build

echo Running CMake configuration...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if !errorlevel! neq 0 (
    echo [ERROR] CMake configuration failed
    cd ..
    goto end
)

echo.
echo Building project...
cmake --build . --config Release
if !errorlevel! neq 0 (
    echo [ERROR] Build failed
    cd ..
    goto end
)

echo.
echo [SUCCESS] Build completed!

if exist "phone-linkc.exe" (
    echo.
    set /p run="Run application? (y/n): "
    if /i "!run!"=="y" (
        echo Starting application...
        phone-linkc.exe
    )
) else (
    echo Note: Executable not found in expected location
)

cd ..

:end
echo.
echo Press any key to exit...
pause >nul
