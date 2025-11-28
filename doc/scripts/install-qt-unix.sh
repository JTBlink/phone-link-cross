#!/bin/bash

# Qt Unix 自动安装脚本 (macOS & Linux)
# 使用阿里云镜像加速下载

set -euo pipefail

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 错误处理
error_exit() {
    log_error "$1"
    exit 1
}

# 清理函数
cleanup() {
    if [ -n "${TEMP_DIR:-}" ] && [ -d "$TEMP_DIR" ]; then
        rm -rf "$TEMP_DIR"
        log_info "清理临时文件完成"
    fi
    
    # Linux: 卸载可能的挂载点
    if [ "$OS" = "Linux" ] && [ -n "${MOUNT_POINT:-}" ] && mountpoint -q "$MOUNT_POINT" 2>/dev/null; then
        sudo umount "$MOUNT_POINT" 2>/dev/null || true
    fi
    
    # macOS: 卸载DMG
    if [ "$OS" = "Darwin" ] && [ -n "${MOUNT_POINT:-}" ]; then
        hdiutil detach "$MOUNT_POINT" &>/dev/null || true
    fi
}

# 设置陷阱
trap cleanup EXIT

# 全局变量
OS=""
DISTRO=""
DISTRO_VERSION=""
INSTALLER_NAME=""
TEMP_DIR=""
MOUNT_POINT=""
MIRROR_URL="https://mirrors.aliyun.com/qt"
INSTALLER_URL="https://mirrors.aliyun.com/qt/official_releases/online_installers"

echo "========================================"
echo "Qt Unix 自动安装脚本 (macOS & Linux)"
echo "使用阿里云镜像源"
echo "========================================"
echo

# 检测操作系统
detect_os() {
    OS=$(uname -s)
    case "$OS" in
        Darwin)
            log_info "检测到 macOS 系统"
            ;;
        Linux)
            log_info "检测到 Linux 系统"
            detect_linux_distro
            ;;
        *)
            error_exit "不支持的操作系统: $OS"
            ;;
    esac
}

# 检测Linux发行版
detect_linux_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO="$ID"
        DISTRO_VERSION="${VERSION_ID:-unknown}"
        log_info "Linux 发行版: $DISTRO $DISTRO_VERSION"
    elif [ -f /etc/redhat-release ]; then
        if grep -q "CentOS" /etc/redhat-release; then
            DISTRO="centos"
            DISTRO_VERSION=$(grep -oE '[0-9]+\.[0-9]+' /etc/redhat-release || echo "unknown")
        elif grep -q "Red Hat" /etc/redhat-release; then
            DISTRO="rhel"
            DISTRO_VERSION=$(grep -oE '[0-9]+\.[0-9]+' /etc/redhat-release || echo "unknown")
        fi
        log_info "Linux 发行版: $DISTRO $DISTRO_VERSION"
    else
        DISTRO="unknown"
        DISTRO_VERSION="unknown"
        log_warning "无法检测 Linux 发行版，将使用通用配置"
    fi
}

# 检查系统要求
check_requirements() {
    log_info "检查系统要求..."
    
    # 检查架构并设置安装器名称
    arch=$(uname -m)
    case "$OS" in
        Darwin)
            case "$arch" in
                x86_64|arm64)
                    log_info "macOS 架构: $arch"
                    INSTALLER_NAME="qt-online-installer-mac-x64-online.dmg"
                    ;;
                *)
                    error_exit "macOS 不支持的架构: $arch"
                    ;;
            esac
            
            # 检查 macOS 版本
            macos_version=$(sw_vers -productVersion)
            log_info "macOS 版本: $macos_version"
            ;;
        Linux)
            case "$arch" in
                x86_64)
                    log_info "Linux 架构: x86_64"
                    INSTALLER_NAME="qt-online-installer-linux-x64-online.run"
                    ;;
                aarch64|arm64)
                    log_info "Linux 架构: ARM64"
                    INSTALLER_NAME="qt-online-installer-linux-arm64-online.run"
                    ;;
                *)
                    error_exit "Linux 不支持的架构: $arch"
                    ;;
            esac
            ;;
    esac
    
    # 检查磁盘空间（至少需要5GB）
    if command -v df &> /dev/null; then
        # 获取可用空间（KB为单位）
        if [ "$OS" = "Darwin" ]; then
            # macOS: df输出是512字节块，第4列是可用空间
            available_kb=$(df / | awk 'NR==2 {print int($4/2)}')
        else
            # Linux: df默认输出1K块，第4列是可用空间
            available_kb=$(df / | awk 'NR==2 {print $4}')
        fi
        
        # 转换为MB
        available_mb=$((available_kb / 1024))
        # 转换为GB（用于显示）
        available_gb=$((available_mb / 1024))
        
        if [[ $available_mb -lt 5120 ]]; then
            log_warning "可用磁盘空间不足 5GB，当前可用: ${available_gb}GB (${available_mb}MB)"
            read -p "是否继续安装? (y/N): " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                error_exit "安装已取消"
            fi
        else
            log_info "可用磁盘空间: ${available_gb}GB"
        fi
    fi
    
    # 检查必要工具
    check_dependencies
}

# 检查和安装依赖
check_dependencies() {
    local missing_tools=()
    
    # 检查通用工具
    if ! command -v curl &> /dev/null && ! command -v wget &> /dev/null; then
        missing_tools+=("curl或wget")
    fi
    
    case "$OS" in
        Darwin)
            # macOS 特定检查
            if ! command -v hdiutil &> /dev/null; then
                error_exit "hdiutil 未找到，这是 macOS 系统必需工具"
            fi
            ;;
        Linux)
            # Linux 特定检查
            if ! command -v chmod &> /dev/null; then
                missing_tools+=("chmod")
            fi
            
            # 检查GUI库（可选）
            if ! ldconfig -p | grep -q libGL 2>/dev/null; then
                log_warning "未找到 OpenGL 库，可能影响 Qt 应用运行"
            fi
            ;;
    esac
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        log_warning "缺少工具: ${missing_tools[*]}"
        
        if [ "$OS" = "Linux" ]; then
            install_linux_dependencies
        else
            error_exit "请手动安装缺失的工具: ${missing_tools[*]}"
        fi
    fi
}

# 安装Linux依赖
install_linux_dependencies() {
    log_info "尝试安装Linux依赖..."
    
    case "$DISTRO" in
        ubuntu|debian)
            if command -v apt-get &> /dev/null; then
                log_info "使用 apt-get 安装依赖..."
                sudo apt-get update
                sudo apt-get install -y curl wget ca-certificates libgl1-mesa-glx libxcb-xinerama0
            fi
            ;;
        centos|rhel|fedora|rocky|almalinux)
            if command -v dnf &> /dev/null; then
                log_info "使用 dnf 安装依赖..."
                sudo dnf install -y curl wget ca-certificates mesa-libGL libxcb
            elif command -v yum &> /dev/null; then
                log_info "使用 yum 安装依赖..."
                sudo yum install -y curl wget ca-certificates mesa-libGL libxcb
            fi
            ;;
        opensuse*|sles)
            if command -v zypper &> /dev/null; then
                log_info "使用 zypper 安装依赖..."
                sudo zypper install -y curl wget ca-certificates Mesa-libGL1 libxcb1
            fi
            ;;
        arch|manjaro)
            if command -v pacman &> /dev/null; then
                log_info "使用 pacman 安装依赖..."
                sudo pacman -S --noconfirm curl wget ca-certificates mesa libxcb
            fi
            ;;
        *)
            log_warning "未识别的发行版 $DISTRO，请手动安装: curl wget ca-certificates"
            ;;
    esac
}

# 下载安装器
download_installer() {
    log_info "正在下载 Qt 在线安装器..."
    
    # 创建临时目录
    TEMP_DIR=$(mktemp -d)
    log_info "临时目录: $TEMP_DIR"
    
    DOWNLOAD_URL="${INSTALLER_URL}/${INSTALLER_NAME}"
    log_info "下载地址: $DOWNLOAD_URL"
    
    # 检查是否已有安装器
    local existing_installer="$HOME/Downloads/$INSTALLER_NAME"
    if [[ -f "$existing_installer" ]]; then
        log_info "发现已存在的安装器: $existing_installer"
        read -p "是否使用已存在的安装器? (Y/n): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Nn]$ ]]; then
            cp "$existing_installer" "$TEMP_DIR/"
            return 0
        fi
    fi
    
    # 下载安装器
    local download_cmd=""
    if command -v curl &> /dev/null; then
        download_cmd="curl -L --progress-bar '$DOWNLOAD_URL' -o '$TEMP_DIR/$INSTALLER_NAME'"
    elif command -v wget &> /dev/null; then
        download_cmd="wget --progress=bar '$DOWNLOAD_URL' -O '$TEMP_DIR/$INSTALLER_NAME'"
    else
        error_exit "未找到 curl 或 wget 下载工具"
    fi
    
    log_info "开始下载..."
    if ! eval "$download_cmd"; then
        log_error "下载失败，请检查网络连接"
        log_info "可尝试的解决方案："
        log_info "1. 检查网络连接是否正常"
        log_info "2. 尝试使用VPN或更换网络"
        log_info "3. 手动下载文件到 ~/Downloads/ 目录：$DOWNLOAD_URL"
        log_info "4. 使用官方下载源：https://download.qt.io/official_releases/online_installers/"
        error_exit "下载中断"
    fi
    
    log_success "下载完成!"
    
    # 验证文件
    if [[ ! -f "$TEMP_DIR/$INSTALLER_NAME" ]]; then
        error_exit "安装器文件不存在"
    fi
    
    # 检查文件大小
    local file_size
    if [ "$OS" = "Darwin" ]; then
        file_size=$(stat -f%z "$TEMP_DIR/$INSTALLER_NAME")
    else
        file_size=$(stat -c%s "$TEMP_DIR/$INSTALLER_NAME")
    fi
    
    if [[ $file_size -lt 1048576 ]]; then
        error_exit "下载的文件可能损坏（文件太小: ${file_size} 字节）"
    fi
    
    log_info "文件大小: $(( file_size / 1048576 )) MB"
}

# 安装 Qt
install_qt() {
    log_info "准备安装 Qt..."
    
    # 显示安装提示
    echo
    echo "========================================"
    echo "安装提示:"
    echo "1. 安装器将使用阿里云镜像加速下载"
    echo "2. 请在安装器中登录您的 Qt 账户"
    echo "3. 推荐安装组件:"
    echo "   - Qt 6.x (最新版本)"
    echo "   - Qt Creator"
    echo "   - CMake"
    if [ "$OS" = "Darwin" ]; then
        echo "   - Xcode 工具链"
        echo "4. 安装路径建议: ~/Qt 或 /Applications/Qt"
    else
        echo "   - GCC 编译器"
        echo "4. 安装路径建议: ~/Qt 或 /opt/Qt"
    fi
    echo "========================================"
    echo
    
    # 询问是否继续
    read -p "按回车键继续安装，或输入 n 取消: " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Nn]$ ]]; then
        log_info "安装已取消"
        exit 0
    fi
    
    case "$OS" in
        Darwin)
            install_qt_macos
            ;;
        Linux)
            install_qt_linux
            ;;
    esac
}

# macOS Qt 安装
install_qt_macos() {
    # 挂载 DMG
    log_info "挂载安装器镜像..."
    MOUNT_POINT=$(hdiutil attach "$TEMP_DIR/$INSTALLER_NAME" | grep "/Volumes/" | awk '{print $3}')
    
    if [[ -z "$MOUNT_POINT" ]]; then
        error_exit "无法挂载安装器镜像"
    fi
    
    log_info "镜像已挂载到: $MOUNT_POINT"
    
    # 查找安装器可执行文件
    local installer_app
    installer_app=$(find "$MOUNT_POINT" -name "*.app" -type d | head -1)
    
    if [[ -z "$installer_app" ]]; then
        error_exit "未找到安装器应用程序"
    fi
    
    local installer_executable="$installer_app/Contents/MacOS/$(basename "$installer_app" .app)"
    
    if [[ ! -f "$installer_executable" ]]; then
        error_exit "未找到安装器可执行文件"
    fi
    
    # 启动安装器
    log_info "启动安装器..."
    log_info "使用镜像源: $MIRROR_URL"
    
    # 尝试使用镜像参数启动
    if ! "$installer_executable" --mirror "$MIRROR_URL"; then
        log_warning "带镜像参数启动失败，尝试正常启动..."
        "$installer_executable"
    fi
}

# Linux Qt 安装
install_qt_linux() {
    # 设置可执行权限
    chmod +x "$TEMP_DIR/$INSTALLER_NAME"
    
    # 启动安装器
    log_info "启动安装器..."
    log_info "使用镜像源: $MIRROR_URL"
    
    # 尝试使用镜像参数启动
    if ! "$TEMP_DIR/$INSTALLER_NAME" --mirror "$MIRROR_URL"; then
        log_warning "带镜像参数启动失败，尝试正常启动..."
        "$TEMP_DIR/$INSTALLER_NAME"
    fi
}

# 配置环境
setup_environment() {
    log_info "配置环境变量..."
    
    # 查找 Qt 安装路径
    local qt_paths=()
    case "$OS" in
        Darwin)
            qt_paths=(
                "$HOME/Qt/*/bin"
                "/Applications/Qt/*/bin"
                "/opt/Qt/*/bin"
                "/usr/local/Qt/*/bin"
            )
            ;;
        Linux)
            qt_paths=(
                "$HOME/Qt/*/bin"
                "/opt/Qt/*/bin"
                "/usr/local/Qt/*/bin"
                "/usr/Qt/*/bin"
            )
            ;;
    esac
    
    local qt_bin_path=""
    for path_pattern in "${qt_paths[@]}"; do
        for path in $path_pattern; do
            if [[ -f "$path/qmake" ]]; then
                qt_bin_path="$path"
                break 2
            fi
        done
    done
    
    if [[ -n "$qt_bin_path" ]]; then
        log_success "找到 Qt 安装路径: $qt_bin_path"
        
        # 配置 shell 环境
        local shells=()
        [[ -f "$HOME/.bashrc" ]] && shells+=(".bashrc")
        [[ -f "$HOME/.bash_profile" ]] && shells+=(".bash_profile")
        [[ -f "$HOME/.zshrc" ]] && shells+=(".zshrc")
        [[ -f "$HOME/.profile" ]] && shells+=(".profile")
        
        if [[ ${#shells[@]} -eq 0 ]]; then
            log_warning "未找到 shell 配置文件"
            log_info "请手动将以下路径添加到 PATH:"
            log_info "export PATH=\"$qt_bin_path:\$PATH\""
            return
        fi
        
        for shell_config in "${shells[@]}"; do
            local shell_file="$HOME/$shell_config"
            
            # 检查是否已配置
            if grep -q "$qt_bin_path" "$shell_file" 2>/dev/null; then
                log_info "$shell_config 已包含 Qt 路径"
                continue
            fi
            
            # 添加到配置文件
            echo "" >> "$shell_file"
            echo "# Qt PATH (added by install script)" >> "$shell_file"
            echo "export PATH=\"$qt_bin_path:\$PATH\"" >> "$shell_file"
            log_success "已更新 $shell_config"
        done
        
        # 设置当前会话的环境变量
        export PATH="$qt_bin_path:$PATH"
        
        log_info "环境变量配置完成"
        log_info "请重新启动终端或运行 'source ~/.zshrc' (或对应的shell配置文件)"
    else
        log_warning "未找到 Qt 安装路径"
        log_info "请在安装完成后手动配置环境变量"
    fi
}

# 验证安装
verify_installation() {
    log_info "验证安装..."
    
    if command -v qmake &> /dev/null; then
        local qt_version
        qt_version=$(qmake --version | head -1)
        log_success "Qt 安装成功!"
        log_info "版本信息: $qt_version"
    else
        log_warning "qmake 命令未找到"
        log_info "可能需要重新启动终端或手动配置环境变量"
    fi
    
    # 检查 Qt Creator
    local qt_creator_paths=()
    case "$OS" in
        Darwin)
            qt_creator_paths=(
                "/Applications/Qt Creator.app"
                "$HOME/Applications/Qt Creator.app"
                "$HOME/Qt/Tools/QtCreator/Qt Creator.app"
                "/Applications/Qt/Tools/QtCreator/Qt Creator.app"
            )
            ;;
        Linux)
            qt_creator_paths=(
                "$HOME/Qt/Tools/QtCreator/bin/qtcreator"
                "/opt/Qt/Tools/QtCreator/bin/qtcreator"
                "/usr/local/Qt/Tools/QtCreator/bin/qtcreator"
                "/usr/bin/qtcreator"
            )
            ;;
    esac
    
    for creator_path in "${qt_creator_paths[@]}"; do
        if [[ -e "$creator_path" ]]; then
            log_success "找到 Qt Creator: $creator_path"
            break
        fi
    done
}

# 主函数
main() {
    # 检测操作系统
    detect_os
    
    # 检查系统要求
    check_requirements
    
    # 下载安装器
    download_installer
    
    # 安装 Qt
    install_qt
    
    # 询问是否配置环境变量
    echo
    read -p "是否需要配置环境变量? (Y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        setup_environment
    fi
    
    # 验证安装
    verify_installation
    
    # 完成
    echo
    echo "========================================"
    log_success "Qt 安装完成!"
    echo
    echo "后续步骤:"
    echo "1. 重启终端以使环境变量生效"
    echo "2. 运行 'qmake --version' 验证安装"
    echo "3. 启动 Qt Creator 开始开发"
    case "$OS" in
        Darwin)
            echo "   - 在 Launchpad 中查找 Qt Creator"
            echo "   - 或运行 'open \"/Applications/Qt Creator.app\"'"
            ;;
        Linux)
            echo "   - 运行 'qtcreator' 命令"
            echo "   - 或在应用程序菜单中查找 Qt Creator"
            ;;
    esac
    echo "========================================"
}

# 运行主函数
main "$@"
