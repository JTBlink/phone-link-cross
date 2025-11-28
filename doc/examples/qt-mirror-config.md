# Qt 镜像配置示例

本文档提供了在不同环境中配置Qt阿里云镜像的示例。

## 1. 环境变量配置

### Windows (PowerShell)
```powershell
# 临时设置（当前会话）
$env:QT_MIRROR_URL = "https://mirrors.aliyun.com/qt"

# 永久设置
[Environment]::SetEnvironmentVariable("QT_MIRROR_URL", "https://mirrors.aliyun.com/qt", "User")
```

### Windows (CMD)
```cmd
# 临时设置
set QT_MIRROR_URL=https://mirrors.aliyun.com/qt

# 永久设置
setx QT_MIRROR_URL "https://mirrors.aliyun.com/qt"
```

### Unix (Bash/Zsh)
```bash
# 临时设置
export QT_MIRROR_URL="https://mirrors.aliyun.com/qt"

# 永久设置 - 添加到 ~/.bashrc 或 ~/.zshrc
echo 'export QT_MIRROR_URL="https://mirrors.aliyun.com/qt"' >> ~/.zshrc
source ~/.zshrc
```

## 2. CMake 项目配置

### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.16)
project(QtMirrorExample)

# 设置Qt镜像（如果环境变量存在）
if(DEFINED ENV{QT_MIRROR_URL})
    message(STATUS "使用Qt镜像: $ENV{QT_MIRROR_URL}")
endif()

# 查找Qt组件
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# 创建可执行文件
qt_standard_project_setup()
qt_add_executable(QtMirrorExample main.cpp)

# 链接Qt库
target_link_libraries(QtMirrorExample Qt6::Core Qt6::Widgets)
```

### CMakePresets.json
```json
{
    "version": 3,
    "configurePresets": [
        {
            "name": "default",
            "displayName": "默认配置",
            "description": "使用阿里云镜像的默认配置",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "QT_MIRROR_URL": "https://mirrors.aliyun.com/qt"
            },
            "environment": {
                "QT_MIRROR_URL": "https://mirrors.aliyun.com/qt"
            }
        }
    ]
}
```

## 3. Qt Creator 配置

### 全局设置文件位置
- **Windows**: `%APPDATA%\QtProject\qtcreator\`
- **macOS**: `~/Library/Preferences/QtProject/`
- **Linux**: `~/.config/QtProject/`

### 手动配置步骤
1. 打开 Qt Creator
2. 进入 `Tools` > `Options` > `Kits`
3. 在 `Qt Versions` 标签中添加自定义Qt版本
4. 设置 `qmake` 路径指向阿里云镜像安装的版本

## 4. qmake 项目配置

### example.pro
```pro
QT += core widgets

TARGET = QtMirrorExample
TEMPLATE = app

# 项目源文件
SOURCES += main.cpp

# 如果使用阿里云镜像安装的Qt
message("当前Qt版本: $$[QT_VERSION]")
message("Qt安装路径: $$[QT_INSTALL_PREFIX]")

# 输出配置信息
CONFIG(debug, debug|release) {
    message("构建模式: Debug")
} else {
    message("构建模式: Release")
}
```

## 5. 持续集成配置

### GitHub Actions
```yaml
name: Qt Build with Aliyun Mirror

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: 安装Qt（使用阿里云镜像）
      run: |
        curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-unix.sh
        chmod +x install-qt-unix.sh
        ./install-qt-unix.sh
      
    - name: 验证Qt安装
      run: |
        qmake --version
        
    - name: 构建项目
      run: |
        qmake
        make
```

### Docker 配置
```dockerfile
FROM ubuntu:22.04

# 安装依赖
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    ca-certificates \
    libgl1-mesa-glx \
    libxcb-xinerama0

# 下载并运行Qt安装脚本
RUN curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-unix.sh \
    && chmod +x install-qt-unix.sh \
    && ./install-qt-unix.sh

# 设置环境变量
ENV PATH="/opt/Qt/6.x.x/gcc_64/bin:${PATH}"
ENV QT_MIRROR_URL="https://mirrors.aliyun.com/qt"

WORKDIR /app
COPY . .

# 构建项目
RUN qmake && make
```

## 6. IDE 集成

### VS Code settings.json
```json
{
    "qt.qmakePath": "/path/to/qt/bin/qmake",
    "qt.cmakePath": "/path/to/cmake/bin/cmake",
    "terminal.integrated.env.linux": {
        "QT_MIRROR_URL": "https://mirrors.aliyun.com/qt"
    },
    "terminal.integrated.env.osx": {
        "QT_MIRROR_URL": "https://mirrors.aliyun.com/qt"
    },
    "terminal.integrated.env.windows": {
        "QT_MIRROR_URL": "https://mirrors.aliyun.com/qt"
    }
}
```

### CLion CMake配置
```
-DCMAKE_PREFIX_PATH=/path/to/qt
-DQT_MIRROR_URL=https://mirrors.aliyun.com/qt
```

## 7. 脚本化安装

### 批量安装脚本 (install-team-qt.sh)
```bash
#!/bin/bash
# 团队Qt环境快速部署脚本

MIRROR_URL="https://mirrors.aliyun.com/qt"
TEAM_QT_VERSION="6.5.0"

echo "=== 团队Qt环境部署 ==="
echo "镜像源: $MIRROR_URL"
echo "目标版本: $TEAM_QT_VERSION"

# 下载并执行安装脚本
curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-unix.sh
chmod +x install-qt-unix.sh

# 设置环境变量后执行
export QT_MIRROR_URL="$MIRROR_URL"
./install-qt-unix.sh

# 验证安装
if command -v qmake &> /dev/null; then
    echo "✅ Qt安装成功!"
    qmake --version
else
    echo "❌ Qt安装失败"
    exit 1
fi
```

## 验证配置

运行以下命令验证配置是否生效：

```bash
# 检查环境变量
echo $QT_MIRROR_URL

# 检查Qt版本
qmake --version

# 检查Qt安装路径
qmake -query QT_INSTALL_PREFIX

# 构建测试项目
mkdir test-qt && cd test-qt
qmake -project
qmake
make  # 或 nmake (Windows)
```

## 故障排除

### 常见问题
1. **环境变量未生效**: 重启终端或IDE
2. **路径包含空格**: 使用引号包围路径
3. **权限问题**: 确保有足够的文件系统权限
4. **版本冲突**: 卸载旧版本或使用完整路径

### 日志查看
```bash
# Qt安装日志
ls ~/.qt-installer-framework/

# CMake详细输出
cmake --debug-output .

# qmake详细输出
qmake -d
