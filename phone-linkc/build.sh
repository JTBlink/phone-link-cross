#!/bin/bash

# iOS 设备管理器构建脚本
# 支持 macOS 和 Linux 系统

set -e  # 遇到错误时退出

echo "========================================="
echo "iOS 设备管理器 - 构建脚本"
echo "========================================="

# 检查 Qt 环境
check_qt() {
    # 首先检查系统 PATH 中的 qmake
    if command -v qmake >/dev/null 2>&1; then
        echo "✓ 找到 Qt (系统PATH): $(qmake -version | head -1)"
        return 0
    fi
    
    # 检查常见的 Qt 安装路径
    QT_POSSIBLE_PATHS=(
        "$HOME/Qt/6.10.1/macos/bin"
        "$HOME/Qt/6.10.0/macos/bin"
        "$HOME/Qt/6.9.1/macos/bin"
        "$HOME/Qt/6.9.0/macos/bin"
        "$HOME/Qt/6.8.1/macos/bin"
        "$HOME/Qt/6.8.0/macos/bin"
        "$HOME/Qt/6.7.3/macos/bin" 
        "$HOME/Qt/6.7.2/macos/bin"
        "$HOME/Qt/6.7.1/macos/bin"
        "$HOME/Qt/6.7.0/macos/bin"
        "$HOME/Qt/6.6.3/macos/bin"
        "$HOME/Qt/6.6.2/macos/bin"
        "$HOME/Qt/6.6.1/macos/bin"
        "$HOME/Qt/6.6.0/macos/bin"
        "$HOME/Qt/6.5.3/macos/bin"
        "$HOME/Qt/6.5.2/macos/bin"
        "$HOME/Qt/6.5.1/macos/bin"
        "$HOME/Qt/6.5.0/macos/bin"
        "$HOME/Qt/*/macos/bin"
        "$HOME/Qt/*/gcc_64/bin"
        "$HOME/Qt/*/linux/bin"
        "/usr/local/opt/qt6/bin"
        "/opt/homebrew/opt/qt6/bin"
        "/usr/lib/qt6/bin"
    )
    
    for qt_path in "${QT_POSSIBLE_PATHS[@]}"; do
        if [ -f "$qt_path/qmake" ]; then
            echo "✓ 找到 Qt: $qt_path"
            export PATH="$qt_path:$PATH"
            QT_FOUND_PATH="$qt_path"
            echo "✓ Qt 版本: $($qt_path/qmake -version | head -1)"
            return 0
        fi
    done
    
    # 尝试查找 Qt 目录
    if [ -d "$HOME/Qt" ]; then
        echo "发现 Qt 安装目录: $HOME/Qt"
        echo "可用的 Qt 版本:"
        find "$HOME/Qt" -name "qmake" -type f 2>/dev/null | head -5 | while read qmake_path; do
            echo "  - $(dirname "$qmake_path")"
        done
        echo ""
        echo "请手动设置 Qt 路径，例如:"
        echo "  export PATH=\"\$HOME/Qt/6.7.3/macos/bin:\$PATH\""
        echo "然后重新运行构建脚本"
        exit 1
    fi
    
    echo "✗ 未找到 Qt，请安装 Qt 6.5+"
    echo "  macOS: brew install qt6"
    echo "  Ubuntu: sudo apt-get install qt6-base-dev"
    echo "  或者从 https://www.qt.io/download-qt-installer 下载"
    exit 1
}

# 检查 CMake
check_cmake() {
    if command -v cmake >/dev/null 2>&1; then
        echo "✓ 找到 CMake: $(cmake --version | head -1)"
    else
        echo "✗ 未找到 CMake，请安装 CMake 3.19+"
        exit 1
    fi
}

# 检查 libimobiledevice（可选）
check_libimobiledevice() {
    if pkg-config --exists libimobiledevice-1.0; then
        echo "✓ 找到 libimobiledevice: $(pkg-config --modversion libimobiledevice-1.0)"
        HAVE_LIBIMOBILEDEVICE=true
    else
        echo "⚠ 未找到 libimobiledevice，将使用模拟模式"
        echo "  安装方法:"
        echo "    macOS: brew install libimobiledevice"
        echo "    Ubuntu: sudo apt-get install libimobiledevice-dev"
        HAVE_LIBIMOBILEDEVICE=false
    fi
}

# 清理构建目录
clean_build() {
    if [ -d "build" ]; then
        echo "清理旧的构建文件..."
        rm -rf build
    fi
}

# 创建构建目录
create_build_dir() {
    echo "创建构建目录..."
    mkdir -p build
    cd build
}

# 配置项目
configure_project() {
    echo "配置项目..."
    
    # 尝试自动找到 Qt
    QT_CMAKE_PATH=""
    
    # 如果之前找到了 Qt 路径，使用它
    if [ -n "$QT_FOUND_PATH" ]; then
        # 从 bin 路径推导 CMAKE_PREFIX_PATH
        QT_CMAKE_PATH=$(dirname "$QT_FOUND_PATH")
        echo "使用检测到的 Qt 路径: $QT_CMAKE_PATH"
    else
        # 检查常见的 Qt CMake 路径
        QT_CMAKE_POSSIBLE_PATHS=(
            "/usr/local/opt/qt6"
            "/opt/homebrew/opt/qt6"
            "/usr/lib/qt6"
            "$HOME/Qt/6.10.1/macos"
            "$HOME/Qt/6.10.0/macos"
            "$HOME/Qt/6.9.1/macos"
            "$HOME/Qt/6.9.0/macos"
            "$HOME/Qt/6.8.1/macos"
            "$HOME/Qt/6.8.0/macos"
            "$HOME/Qt/6.7.3/macos" 
            "$HOME/Qt/6.7.2/macos"
            "$HOME/Qt/6.7.1/macos"
            "$HOME/Qt/6.7.0/macos"
            "$HOME/Qt/6.6.3/macos"
            "$HOME/Qt/6.6.2/macos"
            "$HOME/Qt/6.6.1/macos"
            "$HOME/Qt/6.6.0/macos"
            "$HOME/Qt/6.5.3/macos"
            "$HOME/Qt/6.5.2/macos"
            "$HOME/Qt/6.5.1/macos"
            "$HOME/Qt/6.5.0/macos"
        )
        
        for qt_cmake_path in "${QT_CMAKE_POSSIBLE_PATHS[@]}"; do
            if [ -d "$qt_cmake_path/lib/cmake/Qt6" ] || [ -f "$qt_cmake_path/lib/cmake/Qt6/Qt6Config.cmake" ]; then
                QT_CMAKE_PATH="$qt_cmake_path"
                echo "找到 Qt CMake 配置: $QT_CMAKE_PATH"
                break
            fi
        done
    fi
    
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
    if [ -n "$QT_CMAKE_PATH" ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_PREFIX_PATH=$QT_CMAKE_PATH"
        echo "CMake 将使用 Qt 路径: $QT_CMAKE_PATH"
    else
        echo "⚠ 未能自动检测 Qt CMake 路径，依赖系统默认路径"
    fi
    
    echo "执行: cmake .. $CMAKE_ARGS"
    cmake .. $CMAKE_ARGS
}

# 编译项目
build_project() {
    echo "编译项目..."
    cmake --build . --config Release
}

# 运行项目
run_project() {
    echo ""
    echo "========================================="
    echo "构建完成！"
    echo "========================================="
    
    if [ "$HAVE_LIBIMOBILEDEVICE" = true ]; then
        echo "✓ libimobiledevice 支持已启用"
        echo "  连接 iOS 设备并信任此计算机后即可使用"
    else
        echo "⚠ 运行在模拟模式"
        echo "  将显示模拟设备，不支持真实 iOS 设备"
    fi
    
    echo ""
    echo "运行应用程序:"
    if [ -d "./phone-linkc.app" ]; then
        echo "  open ./phone-linkc.app"
        EXECUTABLE="open ./phone-linkc.app"
    elif [ -f "./phone-linkc" ]; then
        echo "  ./phone-linkc"
        EXECUTABLE="./phone-linkc"
    elif [ -f "./Release/phone-linkc.exe" ]; then
        echo "  ./Release/phone-linkc.exe"
        EXECUTABLE="./Release/phone-linkc.exe"
    elif [ -f "./Debug/phone-linkc.exe" ]; then
        echo "  ./Debug/phone-linkc.exe" 
        EXECUTABLE="./Debug/phone-linkc.exe"
    else
        echo "  找不到可执行文件"
        echo ""
        echo "可能的位置:"
        find . -name "phone-linkc*" -type f -executable 2>/dev/null | head -5
        find . -name "phone-linkc.app" -type d 2>/dev/null | head -5
        return
    fi
    echo ""
    
    read -p "是否立即运行？(y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "启动 iOS 设备管理器..."
        if [ -d "./phone-linkc.app" ]; then
            open ./phone-linkc.app
        elif [ -n "$EXECUTABLE" ] && [ -f "$EXECUTABLE" ]; then
            $EXECUTABLE
        else
            echo "错误: 可执行文件不存在: $EXECUTABLE"
        fi
    fi
}

# 主函数
main() {
    # 检查系统环境
    check_cmake
    check_qt
    check_libimobiledevice
    
    echo ""
    echo "准备构建..."
    
    # 构建项目
    clean_build
    create_build_dir
    configure_project
    build_project
    
    # 完成
    run_project
}

# 处理命令行参数
case "$1" in
    "clean")
        echo "清理构建文件..."
        rm -rf build
        echo "清理完成"
        ;;
    "install-deps")
        echo "安装依赖..."
        if [[ "$OSTYPE" == "darwin"* ]]; then
            # macOS
            echo "在 macOS 上安装依赖:"
            echo "brew install qt6 cmake libimobiledevice"
        elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
            # Linux
            echo "在 Ubuntu/Debian 上安装依赖:"
            echo "sudo apt-get install qt6-base-dev qt6-tools-dev cmake libimobiledevice-dev libplist-dev"
        fi
        ;;
    "help"|"-h"|"--help")
        echo "iOS 设备管理器构建脚本"
        echo ""
        echo "用法:"
        echo "  ./build.sh           - 构建项目"
        echo "  ./build.sh clean     - 清理构建文件"
        echo "  ./build.sh install-deps - 显示依赖安装命令"
        echo "  ./build.sh help      - 显示此帮助信息"
        ;;
    *)
        main
        ;;
esac
