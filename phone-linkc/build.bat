@echo off
REM iOS 设备管理器构建脚本 (Windows)
REM 需要 Qt 6.5+ 和 Visual Studio 或 MinGW

echo =========================================
echo iOS 设备管理器 - 构建脚本 (Windows)
echo =========================================

REM 检查 CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到 CMake，请安装 CMake 3.19+
    echo 下载地址: https://cmake.org/download/
    pause
    exit /b 1
)
echo [成功] 找到 CMake

REM 检查 Qt
where qmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到 Qt，请安装 Qt 6.5+
    echo 下载地址: https://www.qt.io/download-qt-installer
    echo 或确保 Qt 的 bin 目录已添加到 PATH 环境变量
    pause
    exit /b 1
)
echo [成功] 找到 Qt

REM 检查 Visual Studio 构建工具
where cl >nul 2>&1
if %errorlevel% neq 0 (
    where g++ >nul 2>&1
    if %errorlevel% neq 0 (
        echo [警告] 未找到编译器 (MSVC 或 MinGW)
        echo 请安装 Visual Studio Build Tools 或 MinGW
        echo Visual Studio: https://visualstudio.microsoft.com/visual-cpp-build-tools/
        echo MinGW: https://www.mingw-w64.org/
        pause
        exit /b 1
    ) else (
        echo [成功] 找到 MinGW 编译器
        set COMPILER=MinGW
    )
) else (
    echo [成功] 找到 MSVC 编译器
    set COMPILER=MSVC
)

REM libimobiledevice 检查 (Windows 上较复杂，通常使用 vcpkg)
echo [提示] Windows 上的 libimobiledevice 支持需要手动配置
echo 建议使用 vcpkg 安装: vcpkg install libimobiledevice
echo 或使用模拟模式进行开发和测试

echo.
echo 准备构建...

REM 清理旧的构建文件
if exist build (
    echo 清理旧的构建文件...
    rmdir /s /q build
)

REM 创建构建目录
echo 创建构建目录...
mkdir build
cd build

REM 配置项目
echo 配置项目...
if "%COMPILER%"=="MSVC" (
    cmake .. -DCMAKE_BUILD_TYPE=Release
) else (
    cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
)

if %errorlevel% neq 0 (
    echo [错误] CMake 配置失败
    pause
    exit /b 1
)

REM 编译项目
echo 编译项目...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

echo.
echo =========================================
echo 构建完成！
echo =========================================
echo.
echo [提示] 运行在模拟模式（除非已安装 libimobiledevice）
echo 将显示模拟设备，用于开发和测试
echo.
echo 运行应用程序:
if "%COMPILER%"=="MSVC" (
    echo   Release\phone-linkc.exe
) else (
    echo   phone-linkc.exe
)
echo.

set /p choice=是否立即运行？(y/n): 
if /i "%choice%"=="y" (
    echo 启动 iOS 设备管理器...
    if "%COMPILER%"=="MSVC" (
        Release\phone-linkc.exe
    ) else (
        phone-linkc.exe
    )
)

pause
