@echo off
setlocal enabledelayedexpansion

echo =========================================
echo iOS Device Manager - Build Script
echo =========================================
echo.

REM Initialize compiler type variable
set "COMPILER_TYPE="

REM Detect MSVC toolchain
echo Detecting MSVC toolchain...

REM Try to find and configure MSVC
call :detect_msvc
if defined COMPILER_TYPE (
    echo [INFO] MSVC toolchain detected and configured
    goto configure_qt
) else (
    echo [ERROR] MSVC toolchain not found
    echo [ERROR] Please install Visual Studio 2019/2022 with C++ support
    goto end
)

:configure_qt
REM Configure Qt MSVC environment
REM Check for MSVC-compiled Qt versions
if exist "D:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake" (
    set "PATH=D:\Qt\6.10.1\msvc2022_64\bin;!PATH!"
    set "Qt6_DIR=D:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6"
    echo [INFO] Qt 6.10.1 MSVC2022 64-bit configured
    goto configure_cmake
)


echo [ERROR] No MSVC-compiled Qt installation found
echo [ERROR] Please install Qt 6.x with MSVC2022 64-bit support
echo [INFO] Download from: https://www.qt.io/download
goto end

:configure_cmake
REM Configure Qt CMake environment (prioritize Qt's CMake)
if exist "D:\Qt\Tools\CMake_64\bin\cmake.exe" (
    set "PATH=D:\Qt\Tools\CMake_64\bin;!PATH!"
    echo [INFO] Qt CMake configured
)
goto main_logic

REM Function to detect MSVC compiler
:detect_msvc
REM Check if Visual Studio is available through vcvarsall.bat
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    set "COMPILER_TYPE=MSVC"
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
    set "CMAKE_ARCH=-A x64"
    echo [INFO] Visual Studio 2022 Professional detected
    goto :eof
)

if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    set "COMPILER_TYPE=MSVC"
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
    set "CMAKE_ARCH=-A x64"
    echo [INFO] Visual Studio 2022 Community detected
    goto :eof
)

if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    set "COMPILER_TYPE=MSVC"
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
    set "CMAKE_ARCH=-A x64"
    echo [INFO] Visual Studio 2022 Enterprise detected
    goto :eof
)

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    set "COMPILER_TYPE=MSVC"
    set "CMAKE_GENERATOR=Visual Studio 16 2019"
    set "CMAKE_ARCH=-A x64"
    echo [INFO] Visual Studio 2019 Professional detected
    goto :eof
)

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    set "COMPILER_TYPE=MSVC"
    set "CMAKE_GENERATOR=Visual Studio 16 2019"
    set "CMAKE_ARCH=-A x64"
    echo [INFO] Visual Studio 2019 Community detected
    goto :eof
)

REM Check if cl.exe is directly available (Build Tools or already configured environment)
where cl >nul 2>&1
if !errorlevel!==0 (
    set "COMPILER_TYPE=MSVC"
    set "CMAKE_GENERATOR=NMake Makefiles"
    set "CMAKE_ARCH="
    echo [INFO] MSVC compiler (cl.exe) found in PATH
)
goto :eof


:main_logic

echo.

if "%1"=="check-deps" goto check_deps
if "%1"=="help" goto show_help
if "%1"=="" goto full_build
goto show_help

:check_deps
echo Checking dependencies...
echo.

REM Check Qt CMake configuration
if defined Qt6_DIR (
    if exist "%Qt6_DIR%\Qt6Config.cmake" (
        echo [OK] Qt CMake configuration found
        echo     Qt6_DIR: %Qt6_DIR%
    ) else (
        echo [ERROR] Qt6Config.cmake not found at %Qt6_DIR%
    )
) else (
    echo [ERROR] Qt6_DIR not configured
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

REM Check MSVC compiler
where cl >nul 2>&1
if !errorlevel!==0 (
    echo [OK] MSVC compiler found
    for /f "tokens=*" %%i in ('cl 2^>^&1 ^| findstr "Microsoft"') do echo     %%i
) else (
    echo [ERROR] MSVC compiler not accessible
    echo     Please ensure Visual Studio is properly installed and configured
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

REM Check Qt CMake configuration
if defined Qt6_DIR (
    if exist "%Qt6_DIR%\Qt6Config.cmake" (
        echo [OK] Qt CMake configuration found
    ) else (
        echo [ERROR] Qt6Config.cmake not found - Cannot proceed
        goto end
    )
) else (
    echo [ERROR] Qt6_DIR not configured - Cannot proceed
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

REM Check MSVC compiler availability
where cl >nul 2>&1
if !errorlevel!==0 (
    echo [OK] MSVC compiler found
) else (
    echo [ERROR] MSVC compiler not accessible - Cannot proceed
    echo     Please ensure Visual Studio is properly installed and configured
    goto end
)

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

echo Running CMake configuration with !CMAKE_GENERATOR! generator...
echo [INFO] Using compiler: !COMPILER_TYPE!

if "%CMAKE_GENERATOR%"=="Visual Studio 17 2022" (
    cmake .. -G "%CMAKE_GENERATOR%" %CMAKE_ARCH% -DCMAKE_BUILD_TYPE=Release -DQt6_DIR="%Qt6_DIR%"
) else if "%CMAKE_GENERATOR%"=="Visual Studio 16 2019" (
    cmake .. -G "%CMAKE_GENERATOR%" %CMAKE_ARCH% -DCMAKE_BUILD_TYPE=Release -DQt6_DIR="%Qt6_DIR%"
) else (
    cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=Release -DQt6_DIR="%Qt6_DIR%"
)

if !errorlevel! neq 0 (
    echo [ERROR] CMake configuration failed
    cd ..
    goto end
)

echo.
echo Building project with !COMPILER_TYPE! toolchain...
if "%CMAKE_GENERATOR%"=="Visual Studio 17 2022" (
    cmake --build . --config Release --parallel
) else if "%CMAKE_GENERATOR%"=="Visual Studio 16 2019" (
    cmake --build . --config Release --parallel  
) else (
    cmake --build . --config Release
)
if !errorlevel! neq 0 (
    echo [ERROR] Build failed
    cd ..
    goto end
)

echo.
echo [SUCCESS] Build completed!

REM Check for executable and deploy Qt dependencies
if exist "Release\phone-linkc.exe" (
    echo.
    echo Executable found at: Release\phone-linkc.exe
    echo.
    echo Deploying Qt dependencies...
    
    REM Create deployment directory
    if not exist "Release\deploy" mkdir "Release\deploy"
    
    REM Copy executable to deployment directory
    copy "Release\phone-linkc.exe" "Release\deploy\" >nul
    
    REM Use windeployqt to deploy Qt dependencies (minimal deployment)
    if exist "D:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe" (
        echo Running windeployqt [minimal deployment]...
        "D:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe" --release --no-translations --no-system-d3d "Release\deploy\phone-linkc.exe"
        if !errorlevel!==0 (
            echo [SUCCESS] Qt dependencies deployed successfully [minimal package]!
            echo [INFO] Standalone executable available at: Release\deploy\phone-linkc.exe
            echo [INFO] This version can be double-clicked to run independently
            echo [INFO] Minimal deployment: translations and system D3D excluded
        ) else (
            echo [WARNING] windeployqt failed, but executable may still work
        )
    ) else (
        echo [WARNING] windeployqt.exe not found, executable may need Qt DLLs in PATH
    )
    
    echo.
    set /p run="Run application? (y/n): "
    if /i "!run!"=="y" (
        echo Starting application...
        if exist "Release\deploy\phone-linkc.exe" (
            "Release\deploy\phone-linkc.exe"
        ) else (
            "Release\phone-linkc.exe"
        )
    )
) else if exist "Debug\phone-linkc.exe" (
    echo.
    echo Executable found at: Debug\phone-linkc.exe
    echo.
    echo Deploying Qt dependencies for Debug build...
    
    if not exist "Debug\deploy" mkdir "Debug\deploy"
    copy "Debug\phone-linkc.exe" "Debug\deploy\" >nul
    
    if exist "D:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe" (
        echo Running windeployqt for Debug [minimal deployment]...
        "D:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe" --debug --no-translations --no-system-d3d "Debug\deploy\phone-linkc.exe"
        if !errorlevel!==0 (
            echo [SUCCESS] Qt dependencies deployed successfully [minimal package]!
            echo [INFO] Standalone executable available at: Debug\deploy\phone-linkc.exe
            echo [INFO] Minimal deployment: translations and system D3D excluded
        ) else (
            echo [WARNING] windeployqt failed for Debug build
        )
    ) else (
        echo [WARNING] windeployqt.exe not found, executable may need Qt DLLs in PATH
    )
    
    echo.
    set /p run="Run application? (y/n): "
    if /i "!run!"=="y" (
        echo Starting application...
        if exist "Debug\deploy\phone-linkc.exe" (
            "Debug\deploy\phone-linkc.exe"
        ) else (
            "Debug\phone-linkc.exe"
        )
    )
) else if exist "phone-linkc.exe" (
    echo.
    echo Executable found at: phone-linkc.exe
    set /p run="Run application? (y/n): "
    if /i "!run!"=="y" (
        echo Starting application...
        phone-linkc.exe
    )
) else (
    echo Note: Executable not found in expected locations
    echo Checked: Release\phone-linkc.exe, Debug\phone-linkc.exe, phone-linkc.exe
    echo Build completed successfully but executable location may be different
)

cd ..

:end
echo.
echo Press any key to exit...
pause >nul
