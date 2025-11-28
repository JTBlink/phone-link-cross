# Qt 镜像安装指南

## 简介

Qt是一个跨平台的应用程序开发框架，提供了整个软件开发生命周期所需的一切工具。本文档提供了使用阿里云镜像快速安装Qt的方法和脚本。

## 支持平台

- Windows (x86/x64)
- macOS (Intel/Apple Silicon)
- Linux (x64/ARM64)

## 镜像信息

- **镜像地址**: https://mirrors.aliyun.com/qt/
- **在线安装器**: https://mirrors.aliyun.com/qt/official_releases/online_installers/
- **官方主页**: https://www.qt.io/

## 快速安装

### 方法一：使用安装脚本（推荐）

我们提供了适用于不同平台的自动化安装脚本：

#### Windows
```cmd
# 下载并运行安装脚本
curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-windows.bat
install-qt-windows.bat
```

#### macOS & Linux（Unix通用脚本）
```bash
# 下载并运行Unix通用安装脚本
curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-unix.sh
chmod +x install-qt-unix.sh
./install-qt-unix.sh
```

#### 分别使用（如需要）
如果您只需要特定平台的脚本，也可以单独使用：
- **Windows**: `install-qt-windows.bat`
- **macOS & Linux**: `install-qt-unix.sh`（自动检测系统）

### 方法二：手动安装

#### 1. 下载在线安装器

从 [阿里云Qt镜像](https://mirrors.aliyun.com/qt/official_releases/online_installers/) 下载对应平台的在线安装器：

- **Windows**: `qt-unified-windows-x86-online.exe` 或 `qt-unified-windows-x64-online.exe`
- **macOS**: `qt-unified-mac-x64-online.dmg`
- **Linux**: `qt-unified-linux-x64-online.run`

#### 2. 配置镜像源

有两种方式配置阿里云镜像：

##### 方式一：命令行参数（推荐）

新版本安装器（4.0.1-1后）支持 `--mirror` 参数：

**Windows:**
```cmd
qt-unified-windows-x64-online.exe --mirror https://mirrors.aliyun.com/qt
```

**macOS:**
```bash
./qt-unified-mac-x64-online.dmg --mirror https://mirrors.aliyun.com/qt
```

**Linux:**
```bash
./qt-unified-linux-x64-online.run --mirror https://mirrors.aliyun.com/qt
```

##### 方式二：设置界面配置

1. 启动安装器
2. 在设置中禁用默认源
3. 添加新源地址（根据平台选择对应地址）：
   - **Windows**: `https://mirrors.aliyun.com/qt/online/qtsdkrepository/windows_x86/root/qt/`
   - **macOS**: `https://mirrors.aliyun.com/qt/online/qtsdkrepository/mac_x64/root/qt/`
   - **Linux**: `https://mirrors.aliyun.com/qt/online/qtsdkrepository/linux_x64/root/qt/`

## 版本推荐

- **Qt 6.x**: 最新版本，支持现代C++特性和最新平台
- **Qt 5.15 LTS**: 长期支持版本，稳定可靠
- **Qt 5.12 LTS**: 适用于需要向后兼容的项目

## 安装组件建议

推荐安装的组件：

### 核心组件
- Qt Core
- Qt GUI
- Qt Widgets
- Qt Network

### 开发工具
- Qt Creator IDE
- Qt Designer
- Qt Assistant
- CMake
- Ninja

### 平台特定组件

#### Windows
- MSVC 2019/2022 编译器
- Qt for Windows

#### macOS
- Xcode 工具链
- Qt for macOS

#### Linux
- GCC 编译器
- Qt for Linux

## 故障排除

### 常见问题

1. **下载速度慢**
   - 确认已正确配置镜像地址
   - 检查网络连接

2. **安装器无法启动**
   - 确认有足够的磁盘空间
   - 以管理员权限运行（Windows）

3. **编译器找不到**
   - 检查PATH环境变量
   - 重新安装对应的编译器工具链

### 获取帮助

- [Qt官方文档](https://doc.qt.io/)
- [Qt社区论坛](https://forum.qt.io/)
- [阿里云镜像使用说明](https://mirrors.aliyun.com/help/qt)

## 许可证

Qt有多种许可证选项：
- **GPL**: 适用于开源项目
- **LGPL**: 适用于闭源项目（有限制）
- **商业许可**: 适用于商业闭源项目

请根据您的项目需求选择合适的许可证。

## 更新日志

- 2024-11-28: 创建初始文档，支持Windows/macOS/Linux平台
