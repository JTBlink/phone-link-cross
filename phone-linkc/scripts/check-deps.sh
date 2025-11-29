#!/bin/bash

# iOS 设备管理器 - 依赖检测和安装脚本
# 智能检测 libimobiledevice 安装状态并提供解决方案

set -e

echo "========================================="
echo "iOS 设备管理器 - 依赖检测"
echo "========================================="

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检测 macOS 架构
detect_arch() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        ARCH=$(uname -m)
        if [ "$ARCH" = "arm64" ]; then
            HOMEBREW_PREFIX="/opt/homebrew"
            echo "检测到: Apple Silicon Mac (M1/M2)"
        else
            HOMEBREW_PREFIX="/usr/local"
            echo "检测到: Intel Mac"
        fi
    else
        echo "检测到: Linux 系统"
    fi
}

# 检查 Homebrew 状态
check_homebrew() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo ""
        echo "=== Homebrew 状态检查 ==="
        
        if command -v brew >/dev/null 2>&1; then
            BREW_PREFIX=$(brew --prefix)
            echo -e "${GREEN}✓${NC} Homebrew 已安装: $BREW_PREFIX"
            
            # 检查是否与预期路径一致
            if [ "$BREW_PREFIX" != "$HOMEBREW_PREFIX" ]; then
                echo -e "${YELLOW}⚠${NC} Homebrew 路径不匹配:"
                echo "  预期: $HOMEBREW_PREFIX"
                echo "  实际: $BREW_PREFIX"
                HOMEBREW_PREFIX="$BREW_PREFIX"
            fi
        else
            echo -e "${RED}✗${NC} Homebrew 未安装"
            echo "请安装 Homebrew: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
            exit 1
        fi
    fi
}

# 检查 pkg-config
check_pkg_config() {
    echo ""
    echo "=== pkg-config 状态检查 ==="
    
    if command -v pkg-config >/dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} pkg-config 已安装: $(pkg-config --version)"
        
        # 检查 PKG_CONFIG_PATH
        if [ -n "$PKG_CONFIG_PATH" ]; then
            echo "PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
        else
            echo -e "${YELLOW}⚠${NC} PKG_CONFIG_PATH 未设置"
            if [[ "$OSTYPE" == "darwin"* ]]; then
                export PKG_CONFIG_PATH="$HOMEBREW_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
                echo "自动设置: PKG_CONFIG_PATH=$PKG_CONFIG_PATH"
            fi
        fi
        
        # 显示可用的包
        echo "可用的包:"
        pkg-config --list-all | grep -E "(libimobiledevice|libplist|libusbmuxd)" || echo "  未找到 libimobiledevice 相关包"
        
    else
        echo -e "${RED}✗${NC} pkg-config 未安装"
        if [[ "$OSTYPE" == "darwin"* ]]; then
            echo "安装命令: brew install pkg-config"
        else
            echo "安装命令: sudo apt-get install pkg-config"
        fi
        return 1
    fi
}

# 检查 libimobiledevice 包
check_libimobiledevice_packages() {
    echo ""
    echo "=== libimobiledevice 包状态检查 ==="
    
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # 检查 Homebrew 包
        PACKAGES=("libimobiledevice" "libplist" "libusbmuxd")
        for pkg in "${PACKAGES[@]}"; do
            if brew list "$pkg" >/dev/null 2>&1; then
                VERSION=$(brew list --versions "$pkg" | awk '{print $2}')
                echo -e "${GREEN}✓${NC} $pkg 已安装: $VERSION"
            else
                echo -e "${RED}✗${NC} $pkg 未安装"
                MISSING_PACKAGES+=("$pkg")
            fi
        done
    else
        # 检查 apt 包
        PACKAGES=("libimobiledevice-dev" "libplist-dev" "libusbmuxd-dev")
        for pkg in "${PACKAGES[@]}"; do
            if dpkg -l | grep -q "$pkg"; then
                VERSION=$(dpkg -l | grep "$pkg" | awk '{print $3}')
                echo -e "${GREEN}✓${NC} $pkg 已安装: $VERSION"
            else
                echo -e "${RED}✗${NC} $pkg 未安装"
                MISSING_PACKAGES+=("$pkg")
            fi
        done
    fi
}

# 检查头文件和库文件
check_files() {
    echo ""
    echo "=== 文件系统检查 ==="
    
    # 可能的路径
    if [[ "$OSTYPE" == "darwin"* ]]; then
        INCLUDE_PATHS=("$HOMEBREW_PREFIX/include" "/usr/local/include" "/opt/homebrew/include")
        LIB_PATHS=("$HOMEBREW_PREFIX/lib" "/usr/local/lib" "/opt/homebrew/lib")
        PKGCONFIG_PATHS=("$HOMEBREW_PREFIX/lib/pkgconfig" "/usr/local/lib/pkgconfig" "/opt/homebrew/lib/pkgconfig")
    else
        INCLUDE_PATHS=("/usr/include" "/usr/local/include")
        LIB_PATHS=("/usr/lib" "/usr/local/lib" "/lib")
        PKGCONFIG_PATHS=("/usr/lib/pkgconfig" "/usr/local/lib/pkgconfig" "/lib/pkgconfig")
    fi
    
    # 检查头文件
    echo "头文件检查:"
    # 使用正确的头文件路径
    HEADERS=("libimobiledevice/libimobiledevice.h" "plist/plist.h" "usbmuxd.h")
    HEADER_NAMES=("libimobiledevice" "libplist" "libusbmuxd")
    
    for i in "${!HEADERS[@]}"; do
        header="${HEADERS[$i]}"
        name="${HEADER_NAMES[$i]}"
        found=false
        
        for path in "${INCLUDE_PATHS[@]}"; do
            if [ -f "$path/$header" ]; then
                echo -e "${GREEN}✓${NC} $name ($header) 找到: $path/$header"
                found=true
                break
            fi
        done
        
        if [ "$found" = false ]; then
            echo -e "${RED}✗${NC} $name ($header) 未找到"
        fi
    done
    
    echo ""
    echo "库文件检查:"
    LIBS=("libimobiledevice" "libplist" "libusbmuxd")
    for lib in "${LIBS[@]}"; do
        found=false
        for path in "${LIB_PATHS[@]}"; do
            if ls "$path/$lib"*.dylib >/dev/null 2>&1 || ls "$path/$lib"*.so* >/dev/null 2>&1; then
                echo -e "${GREEN}✓${NC} $lib 库找到: $path/"
                ls "$path/$lib"* 2>/dev/null | head -1
                found=true
                break
            fi
        done
        if [ "$found" = false ]; then
            echo -e "${RED}✗${NC} $lib 库未找到"
        fi
    done
    
    echo ""
    echo "pkg-config 文件检查:"
    PKGCONFIGS=("libimobiledevice-1.0.pc" "libplist-2.0.pc" "libusbmuxd-2.0.pc")
    for pc in "${PKGCONFIGS[@]}"; do
        found=false
        for path in "${PKGCONFIG_PATHS[@]}"; do
            if [ -f "$path/$pc" ]; then
                echo -e "${GREEN}✓${NC} $pc 找到: $path/$pc"
                found=true
                break
            fi
        done
        if [ "$found" = false ]; then
            echo -e "${RED}✗${NC} $pc 未找到"
        fi
    done
}

# 检查 pkg-config 查询
check_pkg_config_query() {
    echo ""
    echo "=== pkg-config 查询测试 ==="
    
    MODULES=("libimobiledevice-1.0" "libplist-2.0" "libusbmuxd-2.0")
    ALL_MODULES_FOUND=true
    
    for module in "${MODULES[@]}"; do
        if pkg-config --exists "$module" 2>/dev/null; then
            VERSION=$(pkg-config --modversion "$module")
            CFLAGS=$(pkg-config --cflags "$module")
            LIBS=$(pkg-config --libs "$module")
            echo -e "${GREEN}✓${NC} $module 可查询"
            echo "  版本: $VERSION"
            echo "  CFLAGS: $CFLAGS"
            echo "  LIBS: $LIBS"
            
            # 验证头文件是否真的可以找到
            check_headers_from_cflags "$module" "$CFLAGS"
        else
            echo -e "${RED}✗${NC} $module 无法查询"
            ALL_MODULES_FOUND=false
        fi
        echo ""
    done
    
    # 如果所有模块都找到了，测试编译
    if [ "$ALL_MODULES_FOUND" = true ]; then
        test_compilation
    fi
}

# 验证从 CFLAGS 中的头文件路径
check_headers_from_cflags() {
    local module=$1
    local cflags=$2
    
    echo "    验证头文件可访问性:"
    
    # 提取 -I 路径
    include_paths=$(echo "$cflags" | grep -o '\-I[^ ]*' | sed 's/-I//')
    
    if [ "$module" = "libimobiledevice-1.0" ]; then
        header="libimobiledevice/libimobiledevice.h"
    elif [ "$module" = "libplist-2.0" ]; then
        header="plist/plist.h"  # 使用正确的路径
    elif [ "$module" = "libusbmuxd-2.0" ]; then
        header="usbmuxd.h"      # 使用正确的路径
    fi
    
    found_header=false
    for path in $include_paths; do
        if [ -f "$path/$header" ]; then
            echo -e "    ${GREEN}✓${NC} $header 在 $path/ 中找到"
            found_header=true
        fi
    done
    
    if [ "$found_header" = false ]; then
        echo -e "    ${RED}✗${NC} $header 在指定的包含路径中未找到"
        echo -e "    ${YELLOW}⚠${NC} 这可能导致 CMake 配置失败"
    fi
}

# 测试实际编译
test_compilation() {
    echo ""
    echo "=== 编译测试 ==="
    
    # 创建临时测试文件
    TEST_DIR="/tmp/libimobiledevice_test_$$"
    mkdir -p "$TEST_DIR"
    
    cat > "$TEST_DIR/test.c" << 'EOF'
#include <stdio.h>
#include <libimobiledevice/libimobiledevice.h>
#include <plist/plist.h>

int main() {
    char **devices = NULL;
    int count = 0;
    
    if (idevice_get_device_list(&devices, &count) == IDEVICE_E_SUCCESS) {
        printf("libimobiledevice 编译测试成功！\n");
        idevice_device_list_free(devices);
        return 0;
    }
    
    return 1;
}
EOF

    # 尝试编译
    cd "$TEST_DIR"
    
    CFLAGS=$(pkg-config --cflags libimobiledevice-1.0 libplist-2.0)
    LIBS=$(pkg-config --libs libimobiledevice-1.0 libplist-2.0)
    
    echo "编译命令: gcc test.c $CFLAGS $LIBS -o test"
    
    if gcc test.c $CFLAGS $LIBS -o test 2>/dev/null; then
        echo -e "${GREEN}✓${NC} 编译测试成功！"
        echo "libimobiledevice 库可以正常使用"
        
        # 尝试运行测试程序
        if ./test >/dev/null 2>&1; then
            echo -e "${GREEN}✓${NC} 运行测试成功！"
        else
            echo -e "${YELLOW}⚠${NC} 编译成功但运行失败（这是正常的，可能需要连接设备）"
        fi
    else
        echo -e "${RED}✗${NC} 编译测试失败！"
        echo "错误详情:"
        gcc test.c $CFLAGS $LIBS -o test
        echo ""
        echo -e "${YELLOW}可能的解决方案:${NC}"
        echo "1. 重新安装 libimobiledevice: brew reinstall libimobiledevice"
        echo "2. 清理并重新安装所有依赖: brew uninstall libimobiledevice libplist libusbmuxd && brew install libimobiledevice"
        echo "3. 检查 Xcode 命令行工具: xcode-select --install"
    fi
    
    # 清理测试文件
    cd - >/dev/null
    rm -rf "$TEST_DIR"
}

# 检查 CMake 配置问题
check_cmake_issues() {
    echo ""
    echo "=== CMake 配置问题诊断 ==="
    
    # 检查是否存在构建目录
    if [ -d "build" ]; then
        echo "检查现有的 CMake 配置..."
        
        if [ -f "build/CMakeCache.txt" ]; then
            echo "分析 CMakeCache.txt:"
            
            # 检查 libimobiledevice 相关的 CMake 变量
            if grep -q "HAVE_LIBIMOBILEDEVICE" "build/CMakeCache.txt"; then
                HAVE_LIB=$(grep "HAVE_LIBIMOBILEDEVICE" "build/CMakeCache.txt" | cut -d'=' -f2)
                echo "  HAVE_LIBIMOBILEDEVICE: $HAVE_LIB"
            fi
            
            # 检查包含目录
            if grep -q "IMOBILEDEVICE_INCLUDE_DIRS" "build/CMakeCache.txt"; then
                INCLUDE_DIRS=$(grep "IMOBILEDEVICE_INCLUDE_DIRS" "build/CMakeCache.txt" | cut -d'=' -f2)
                echo "  包含目录: $INCLUDE_DIRS"
            fi
            
            # 检查库文件
            if grep -q "IMOBILEDEVICE_LIBRARIES" "build/CMakeCache.txt"; then
                LIBRARIES=$(grep "IMOBILEDEVICE_LIBRARIES" "build/CMakeCache.txt" | cut -d'=' -f2)
                echo "  库文件: $LIBRARIES"
            fi
            
            # 检查头文件检查结果
            if grep -q "HAVE_IDEVICE_HEADER" "build/CMakeCache.txt"; then
                HAVE_HEADER=$(grep "HAVE_IDEVICE_HEADER" "build/CMakeCache.txt" | cut -d'=' -f2)
                echo "  头文件检查: HAVE_IDEVICE_HEADER=$HAVE_HEADER"
                
                if [ "$HAVE_HEADER" != "1" ]; then
                    echo -e "  ${RED}✗${NC} CMake 无法找到 libimobiledevice 头文件"
                    echo -e "  ${YELLOW}建议解决方案:${NC}"
                    echo "    1. 清理构建缓存: rm -rf build && mkdir build"
                    echo "    2. 重新安装 libimobiledevice: brew reinstall libimobiledevice"
                    echo "    3. 检查环境变量: echo \$PKG_CONFIG_PATH"
                else
                    echo -e "  ${GREEN}✓${NC} CMake 可以找到头文件"
                fi
            fi
            
            if grep -q "HAVE_PLIST_HEADER" "build/CMakeCache.txt"; then
                HAVE_PLIST=$(grep "HAVE_PLIST_HEADER" "build/CMakeCache.txt" | cut -d'=' -f2)
                echo "  plist 头文件检查: HAVE_PLIST_HEADER=$HAVE_PLIST"
                
                if [ "$HAVE_PLIST" != "1" ]; then
                    echo -e "  ${RED}✗${NC} CMake 无法找到 libplist 头文件"
                fi
            fi
        else
            echo -e "${YELLOW}⚠${NC} 未找到 CMakeCache.txt，可能需要先运行 CMake 配置"
        fi
        
        echo ""
        echo "建议的 CMake 重新配置步骤:"
        echo "1. cd phone-linkc"
        echo "2. rm -rf build"
        echo "3. mkdir build && cd build"
        echo "4. export PKG_CONFIG_PATH=\"$HOMEBREW_PREFIX/lib/pkgconfig:\$PKG_CONFIG_PATH\""
        echo "5. cmake .."
        echo "6. make"
    else
        echo "未找到构建目录，请先运行构建脚本"
    fi
}

# 提供安装建议
provide_installation_advice() {
    echo ""
    echo "========================================="
    echo "安装建议"
    echo "========================================="
    
    if [ ${#MISSING_PACKAGES[@]} -gt 0 ]; then
        echo -e "${YELLOW}缺失的包:${NC}"
        for pkg in "${MISSING_PACKAGES[@]}"; do
            echo "  - $pkg"
        done
        echo ""
        
        if [[ "$OSTYPE" == "darwin"* ]]; then
            echo -e "${GREEN}macOS 安装命令:${NC}"
            echo "brew install ${MISSING_PACKAGES[*]}"
            echo ""
            echo "如果已经安装但检测不到，尝试:"
            echo "brew reinstall libimobiledevice libplist libusbmuxd"
            echo ""
            echo "设置环境变量 (添加到 ~/.zshrc 或 ~/.bash_profile):"
            echo "export PKG_CONFIG_PATH=\"$HOMEBREW_PREFIX/lib/pkgconfig:\$PKG_CONFIG_PATH\""
        else
            echo -e "${GREEN}Linux 安装命令:${NC}"
            echo "sudo apt-get update"
            echo "sudo apt-get install ${MISSING_PACKAGES[*]} pkg-config"
        fi
        
        echo ""
        echo -e "${YELLOW}安装后重新运行此脚本验证。${NC}"
    else
        echo -e "${GREEN}✓ 所有必需的包都已安装！${NC}"
        echo ""
        echo "如果 CMake 仍然找不到 libimobiledevice，请检查:"
        echo "1. PKG_CONFIG_PATH 环境变量是否正确设置"
        echo "2. 重新开启终端会话让环境变量生效"
        echo "3. 运行 'pkg-config --exists libimobiledevice-1.0' 验证"
        echo "4. 清理并重新构建: rm -rf build && mkdir build"
    fi
}

# 自动安装 (可选)
auto_install() {
    if [ ${#MISSING_PACKAGES[@]} -gt 0 ]; then
        echo ""
        read -p "是否自动安装缺失的包? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            if [[ "$OSTYPE" == "darwin"* ]]; then
                echo "正在通过 Homebrew 安装..."
                brew install "${MISSING_PACKAGES[@]}"
                
                # 设置环境变量
                ENV_FILE=""
                if [ -f "$HOME/.zshrc" ]; then
                    ENV_FILE="$HOME/.zshrc"
                elif [ -f "$HOME/.bash_profile" ]; then
                    ENV_FILE="$HOME/.bash_profile"
                fi
                
                if [ -n "$ENV_FILE" ]; then
                    if ! grep -q "PKG_CONFIG_PATH.*$HOMEBREW_PREFIX/lib/pkgconfig" "$ENV_FILE"; then
                        echo "" >> "$ENV_FILE"
                        echo "# libimobiledevice pkg-config path" >> "$ENV_FILE"
                        echo "export PKG_CONFIG_PATH=\"$HOMEBREW_PREFIX/lib/pkgconfig:\$PKG_CONFIG_PATH\"" >> "$ENV_FILE"
                        echo "环境变量已添加到 $ENV_FILE"
                        echo "请重新开启终端或运行: source $ENV_FILE"
                    fi
                fi
                
            else
                echo "正在通过 apt 安装..."
                sudo apt-get update
                sudo apt-get install "${MISSING_PACKAGES[@]}" pkg-config
            fi
            
            echo ""
            echo "安装完成，重新检查..."
            check_libimobiledevice_packages
            check_files
            check_pkg_config_query
        fi
    fi
}

# 主函数
main() {
    MISSING_PACKAGES=()
    
    detect_arch
    check_homebrew
    check_pkg_config
    check_libimobiledevice_packages
    check_files
    check_pkg_config_query
    
    # 检查 CMake 配置问题（如果存在构建目录）
    if [ -d "build" ] || [ -d "../build" ]; then
        check_cmake_issues
    fi
    
    provide_installation_advice
    
    # 如果在交互模式下，提供自动安装选项
    if [ -t 0 ]; then
        auto_install
    fi
    
    echo ""
    echo "========================================="
    echo "检查完成"
    echo "========================================="
    echo ""
    echo "如果所有检查都通过但应用程序仍显示'libimobiledevice 不可用'："
    echo "1. 确保环境变量已正确设置并重新开启终端"
    echo "2. 清理并重新构建项目"
    echo "3. 运行此脚本确认所有检查都通过"
    echo ""
    echo "接下来可以运行:"
    echo "  ./build.sh build         - 构建项目"
    echo "  ./build.sh check-deps    - 再次检查依赖"
    echo "  ./scripts/check-deps.sh  - 运行完整的依赖诊断"
}

# 运行主函数
main "$@"
