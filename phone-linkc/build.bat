@echo off
REM iOS 设备管理器 - Windows 主构建脚本
REM 统一入口，调用 scripts 目录下的各种工具

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SCRIPTS_DIR=%SCRIPT_DIR%scripts"

:show_help
if "%1"=="help" goto :help
if "%1"=="-h" goto :help
if "%1"=="--help" goto :help
if "%1"=="/?" goto :help
goto :check_args

:help
echo =========================================
echo iOS 设备管理器 - 构建工具 (Windows)
echo =========================================
echo.
echo 用法: build.bat [选项]
echo.
echo 选项:
echo   (无参数)          - 运行完整构建流程
echo   check-deps        - 检查和诊断依赖
echo   build             - 仅构建项目
echo   clean             - 清理构建文件
echo   install-deps      - 显示依赖安装命令
echo   help, -h, --help  - 显示此帮助信息
echo.
echo 构建流程:
echo   1. 检查依赖 (Qt, CMake, Visual Studio)
echo   2. 配置项目 (CMake)
echo   3. 编译项目
echo   4. 运行应用程序
echo.
echo 脚本文件位置: %SCRIPTS_DIR%\
echo   - build.sh       - 主构建脚本 (Linux/macOS)
echo   - build.bat      - Windows 构建脚本
echo   - check-deps.sh  - 依赖诊断脚本 (Linux/macOS)
echo.
echo 注意: Windows 版本主要使用模拟模式 (不支持真实 iOS 设备)
goto :end

:check_args
REM 检查 scripts 目录是否存在
if not exist "%SCRIPTS_DIR%" (
    echo 错误: 找不到 scripts 目录: %SCRIPTS_DIR%
    exit /b 1
)

if "%1"=="check-deps" goto :run_check_deps
if "%1"=="build" goto :run_build
if "%1"=="clean" goto :run_clean
if "%1"=="install-deps" goto :run_install_deps
if "%1"=="" goto :run_full_build

echo 未知选项: %1
echo.
goto :help

:run_check_deps
echo 正在检查依赖...
echo.
echo Windows 依赖检查:
echo.
REM 检查 Qt
where /q qmake
if %errorlevel%==0 (
    echo [√] Qt 已找到
    for /f "tokens=*" %%i in ('qmake -version ^| findstr "Qt version"') do echo     %%i
) else (
    echo [×] Qt 未找到
    echo     请安装 Qt 6.6+ 或将 qmake 添加到 PATH
)

REM 检查 CMake
where /q cmake
if %errorlevel%==0 (
    echo [√] CMake 已找到
    for /f "tokens=*" %%i in ('cmake --version ^| findstr "cmake version"') do echo     %%i
) else (
    echo [×] CMake 未找到
    echo     请安装 CMake 3.19+ 或将 cmake 添加到 PATH
)

REM 检查 Visual Studio 编译器
where /q cl
if %errorlevel%==0 (
    echo [√] Visual Studio 编译器已找到
) else (
    echo [×] Visual Studio 编译器未找到
    echo     请安装 Visual Studio 2019+ 或运行 "Developer Command Prompt"
)

echo.
echo Windows 版本说明:
echo   - libimobiledevice 在 Windows 上安装复杂，建议使用模拟模式
echo   - 模拟模式将显示虚拟 iOS 设备，用于界面演示和功能测试
echo   - 真实 iOS 设备支持请在 macOS 或 Linux 上使用
goto :end

:run_build
echo 正在构建项目...
if exist "%SCRIPTS_DIR%\build-impl.bat" (
    call "%SCRIPTS_DIR%\build-impl.bat" build
) else (
    echo 错误: 找不到构建脚本
    exit /b 1
)
goto :end

:run_clean
echo 正在清理构建文件...
if exist "%SCRIPTS_DIR%\build-impl.bat" (
    call "%SCRIPTS_DIR%\build-impl.bat" clean
) else (
    echo 错误: 找不到构建脚本
    exit /b 1
)
goto :end

:run_install_deps
echo Windows 依赖安装指南:
echo.
echo 1. Qt 6.6+:
echo    - 官方下载: https://www.qt.io/download-qt-installer
echo    - 选择 Qt 6.6.x for MSVC 2019/2022 64-bit
echo    - 或使用 vcpkg: vcpkg install qt6[core,widgets,network]:x64-windows
echo.
echo 2. CMake 3.19+:
echo    - 官方下载: https://cmake.org/download/
echo    - 或使用 chocolatey: choco install cmake
echo.
echo 3. Visual Studio 2019/2022:
echo    - 官方下载: https://visualstudio.microsoft.com/
echo    - 确保安装 "Desktop development with C++" 工作负载
echo    - 或安装 "Build Tools for Visual Studio"
echo.
echo 4. 可选 - libimobiledevice (复杂，建议跳过):
echo    - 使用 vcpkg: vcpkg install libimobiledevice:x64-windows
echo    - 需要额外的依赖配置
echo.
echo 推荐: 使用模拟模式进行开发和测试
goto :end

:run_full_build
echo =========================================
echo iOS 设备管理器 - 完整构建流程 (Windows)
echo =========================================
echo.

REM 步骤1: 检查依赖
echo 步骤 1/2: 检查依赖
call :run_check_deps

echo.
set /p continue="依赖检查完成，是否继续构建项目？(y/n) "
if /i not "%continue%"=="y" (
    echo 构建已取消
    goto :end
)

REM 步骤2: 构建项目
echo.
echo 步骤 2/2: 构建项目
call :run_build

echo.
echo =========================================
echo 构建流程完成！
echo =========================================
goto :end

:end
endlocal
