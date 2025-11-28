@echo off
setlocal enabledelayedexpansion

:: Qt Windows 自动安装脚本
:: 使用阿里云镜像加速下载

echo ========================================
echo Qt Windows 自动安装脚本
echo 使用阿里云镜像源
echo ========================================
echo.

:: 检查管理员权限
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [INFO] 检测到管理员权限，继续安装...
) else (
    echo [WARNING] 建议以管理员权限运行此脚本
    echo [WARNING] 某些功能可能需要管理员权限
    timeout /t 3 >nul
)

:: 设置变量
set MIRROR_URL=https://mirrors.aliyun.com/qt
set INSTALLER_URL=https://mirrors.aliyun.com/qt/official_releases/online_installers
set TEMP_DIR=%TEMP%\qt_installer
set INSTALLER_NAME=qt-unified-windows-x64-online.exe

:: 检测系统架构
if "%PROCESSOR_ARCHITECTURE%"=="x86" (
    if not defined PROCESSOR_ARCHITEW6432 (
        set INSTALLER_NAME=qt-unified-windows-x86-online.exe
        echo [INFO] 检测到 32位 Windows 系统
    ) else (
        echo [INFO] 检测到 64位 Windows 系统
    )
) else (
    echo [INFO] 检测到 64位 Windows 系统
)

:: 检查磁盘空间（至少需要5GB）
echo [INFO] 检查磁盘空间...
for /f "tokens=3" %%a in ('dir /-c "%SystemDrive%\" ^| find "bytes free"') do set AVAILABLE_BYTES=%%a
set /a AVAILABLE_GB=%AVAILABLE_BYTES% / 1073741824
set /a AVAILABLE_MB=%AVAILABLE_BYTES% / 1048576

if %AVAILABLE_GB% lss 5 (
    echo [WARNING] 可用磁盘空间不足 5GB，当前可用: %AVAILABLE_GB%GB ^(%AVAILABLE_MB%MB^)
    set /p CONTINUE_ANYWAY="是否继续安装? (y/N): "
    if /i not "!CONTINUE_ANYWAY!"=="Y" (
        echo [INFO] 安装已取消
        pause
        exit /b 0
    )
) else (
    echo [INFO] 可用磁盘空间: %AVAILABLE_GB%GB
)

:: 创建临时目录
if not exist "%TEMP_DIR%" (
    mkdir "%TEMP_DIR%"
    echo [INFO] 创建临时目录: %TEMP_DIR%
)

:: 检查是否已有安装器
if exist "%TEMP_DIR%\%INSTALLER_NAME%" (
    echo [INFO] 发现已存在的安装器
    set /p USE_EXISTING="是否使用已存在的安装器? (Y/N): "
    if /i "!USE_EXISTING!"=="N" (
        goto DOWNLOAD_INSTALLER
    ) else (
        goto RUN_INSTALLER
    )
)

:DOWNLOAD_INSTALLER
echo [INFO] 正在下载 Qt 在线安装器...
echo [INFO] 下载地址: %INSTALLER_URL%/%INSTALLER_NAME%

:: 使用PowerShell下载（支持进度显示）
powershell -Command "& {
    $ProgressPreference = 'Continue'
    try {
        Write-Host '[INFO] 开始下载 Qt 安装器...'
        Invoke-WebRequest -Uri '%INSTALLER_URL%/%INSTALLER_NAME%' -OutFile '%TEMP_DIR%\%INSTALLER_NAME%' -UseBasicParsing
        Write-Host '[SUCCESS] 下载完成!'
    } catch {
        Write-Host '[ERROR] 下载失败:' $_.Exception.Message
        exit 1
    }
}"

if %errorLevel% neq 0 (
    echo [ERROR] 下载失败，请检查网络连接
    pause
    exit /b 1
)

:RUN_INSTALLER
echo [INFO] 正在启动 Qt 安装器...
echo [INFO] 使用镜像源: %MIRROR_URL%

:: 显示安装提示
echo.
echo ========================================
echo 安装提示:
echo 1. 安装器将使用阿里云镜像加速下载
echo 2. 请在安装器中登录您的 Qt 账户
echo 3. 推荐安装组件:
echo    - Qt 6.x (最新版本)
echo    - Qt Creator
echo    - CMake
echo    - MSVC 2019/2022 编译器
echo 4. 安装路径建议: C:\Qt
echo ========================================
echo.

:: 询问是否继续
set /p CONTINUE="按回车键继续安装，或输入 N 取消: "
if /i "%CONTINUE%"=="N" (
    echo [INFO] 安装已取消
    pause
    exit /b 0
)

:: 启动安装器并配置镜像
echo [INFO] 启动安装器中...
start /wait "%TEMP_DIR%\%INSTALLER_NAME%" --mirror %MIRROR_URL%

if %errorLevel% neq 0 (
    echo [WARNING] 安装器可能遇到问题
    echo [INFO] 尝试不带镜像参数启动...
    start /wait "%TEMP_DIR%\%INSTALLER_NAME%"
)

:: 安装后配置
echo.
echo [INFO] 安装器已关闭
set /p SETUP_ENV="是否需要配置环境变量? (Y/N): "
if /i "%SETUP_ENV%"=="Y" (
    call :SETUP_ENVIRONMENT
)

:: 清理临时文件
set /p CLEANUP="是否删除临时安装文件? (Y/N): "
if /i "%CLEANUP%"=="Y" (
    echo [INFO] 清理临时文件...
    del /q "%TEMP_DIR%\%INSTALLER_NAME%" 2>nul
    rmdir "%TEMP_DIR%" 2>nul
)

echo.
echo ========================================
echo Qt 安装完成!
echo.
echo 后续步骤:
echo 1. 重启命令提示符以使环境变量生效
echo 2. 运行 'qmake --version' 验证安装
echo 3. 启动 Qt Creator 开始开发
echo ========================================
pause
exit /b 0

:SETUP_ENVIRONMENT
echo [INFO] 配置环境变量...

:: 查找Qt安装路径
set QT_PATH=
for /d %%i in (C:\Qt\*) do (
    if exist "%%i\bin\qmake.exe" (
        set QT_PATH=%%i\bin
        goto :found_qt
    )
)

:: 如果在C:\Qt没找到，尝试其他位置
for /d %%i in (D:\Qt\*) do (
    if exist "%%i\bin\qmake.exe" (
        set QT_PATH=%%i\bin
        goto :found_qt
    )
)

:found_qt
if defined QT_PATH (
    echo [INFO] 找到 Qt 安装路径: %QT_PATH%
    
    :: 添加到系统PATH
    setx PATH "%PATH%;%QT_PATH%" /M >nul 2>&1
    if %errorLevel% == 0 (
        echo [SUCCESS] 环境变量配置成功
    ) else (
        echo [WARNING] 需要管理员权限配置系统环境变量
        echo [INFO] 请手动将以下路径添加到 PATH:
        echo [INFO] %QT_PATH%
    )
) else (
    echo [WARNING] 未找到 Qt 安装路径
    echo [INFO] 请手动配置环境变量
)
goto :eof

:ERROR
echo [ERROR] %1
pause
exit /b 1
