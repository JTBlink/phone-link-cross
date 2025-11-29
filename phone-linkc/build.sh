#!/bin/bash

# iOS 设备管理器 - 主构建脚本
# 统一入口，调用 scripts 目录下的各种工具

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS_DIR="$SCRIPT_DIR/scripts"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

show_help() {
    echo -e "${BLUE}=========================================${NC}"
    echo -e "${BLUE}iOS 设备管理器 - 构建工具${NC}"
    echo -e "${BLUE}=========================================${NC}"
    echo ""
    echo "用法: ./build.sh [选项]"
    echo ""
    echo "选项:"
    echo "  (无参数)          - 运行完整构建流程"
    echo "  check-deps        - 检查和诊断依赖"
    echo "  build             - 仅构建项目"
    echo "  clean             - 清理构建文件"
    echo "  install-deps      - 显示依赖安装命令"
    echo "  help, -h, --help  - 显示此帮助信息"
    echo ""
    echo "构建流程:"
    echo "  1. 检查依赖 (libimobiledevice, Qt, CMake)"
    echo "  2. 配置项目 (CMake)"
    echo "  3. 编译项目"
    echo "  4. 运行应用程序"
    echo ""
    echo "脚本文件位置: $SCRIPTS_DIR/"
    echo "  - build-impl.sh  - Unix 构建脚本实现"
    echo "  - build-impl.bat - Windows 构建脚本实现"
    echo "  - check-deps.sh  - 依赖诊断脚本"
}

check_scripts_dir() {
    if [ ! -d "$SCRIPTS_DIR" ]; then
        echo -e "${RED}错误: 找不到 scripts 目录: $SCRIPTS_DIR${NC}"
        exit 1
    fi
}

run_check_deps() {
    echo -e "${YELLOW}正在检查依赖...${NC}"
    if [ -f "$SCRIPTS_DIR/check-deps.sh" ]; then
        "$SCRIPTS_DIR/check-deps.sh"
    else
        echo -e "${RED}错误: 找不到依赖检查脚本${NC}"
        exit 1
    fi
}

run_build() {
    echo -e "${YELLOW}正在构建项目...${NC}"
    if [ -f "$SCRIPTS_DIR/build-impl.sh" ]; then
        "$SCRIPTS_DIR/build-impl.sh" build
    else
        echo -e "${RED}错误: 找不到构建脚本${NC}"
        exit 1
    fi
}

run_clean() {
    echo -e "${YELLOW}正在清理构建文件...${NC}"
    if [ -f "$SCRIPTS_DIR/build-impl.sh" ]; then
        "$SCRIPTS_DIR/build-impl.sh" clean
    else
        echo -e "${RED}错误: 找不到构建脚本${NC}"
        exit 1
    fi
}

run_install_deps() {
    echo -e "${YELLOW}依赖安装指南:${NC}"
    if [ -f "$SCRIPTS_DIR/build-impl.sh" ]; then
        "$SCRIPTS_DIR/build-impl.sh" install-deps
    else
        echo -e "${RED}错误: 找不到构建脚本${NC}"
        exit 1
    fi
}

run_full_build() {
    echo -e "${BLUE}=========================================${NC}"
    echo -e "${BLUE}iOS 设备管理器 - 完整构建流程${NC}"
    echo -e "${BLUE}=========================================${NC}"
    echo ""
    
    # 步骤1: 检查依赖
    echo -e "${YELLOW}步骤 1/2: 检查依赖${NC}"
    run_check_deps
    
    echo ""
    read -p "依赖检查完成，是否继续构建项目？(y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "构建已取消"
        exit 0
    fi
    
    # 步骤2: 构建项目
    echo ""
    echo -e "${YELLOW}步骤 2/2: 构建项目${NC}"
    run_build
    
    echo ""
    echo -e "${GREEN}=========================================${NC}"
    echo -e "${GREEN}构建流程完成！${NC}"
    echo -e "${GREEN}=========================================${NC}"
}

# 主逻辑
check_scripts_dir

case "${1:-}" in
    "check-deps")
        run_check_deps
        ;;
    "build")
        run_build
        ;;
    "clean")
        run_clean
        ;;
    "install-deps")
        run_install_deps
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    "")
        run_full_build
        ;;
    *)
        echo -e "${RED}未知选项: $1${NC}"
        echo ""
        show_help
        exit 1
        ;;
esac
