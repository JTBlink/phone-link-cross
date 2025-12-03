@echo off
REM iOS 设备管理器 - 依赖安装脚本 (Windows 入口)
REM 调用 Python 脚本执行实际安装

REM 设置控制台编码为 UTF-8
chcp 65001 >nul 2>&1

echo =========================================
echo iOS 设备管理器 - 依赖安装器
echo =========================================
echo.

REM 检查管理员权限
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 此脚本需要管理员权限
    echo 请右键点击此文件，选择"以管理员身份运行"
    echo.
    echo 或者在管理员命令提示符中运行:
    echo   cd /d "%~dp0"
    echo   install-deps.bat
    pause
    exit /b 1
)

echo [信息] 管理员权限确认
echo.

REM 检查 Python
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到 Python
    echo 请安装 Python 3.7+ 并确保已添加到 PATH
    echo 下载地址: https://www.python.org/downloads/
    echo.
    echo 安装后请重新运行此脚本
    pause
    exit /b 1
)

REM 获取 Python 版本
for /f "tokens=2" %%i in ('python --version 2^>^&1') do set PYTHON_VERSION=%%i
echo [信息] 找到 Python %PYTHON_VERSION%

REM 切换到脚本目录
cd /d "%~dp0"

REM 检查 install-deps.py 是否存在
if not exist "install-deps.py" (
    echo [错误] 未找到 install-deps.py
    echo 请确保脚本文件完整
    pause
    exit /b 1
)

echo [信息] 准备调用 Python 安装脚本...
echo.

REM 解析命令行参数
set PYTHON_ARGS=
set SHOW_HELP=false

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="--help" set SHOW_HELP=true
if /i "%~1"=="-h" set SHOW_HELP=true
if /i "%~1"=="help" set SHOW_HELP=true
if /i "%~1"=="--skip-itunes" set PYTHON_ARGS=%PYTHON_ARGS% --skip-itunes
if /i "%~1"=="--no-tests" set PYTHON_ARGS=%PYTHON_ARGS% --no-tests
if /i "%~1"=="--temp-dir" (
    shift
    set PYTHON_ARGS=%PYTHON_ARGS% --temp-dir "%~1"
)
shift
goto parse_args

:args_done

REM 显示帮助
if "%SHOW_HELP%"=="true" (
    echo 用法: install-deps.bat [选项]
    echo.
    echo 选项:
    echo   --skip-itunes    跳过 iTunes 安装
    echo   --no-tests       跳过安装后测试
    echo   --temp-dir DIR   指定临时目录
    echo   --help, -h       显示此帮助信息
    echo.
    echo 示例:
    echo   install-deps.bat                    # 完整安装
    echo   install-deps.bat --skip-itunes     # 跳过 iTunes
    echo   install-deps.bat --no-tests        # 跳过测试
    echo.
    pause
    exit /b 0
)

REM 调用 Python 脚本
echo [信息] 调用: python install-deps.py%PYTHON_ARGS%
echo.

python install-deps.py%PYTHON_ARGS%
set PYTHON_EXIT_CODE=%errorlevel%

echo.
echo =========================================

if %PYTHON_EXIT_CODE% equ 0 (
    echo [成功] 依赖安装完成
    echo.
    echo 重要提醒:
    echo 1. 重新打开命令提示符使环境变量生效
    echo 2. 运行测试: python scripts\check-deps.py
    echo 3. 构建项目: build.bat
) else (
    echo [失败] 依赖安装失败，返回码: %PYTHON_EXIT_CODE%
    echo.
    echo 故障排除:
    echo 1. 确保以管理员身份运行
    echo 2. 检查网络连接
    echo 3. 查看上方错误信息
    echo 4. 运行检查脚本: python scripts\check-deps.py
)

echo.
echo 按任意键退出...
pause >nul
exit /b %PYTHON_EXIT_CODE%
