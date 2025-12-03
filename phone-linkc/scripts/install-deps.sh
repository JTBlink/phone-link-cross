#!/bin/bash
# iOS 设备管理器 - 依赖安装脚本 (Unix/Linux 入口)
# 调用 Python 脚本执行实际安装

set -e

echo "========================================="
echo "iOS 设备管理器 - 依赖安装器"
echo "========================================="
echo

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 检查操作系统
check_os() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macOS"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="Linux"
    else
        echo -e "${RED}[错误]${NC} 不支持的操作系统: $OSTYPE"
        exit 1
    fi
    
    echo -e "${GREEN}[信息]${NC} 检测到操作系统: $OS"
}

# 检查 Python
check_python() {
    if command -v python3 >/dev/null 2>&1; then
        PYTHON_CMD="python3"
        PYTHON_VERSION=$(python3 --version 2>&1 | cut -d' ' -f2)
    elif command -v python >/dev/null 2>&1; then
        PYTHON_CMD="python"
        PYTHON_VERSION=$(python --version 2>&1 | cut -d' ' -f2)
        
        # 检查是否是 Python 3
        if [[ $PYTHON_VERSION == 2.* ]]; then
            echo -e "${RED}[错误]${NC} 需要 Python 3.7+，当前版本: $PYTHON_VERSION"
            echo "请安装 Python 3:"
            if [[ "$OS" == "macOS" ]]; then
                echo "  brew install python3"
            elif [[ "$OS" == "Linux" ]]; then
                echo "  sudo apt-get install python3 python3-pip"
            fi
            exit 1
        fi
    else
        echo -e "${RED}[错误]${NC} 未找到 Python"
        echo "请安装 Python 3.7+:"
        if [[ "$OS" == "macOS" ]]; then
            echo "  brew install python3"
        elif [[ "$OS" == "Linux" ]]; then
            echo "  sudo apt-get install python3 python3-pip"
        fi
        exit 1
    fi
    
    echo -e "${GREEN}[信息]${NC} 找到 Python: $PYTHON_CMD $PYTHON_VERSION"
}

# 检查权限
check_permissions() {
    if [[ "$OS" == "Linux" ]]; then
        # Linux 上检查 sudo 权限
        if ! sudo -n true 2>/dev/null; then
            echo -e "${YELLOW}[警告]${NC} 可能需要 sudo 权限来安装包"
            echo "如果出现权限错误，请使用 sudo 运行此脚本"
        else
            echo -e "${GREEN}[信息]${NC} sudo 权限可用"
        fi
    elif [[ "$OS" == "macOS" ]]; then
        # macOS 检查 Homebrew 写入权限
        if command -v brew >/dev/null 2>&1; then
            BREW_PREFIX=$(brew --prefix)
            if [[ -w "$BREW_PREFIX" ]]; then
                echo -e "${GREEN}[信息]${NC} Homebrew 写入权限可用"
            else
                echo -e "${YELLOW}[警告]${NC} Homebrew 可能需要管理员权限"
            fi
        fi
    fi
}

# 显示帮助
show_help() {
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  --skip-itunes    跳过 iTunes 安装 (仅 macOS)"
    echo "  --no-tests       跳过安装后测试"
    echo "  --temp-dir DIR   指定临时目录"
    echo "  --help, -h       显示此帮助信息"
    echo
    echo "示例:"
    echo "  $0                    # 完整安装"
    echo "  $0 --skip-itunes     # 跳过 iTunes (macOS)"
    echo "  $0 --no-tests        # 跳过测试"
    echo
}

# 解析命令行参数
PYTHON_ARGS=""
SHOW_HELP=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --help|-h|help)
            SHOW_HELP=true
            shift
            ;;
        --skip-itunes)
            PYTHON_ARGS="$PYTHON_ARGS --skip-itunes"
            shift
            ;;
        --no-tests)
            PYTHON_ARGS="$PYTHON_ARGS --no-tests"
            shift
            ;;
        --temp-dir)
            PYTHON_ARGS="$PYTHON_ARGS --temp-dir \"$2\""
            shift 2
            ;;
        *)
            echo -e "${RED}[错误]${NC} 未知参数: $1"
            show_help
            exit 1
            ;;
    esac
done

# 显示帮助并退出
if [[ "$SHOW_HELP" == true ]]; then
    show_help
    exit 0
fi

# 切换到脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 检查 install-deps.py 是否存在
if [[ ! -f "install-deps.py" ]]; then
    echo -e "${RED}[错误]${NC} 未找到 install-deps.py"
    echo "请确保脚本文件完整"
    exit 1
fi

# 主函数
main() {
    echo
    echo "开始依赖检查和安装..."
    echo
    
    # 系统检查
    check_os
    check_python
    check_permissions
    
    echo
    echo -e "${BLUE}[信息]${NC} 准备调用 Python 安装脚本..."
    echo -e "${BLUE}[信息]${NC} 命令: $PYTHON_CMD install-deps.py$PYTHON_ARGS"
    echo
    
    # 调用 Python 脚本
    if eval "$PYTHON_CMD install-deps.py$PYTHON_ARGS"; then
        PYTHON_EXIT_CODE=0
    else
        PYTHON_EXIT_CODE=$?
    fi
    
    echo
    echo "========================================="
    
    if [[ $PYTHON_EXIT_CODE -eq 0 ]]; then
        echo -e "${GREEN}[成功]${NC} 依赖安装完成"
        echo
        echo "接下来:"
        echo "1. 连接 iOS 设备并信任此电脑"
        echo "2. 运行测试: ./scripts/check-deps.sh"
        echo "3. 构建项目: ./build.sh"
        
        if [[ "$OS" == "macOS" ]]; then
            echo
            echo "macOS 特别说明:"
            echo "- 如果使用了 Homebrew，可能需要重新加载 shell 配置"
            echo "- 运行: source ~/.zshrc 或 source ~/.bash_profile"
        fi
    else
        echo -e "${RED}[失败]${NC} 依赖安装失败，返回码: $PYTHON_EXIT_CODE"
        echo
        echo "故障排除:"
        echo "1. 检查网络连接"
        echo "2. 确保有足够的权限"
        echo "3. 查看上方错误信息"
        echo "4. 运行检查脚本: ./scripts/check-deps.sh"
        
        if [[ "$OS" == "Linux" ]]; then
            echo "5. 尝试使用 sudo 运行: sudo $0"
        elif [[ "$OS" == "macOS" ]]; then
            echo "5. 确保 Homebrew 已正确安装"
            echo "6. 尝试手动安装: brew install libimobiledevice"
        fi
    fi
    
    echo
    return $PYTHON_EXIT_CODE
}

# 处理信号
trap 'echo -e "\n${YELLOW}[警告]${NC} 安装被中断"; exit 130' INT TERM

# 运行主函数
main "$@"
exit $?
