@echo off
REM 设置控制台编码为 UTF-8
chcp 65001 >nul 2>&1

setlocal enabledelayedexpansion

echo =========================================
echo iOS Device Manager - Build Script
echo =========================================
echo.

REM Initialize compiler type variable
set "COMPILER_TYPE="

REM ===== Jump to main logic to avoid executing function definitions =====
goto main_logic

REM ===== Constants Definition =====

:init_constants
REM Visual Studio path constants
set "VS2022_BASE=%ProgramFiles%\Microsoft Visual Studio\2022"
set "VS2019_BASE=%ProgramFiles(x86)%\Microsoft Visual Studio\2019"
set "VCVARS_SUFFIX=\VC\Auxiliary\Build\vcvarsall.bat"

REM Qt path constants
set "QT_BASE=D:\Qt\6.10.1\msvc2022_64"
set "QT_CMAKE_TOOLS=D:\Qt\Tools\CMake_64\bin\cmake.exe"
set "QT_WINDEPLOYQT=D:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe"

REM Build config constants (will be set based on parameter)
set "BUILD_CONFIG=Release"
set "CMAKE_BUILD_TYPE=Release"

REM libimobiledevice path constants  
set "SCRIPT_DIR=%~dp0"
set "LIBIMOBILEDEVICE_ROOT=%SCRIPT_DIR%thirdparty\libimobiledevice"
set "LIBIMOBILEDEVICE_BIN=%LIBIMOBILEDEVICE_ROOT%"
set "LIBIMOBILEDEVICE_LIB=%LIBIMOBILEDEVICE_ROOT%"
set "LIBIMOBILEDEVICE_INCLUDE=%LIBIMOBILEDEVICE_ROOT%\include"
goto :eof

REM ===== Log Function Definitions =====

REM Info log
:log_info
echo [INFO] %~1
goto :eof

REM Error log
:log_error
echo [ERROR] %~1
goto :eof

REM Success log
:log_success
echo [SUCCESS] %~1
goto :eof

REM OK log
:log_ok
echo [OK] %~1
goto :eof

REM Warning log
:log_warning
echo [WARNING] %~1
goto :eof

REM Indented info log
:log_info_indent
echo     %~1
goto :eof

REM Message log
:log_message
echo %~1
goto :eof

REM New line
:log_newline
echo.
goto :eof

:main_logic
REM Set default build configuration to Debug
set "BUILD_CONFIG=Debug"
set "CMAKE_BUILD_TYPE=Debug"

REM Parse build type parameter
if /i "%1"=="debug" (
    set "BUILD_CONFIG=Debug"
    set "CMAKE_BUILD_TYPE=Debug"
    shift
    goto parse_remaining_args
)
if /i "%1"=="release" (
    set "BUILD_CONFIG=Release"
    set "CMAKE_BUILD_TYPE=Release"
    shift
    goto parse_remaining_args
)

:parse_remaining_args
if "%1"=="check-deps" goto check_deps
if "%1"=="install-deps" goto install_deps
if "%1"=="help" goto show_help  
if "%1"=="" goto init_build
if /i "%1"=="debug" goto init_build
if /i "%1"=="release" goto init_build
goto show_help

REM ===== Common Function Definitions =====

REM Try to configure specified Visual Studio version
REM Parameters: %1=base path, %2=edition name, %3=CMake generator, %4=architecture
:try_configure_vs
set "VCVARS_PATH=%~1\%~2%VCVARS_SUFFIX%"
if exist "%VCVARS_PATH%" (
    call "%VCVARS_PATH%" x64 >nul 2>&1
    set "COMPILER_TYPE=MSVC"
    set "CMAKE_GENERATOR=%~3"
    set "CMAKE_ARCH=%~4"
    call :log_info "%~2 detected"
    exit /b 0
)
exit /b 1

REM MSVC compiler detection function
:detect_msvc
REM Initialize constants (preserve BUILD_CONFIG and CMAKE_BUILD_TYPE)
set "SAVED_BUILD_CONFIG=%BUILD_CONFIG%"
set "SAVED_CMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE%"
call :init_constants
set "BUILD_CONFIG=%SAVED_BUILD_CONFIG%"
set "CMAKE_BUILD_TYPE=%SAVED_CMAKE_BUILD_TYPE%"

REM Try Visual Studio 2022 editions first
call :try_configure_vs "%VS2022_BASE%" "Professional" "Visual Studio 17 2022" "-A x64"
if !errorlevel!==0 goto :eof

call :try_configure_vs "%VS2022_BASE%" "Community" "Visual Studio 17 2022" "-A x64"
if !errorlevel!==0 goto :eof

call :try_configure_vs "%VS2022_BASE%" "Enterprise" "Visual Studio 17 2022" "-A x64"
if !errorlevel!==0 goto :eof

REM Try Visual Studio 2019 editions
call :try_configure_vs "%VS2019_BASE%" "Professional" "Visual Studio 16 2019" "-A x64"
if !errorlevel!==0 goto :eof

call :try_configure_vs "%VS2019_BASE%" "Community" "Visual Studio 16 2019" "-A x64"
if !errorlevel!==0 goto :eof

REM Check if cl.exe is directly available
where cl >nul 2>&1
if !errorlevel!==0 (
    set "COMPILER_TYPE=MSVC"
    set "CMAKE_GENERATOR=NMake Makefiles"
    set "CMAKE_ARCH="
    call :log_info "MSVC compiler (cl.exe) found in PATH"
)
goto :eof

REM CMake configuration function
:run_cmake_configure
REM Preserve build configuration
set "SAVED_BUILD_CONFIG=%BUILD_CONFIG%"
set "SAVED_CMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE%"
call :init_constants
set "BUILD_CONFIG=%SAVED_BUILD_CONFIG%"
set "CMAKE_BUILD_TYPE=%SAVED_CMAKE_BUILD_TYPE%"
echo [INFO] Executing CMake configuration:
if "%CMAKE_GENERATOR%"=="Visual Studio 17 2022" (
    echo [INFO]   cmake .. -G "%CMAKE_GENERATOR%" %CMAKE_ARCH% -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DQt6_DIR="%Qt6_DIR%"
    cmake .. -G "%CMAKE_GENERATOR%" %CMAKE_ARCH% -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DQt6_DIR="%Qt6_DIR%"
) else if "%CMAKE_GENERATOR%"=="Visual Studio 16 2019" (
    echo [INFO]   cmake .. -G "%CMAKE_GENERATOR%" %CMAKE_ARCH% -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DQt6_DIR="%Qt6_DIR%"
    cmake .. -G "%CMAKE_GENERATOR%" %CMAKE_ARCH% -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DQt6_DIR="%Qt6_DIR%"
) else (
    echo [INFO]   cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DQt6_DIR="%Qt6_DIR%"
    cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DQt6_DIR="%Qt6_DIR%"
)
goto :eof

REM CMake build function
:run_cmake_build
REM Preserve build configuration
set "SAVED_BUILD_CONFIG=%BUILD_CONFIG%"
set "SAVED_CMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE%"
call :init_constants
set "BUILD_CONFIG=%SAVED_BUILD_CONFIG%"
set "CMAKE_BUILD_TYPE=%SAVED_CMAKE_BUILD_TYPE%"
echo [INFO] Executing CMake build:
if "%CMAKE_GENERATOR%"=="Visual Studio 17 2022" (
    echo [INFO]   cmake --build . --config %BUILD_CONFIG% --parallel
    cmake --build . --config %BUILD_CONFIG% --parallel
) else if "%CMAKE_GENERATOR%"=="Visual Studio 16 2019" (
    echo [INFO]   cmake --build . --config %BUILD_CONFIG% --parallel
    cmake --build . --config %BUILD_CONFIG% --parallel
) else (
    echo [INFO]   cmake --build . --config %BUILD_CONFIG%
    cmake --build . --config %BUILD_CONFIG%
)
goto :eof

REM libimobiledevice check function
:check_libimobiledevice
call :init_constants
echo libimobiledevice status check:

REM Check default installation path
if exist "%LIBIMOBILEDEVICE_ROOT%" (
    call :log_ok "libimobiledevice installation directory found"
    call :log_info_indent "Path: %LIBIMOBILEDEVICE_ROOT%"
    
    REM Check key executable files
    if exist "%LIBIMOBILEDEVICE_BIN%\idevice_id.exe" (
        call :log_ok "idevice_id.exe found"
        
        REM Test device connection
        call :log_info_indent "Testing device connection..."
        "%LIBIMOBILEDEVICE_BIN%\idevice_id.exe" -l >nul 2>&1
        if !errorlevel! equ 0 (
            call :log_ok "libimobiledevice tools working properly"
        ) else (
            call :log_warning "libimobiledevice tools may need additional dependencies"
        )
    ) else (
        call :log_error "idevice_id.exe not found"
        call :log_info_indent "Installation may be incomplete"
    )
    
    REM Check library files
    if exist "%LIBIMOBILEDEVICE_LIB%\libimobiledevice-1.0.dll" (
        call :log_ok "libimobiledevice-1.0.dll found"
    ) else (
        call :log_warning "libimobiledevice-1.0.dll not found"
    )
    
    REM Check header files
    if exist "%LIBIMOBILEDEVICE_INCLUDE%\libimobiledevice\libimobiledevice.h" (
        call :log_ok "libimobiledevice header files found"
    ) else (
        call :log_warning "libimobiledevice header files not found"
    )
    
) else (
    call :log_error "libimobiledevice not installed"
    call :log_info_indent "Expected installation path: %LIBIMOBILEDEVICE_ROOT%"
    call :log_info_indent "Run the following command to install:"
    call :log_info_indent "  build.bat install-deps"
    call :log_info_indent "Or use Python script directly:"
    call :log_info_indent "  python scripts/install-deps.py"
)

REM Check PATH environment variable
where idevice_id.exe >nul 2>&1
if !errorlevel! equ 0 (
    call :log_info "idevice_id.exe found in PATH"
    for /f "tokens=*" %%i in ('where idevice_id.exe') do call :log_info_indent "Location: %%i"
) else (
    if not exist "%LIBIMOBILEDEVICE_ROOT%" (
        call :log_warning "idevice_id.exe not in system PATH"
    )
)

goto :eof

REM Common deployment function
REM Parameters: %1=build config (Release/Debug), %2=executable path
:deploy_qt_dependencies
call :init_constants
set "DEPLOY_CONFIG=%~1"
set "EXE_PATH=%~2"
set "DEPLOY_DIR=%DEPLOY_CONFIG%\deploy"

call :log_newline
call :log_message "Executable found at: %EXE_PATH%"
call :log_newline
call :log_message "Deploying Qt dependencies for %DEPLOY_CONFIG% build..."

REM Create deployment directory
if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"

REM Copy executable to deployment directory
copy "%EXE_PATH%" "%DEPLOY_DIR%\" >nul

REM Use windeployqt to deploy Qt dependencies
if exist "%QT_WINDEPLOYQT%" (
    call :log_message "Running windeployqt [minimal deployment]..."
    echo [INFO] Executing windeployqt:
    if /i "%DEPLOY_CONFIG%"=="Release" (
        echo [INFO]   "%QT_WINDEPLOYQT%" --release --no-translations "%DEPLOY_DIR%\phone-linkc.exe"
        "%QT_WINDEPLOYQT%" --release --no-translations "%DEPLOY_DIR%\phone-linkc.exe"
    ) else (
        echo [INFO]   "%QT_WINDEPLOYQT%" --debug --no-translations "%DEPLOY_DIR%\phone-linkc.exe"
        "%QT_WINDEPLOYQT%" --debug --no-translations "%DEPLOY_DIR%\phone-linkc.exe"
    )
    if !errorlevel!==0 (
        call :log_success "Qt dependencies deployed successfully [minimal package]!"
        call :log_info "Standalone executable available at: %DEPLOY_DIR%\phone-linkc.exe"
        call :log_info "Minimal deployment: translations and system D3D excluded"
    ) else (
        call :log_warning "windeployqt failed for %DEPLOY_CONFIG% build"
    )
) else (
    call :log_warning "windeployqt.exe not found, executable may need Qt DLLs in PATH"
)

REM Deploy libimobiledevice dependencies
call :log_newline
call :log_message "Deploying libimobiledevice dependencies..."
if exist "%LIBIMOBILEDEVICE_ROOT%" (
    REM Create libimobiledevice subdirectory
    if not exist "%DEPLOY_DIR%\libimobiledevice" mkdir "%DEPLOY_DIR%\libimobiledevice"
    
    REM Copy all DLL files
    if exist "%LIBIMOBILEDEVICE_ROOT%\*.dll" (
        copy "%LIBIMOBILEDEVICE_ROOT%\*.dll" "%DEPLOY_DIR%\libimobiledevice\" >nul 2>&1
        call :log_ok "libimobiledevice DLL files copied"
    )
    
    REM Copy all executable files
    if exist "%LIBIMOBILEDEVICE_ROOT%\*.exe" (
        copy "%LIBIMOBILEDEVICE_ROOT%\*.exe" "%DEPLOY_DIR%\libimobiledevice\" >nul 2>&1
        call :log_ok "libimobiledevice executable files copied"
    )
    
    REM Copy include directory for dynamic loading
    if exist "%LIBIMOBILEDEVICE_INCLUDE%" (
        xcopy "%LIBIMOBILEDEVICE_INCLUDE%" "%DEPLOY_DIR%\libimobiledevice\include\" /E /I /Q >nul 2>&1
        call :log_ok "libimobiledevice header files copied"
    )
    
    REM Copy documentation if exists
    if exist "%LIBIMOBILEDEVICE_ROOT%\doc" (
        xcopy "%LIBIMOBILEDEVICE_ROOT%\doc" "%DEPLOY_DIR%\libimobiledevice\doc\" /E /I /Q >nul 2>&1
        call :log_ok "libimobiledevice documentation copied"
    )
    
    call :log_success "libimobiledevice dependencies deployed successfully!"
    call :log_info "libimobiledevice path: %DEPLOY_DIR%\libimobiledevice\"
    call :log_info "Core libraries and tools included for iOS device support"
) else (
    call :log_warning "libimobiledevice not found at: %LIBIMOBILEDEVICE_ROOT%"
    call :log_info "Application will use fallback/simulation mode"
)

call :log_newline
set /p run="Run application? (y/n): "
if /i "!run!"=="y" (
    call :log_message "Starting application..."
    if exist "%DEPLOY_DIR%\phone-linkc.exe" (
        "%DEPLOY_DIR%\phone-linkc.exe"
    ) else (
        "%EXE_PATH%"
    )
)
goto :eof

:init_build
REM Detect MSVC toolchain
call :log_message "Detecting MSVC toolchain..."

REM Try to find and configure MSVC
call :detect_msvc
if defined COMPILER_TYPE (
    call :log_info "MSVC toolchain detected and configured"
    goto configure_qt
) else (
    call :log_error "MSVC toolchain not found"
    call :log_error "Please install Visual Studio 2019/2022 with C++ support"
    goto end
)

:configure_qt
REM Configure Qt MSVC environment (preserve BUILD_CONFIG and CMAKE_BUILD_TYPE)
set "SAVED_BUILD_CONFIG=%BUILD_CONFIG%"
set "SAVED_CMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE%"
call :init_constants
set "BUILD_CONFIG=%SAVED_BUILD_CONFIG%"
set "CMAKE_BUILD_TYPE=%SAVED_CMAKE_BUILD_TYPE%"
REM Check for MSVC-compiled Qt versions
if exist "%QT_BASE%\lib\cmake\Qt6\Qt6Config.cmake" (
    set "PATH=%QT_BASE%\bin;!PATH!"
    set "Qt6_DIR=%QT_BASE%\lib\cmake\Qt6"
    call :log_info "Qt 6.10.1 MSVC2022 64-bit configured"
    goto configure_cmake
)

call :log_newline
call :log_error "No MSVC-compiled Qt installation found"
call :log_error "Please install Qt 6.x with MSVC2022 64-bit support"
call :log_info "Download from: https://www.qt.io/download"
goto end

:configure_cmake
REM Configure Qt CMake environment (prioritize Qt's CMake)
if exist "%QT_CMAKE_TOOLS%" (
    set "PATH=D:\Qt\Tools\CMake_64\bin;!PATH!"
    call :log_info "Qt CMake configured"
)
goto start_build

:check_deps
call :log_message "Checking dependencies..."
call :log_newline

REM Check Qt CMake configuration
if defined Qt6_DIR (
    if exist "%Qt6_DIR%\Qt6Config.cmake" (
        call :log_ok "Qt CMake configuration found"
        call :log_info_indent "Qt6_DIR: %Qt6_DIR%"
    ) else (
        call :log_error "Qt6Config.cmake not found at %Qt6_DIR%"
    )
) else (
    call :log_error "Qt6_DIR not configured"
)

call :log_newline

REM Check CMake  
where cmake >nul 2>&1
if !errorlevel!==0 (
    call :log_ok "CMake found"
    for /f "tokens=*" %%i in ('cmake --version ^| findstr "cmake version"') do call :log_info_indent "%%i"
) else (
    call :log_error "CMake not found - Please install from https://cmake.org/download/"
)

call :log_newline

REM Check MSVC compiler
where cl >nul 2>&1
if !errorlevel!==0 (
    call :log_ok "MSVC compiler found"
    for /f "tokens=*" %%i in ('cl 2^>^&1 ^| findstr "Microsoft"') do call :log_info_indent "%%i"
) else (
    call :log_error "MSVC compiler not accessible"
    call :log_info_indent "Please ensure Visual Studio is properly installed and configured"
)

call :log_newline

REM Check libimobiledevice
call :check_libimobiledevice

call :log_newline
goto end

:install_deps
call :log_message "启动完整依赖安装..."

REM 检查安装脚本
set "INSTALL_SCRIPT=%~dp0scripts\install-deps.bat"
if exist "%INSTALL_SCRIPT%" (
    call :log_info "使用安装脚本: %INSTALL_SCRIPT%"
    call "%INSTALL_SCRIPT%"
    if !errorlevel! equ 0 (
        call :log_success "依赖安装完成"
        call :log_info "请重新打开命令提示符以使环境变量生效，然后运行构建"
    ) else (
        call :log_error "依赖安装失败"
    )
) else (
    call :log_error "安装脚本未找到: %INSTALL_SCRIPT%"
    call :log_info "请确保 scripts 目录包含安装脚本"
    call :log_info "或直接运行: python scripts/install-deps.py"
)
goto end

:show_help
echo Usage: build.bat [build_type] [option]
echo.
echo Build Types:
echo   debug                   - Build in Debug mode (with debug symbols)
echo   release                 - Build in Release mode (optimized, default)
echo.
echo Options:
echo   check-deps              - Check dependencies
echo   install-deps            - Install all dependencies
echo   help                    - Show this help
echo   (no args)               - Run full build (Release mode)
echo.
echo Examples:
echo   build.bat                           - Run full Release build
echo   build.bat debug                     - Run full Debug build
echo   build.bat release                   - Run full Release build
echo   build.bat check-deps               - Check all dependencies
echo   build.bat install-deps             - Install all dependencies
echo   build.bat debug check-deps         - Check deps (Debug mode set)
echo   build.bat release install-deps     - Install deps (Release mode set)
echo.
echo Build Mode Features:
echo   Debug:   Includes debug symbols, no optimization, larger executable
echo   Release: Optimized build, smaller executable, better performance
echo.
echo Recommended workflow:
echo   1. build.bat install-deps          - Install dependencies
echo   2. Restart command prompt          - Apply environment variables  
echo   3. build.bat check-deps           - Verify installation
echo   4. build.bat debug                - Build for development
echo   5. build.bat release              - Build for production
echo.
goto end

:full_build
goto init_build

:start_build
call :log_message "Starting full build process..."
call :log_info "Build configuration: %BUILD_CONFIG% mode"
call :log_newline
goto check_deps_inline

:check_deps_inline
call :log_message "Checking dependencies..."
call :log_newline

REM Check Qt CMake configuration
if defined Qt6_DIR (
    if exist "%Qt6_DIR%\Qt6Config.cmake" (
        call :log_ok "Qt CMake configuration found"
    ) else (
        call :log_error "Qt6Config.cmake not found - Cannot proceed"
        goto end
    )
) else (
    call :log_error "Qt6_DIR not configured - Cannot proceed"
    goto end
)

REM Check CMake
where cmake >nul 2>&1
if !errorlevel!==0 (
    call :log_ok "CMake found"
) else (
    call :log_error "CMake not found - Cannot proceed"
    goto end
)

REM Check MSVC compiler availability
where cl >nul 2>&1
if !errorlevel!==0 (
    call :log_ok "MSVC compiler found"
) else (
    call :log_error "MSVC compiler not accessible - Cannot proceed"
    call :log_info_indent "Please ensure Visual Studio is properly installed and configured"
    goto end
)

call :log_newline
call :log_message "All dependencies satisfied!"
call :log_newline
call :log_message "Proceeding with build..."
call :log_newline
call :log_message "Building project..."
call :log_newline

if exist build (
    call :log_message "Cleaning previous build..."
    rmdir /s /q build
)
mkdir build
cd build

call :log_message "Running CMake configuration with !CMAKE_GENERATOR! generator..."
call :log_info "Using compiler: !COMPILER_TYPE!"

REM Use common function to execute CMake configuration
call :run_cmake_configure
if !errorlevel! neq 0 (
    call :log_error "CMake configuration failed"
    cd ..
    goto end
)

call :log_newline
call :log_message "Building project with !COMPILER_TYPE! toolchain..."

REM Use common function to execute build
call :run_cmake_build
if !errorlevel! neq 0 (
    call :log_error "Build failed"
    cd ..
    goto end
)

call :log_newline
call :log_success "Build completed!"

REM Check for executable and deploy Qt dependencies
if exist "Release\phone-linkc.exe" (
    call :deploy_qt_dependencies "Release" "Release\phone-linkc.exe"
) else if exist "Debug\phone-linkc.exe" (
    call :deploy_qt_dependencies "Debug" "Debug\phone-linkc.exe"
) else if exist "phone-linkc.exe" (
    call :log_newline
    call :log_message "Executable found at: phone-linkc.exe"
    set /p run="Run application? (y/n): "
    if /i "!run!"=="y" (
        call :log_message "Starting application..."
        phone-linkc.exe
    )
) else (
    call :log_message "Note: Executable not found in expected locations"
    call :log_message "Checked: Release\phone-linkc.exe, Debug\phone-linkc.exe, phone-linkc.exe"
    call :log_message "Build completed successfully but executable location may be different"
)

cd ..

:end
echo.
echo Press any key to exit...
pause >nul
