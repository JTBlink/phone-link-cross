# phone-link-cross

[![CMake Multi-Platform Build](https://github.com/JTBlink/phone-link-cross/workflows/CMake%20Multi-Platform%20Build/badge.svg)](https://github.com/JTBlink/phone-link-cross/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Qt Version](https://img.shields.io/badge/Qt-6.6+-blue.svg)](https://www.qt.io)

> 基于 libimobiledevice 和 Apple Mobile Device Support 的跨平台 iOS 设备管理工具

## 项目概述

phone-link-cross 是一个完整的跨平台解决方案，包含：

1. **📱 iOS 设备管理应用 (phone-linkc/)** - 基于 Qt + libimobiledevice 的图形界面应用
2. **📚 详细文档系统 (doc/)** - 完整的 libimobiledevice 能力总览和最佳实践
3. **⚙️ 自动化构建** - GitHub Actions 多平台 CI/CD

## 快速开始

### 🚀 iOS 设备管理应用

```bash
# 克隆项目
git clone https://github.com/JTBlink/phone-link-cross.git
cd phone-link-cross/phone-linkc

# 构建和运行
./build.sh        # Linux/macOS
# 或
build.bat         # Windows
```

### 功能特性

- ✅ **实时设备发现**: 自动检测连接的 iOS 设备
- ✅ **设备连接管理**: 建立和管理设备连接
- ✅ **详细设备信息**: 获取硬件和软件信息
- ✅ **用户友好界面**: Qt 图形界面
- ✅ **模拟模式**: 无需真实设备即可测试
- ✅ **跨平台支持**: Windows、macOS、Linux

## 项目结构

```
phone-link-cross/
├── .github/workflows/           # GitHub Actions CI/CD
├── phone-linkc/                # Qt + libimobiledevice 应用
│   ├── build.sh/.bat          # 构建脚本
│   ├── CMakeLists.txt         # CMake 配置
│   ├── devicemanager.*        # 设备管理核心
│   ├── deviceinfo.*          # 设备信息管理
│   ├── mainwindow.*           # Qt 图形界面
│   └── README.md              # 应用使用指南
├── doc/                       # 文档系统
│   ├── libimobiledevice-overview.md      # 功能总览
│   └── examples/              # 代码示例和最佳实践
└── README.md                  # 项目主文档 (本文件)
```

## 文档系统

### 📚 libimobiledevice 完整指南

- **[功能总览](doc/libimobiledevice-overview.md)** - libimobiledevice 完整能力介绍
- **[代码示例](doc/examples/libimobiledevice-examples.md)** - 从基础到高级的实用代码
- **[最佳实践](doc/examples/libimobiledevice-best-practices.md)** - 错误处理、内存管理、性能优化
- **[Qt 集成指南](doc/examples/qt-libimobiledevice-example.md)** - 完整的 Qt 项目集成示例

### 🛠️ Qt 环境配置

- **[完整安装指南](doc/README.md)** - 详细的 Qt 安装文档和配置说明
- **[快速开始](doc/QUICKSTART.md)** - 一键安装 Qt 开发环境
- **[配置示例](doc/examples/qt-mirror-config.md)** - 各种环境下的配置示例

## 环境要求

### 必需依赖

- **Qt 6.6+** - 图形界面框架
- **CMake 3.19+** - 构建系统
- **C++17** 编译器

### 可选依赖 (推荐)

- **libimobiledevice** - 真实 iOS 设备支持
- **libplist** - 属性列表处理
- **libusbmuxd** - USB 多路复用支持

## 安装依赖

### 自动化安装脚本

#### Windows
```cmd
curl -O https://raw.githubusercontent.com/JTBlink/phone-link-cross/main/doc/scripts/install-qt-windows.bat
install-qt-windows.bat
```

#### macOS & Linux
```bash
curl -O https://raw.githubusercontent.com/JTBlink/phone-link-cross/main/doc/scripts/install-qt-unix.sh
chmod +x install-qt-unix.sh
./install-qt-unix.sh
```

### 手动安装

#### macOS (Homebrew)
```bash
# 必需组件
brew install qt6 cmake

# 可选 - iOS 设备支持
brew install libimobiledevice libplist libusbmuxd
```

#### Ubuntu/Debian
```bash
# 必需组件
sudo apt-get install qt6-base-dev qt6-tools-dev cmake

# 可选 - iOS 设备支持
sudo apt-get install libimobiledevice-dev libplist-dev libusbmuxd-dev
```

#### Windows
```powershell
# 使用 vcpkg (推荐)
vcpkg install qt6 cmake

# 可选 - iOS 设备支持
vcpkg install libimobiledevice libplist libusbmuxd
```

## CI/CD 构建状态

| 平台 | 编译器 | 状态 |
|------|--------|------|
| Ubuntu | GCC | [![Ubuntu GCC](https://img.shields.io/badge/Ubuntu-GCC-success)](../../actions) |
| Ubuntu | Clang | [![Ubuntu Clang](https://img.shields.io/badge/Ubuntu-Clang-success)](../../actions) |
| macOS | Clang | [![macOS Clang](https://img.shields.io/badge/macOS-Clang-success)](../../actions) |
| Windows | MSVC | [![Windows MSVC](https://img.shields.io/badge/Windows-MSVC-success)](../../actions) |

### 自动构建功能

- ✅ 多平台并行构建 (Ubuntu, macOS, Windows)
- ✅ 多编译器支持 (GCC, Clang, MSVC)
- ✅ 依赖自动安装 (Qt, libimobiledevice)
- ✅ 构建产物上传
- ✅ 自动发布 (Git 标签触发)

## 支持的系统

### Windows
- Windows 10/11 (x86/x64)
- Windows Server 2019/2022
- 支持 Visual Studio 2019+ 和 MinGW

### macOS
- macOS 10.15+ (Intel)
- macOS 11.0+ (Apple Silicon M1/M2)
- 支持 Xcode Command Line Tools

### Linux
- Ubuntu 18.04+ / Debian 10+
- CentOS/RHEL 7+ / Fedora 30+
- openSUSE Leap 15+ / Arch Linux
- 支持 GCC 8+ 和 Clang 10+

## 使用示例

### 基本设备管理

```cpp
#include "devicemanager.h"

// 创建设备管理器
DeviceManager *manager = new DeviceManager(this);

// 监听设备事件
connect(manager, &DeviceManager::deviceFound, 
        [](const QString &udid, const QString &name) {
    qDebug() << "发现设备:" << name;
});

// 开始设备发现
manager->startDiscovery();
```

### 获取设备信息

```cpp
#include "deviceinfo.h"

DeviceInfoManager *infoManager = new DeviceInfoManager(this);
DeviceInfo info = infoManager->getDeviceInfo("device-udid");

qDebug() << "设备名称:" << info.name;
qDebug() << "iOS版本:" << info.productVersion;
```

## 发布版本

### 下载预编译版本

访问 [Releases](../../releases) 页面下载适合您平台的预编译版本：

- **Linux**: `phone-linkc` (需要安装 Qt 6.6+ 和 libimobiledevice)
- **macOS**: `phone-linkc.app` (需要安装 Qt 6.6+ 和 libimobiledevice)  
- **Windows**: `phone-linkc.exe` (需要安装 Qt 6.6+，模拟模式)

### 创建发布版本

```bash
# 创建并推送标签来触发自动发布
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

## 贡献指南

### 开发环境设置

1. Fork 本项目
2. 克隆您的 fork: `git clone https://github.com/JTBlink/phone-link-cross.git`
3. 安装依赖 (见上方安装说明)
4. 构建项目: `cd phone-linkc && ./build.sh`

### 提交流程

1. 创建功能分支: `git checkout -b feature/amazing-feature`
2. 提交更改: `git commit -m 'Add amazing feature'`
3. 推送分支: `git push origin feature/amazing-feature`
4. 创建 Pull Request

### 代码规范

- 遵循 Qt 代码风格
- 添加适当的注释和文档
- 确保所有平台构建通过
- 添加必要的测试

## 故障排除

### 常见问题

1. **Qt 找不到**: 确保 Qt 已正确安装并添加到 PATH
2. **libimobiledevice 找不到**: 安装相应开发包，或使用模拟模式
3. **编译错误**: 检查 Qt 版本 (需要 6.6+) 和 CMake 版本 (需要 3.19+)

### 获取帮助

- 📋 [提交 Issue](../../issues) - 报告 Bug 或请求功能
- 💬 [讨论区](../../discussions) - 提问和讨论
- 📖 [文档](doc/) - 查看详细文档

## 许可证

本项目基于 [MIT 许可证](LICENSE) 开源。

## 致谢

- [libimobiledevice](https://libimobiledevice.org/) - iOS 设备通信库
- [Qt](https://www.qt.io/) - 跨平台图形界面框架
- 所有贡献者和用户的支持

---

<p align="center">
  <strong>📱 享受 iOS 设备管理的便利！</strong>
</p>

<p align="center">
  ⭐ 如果这个项目对您有帮助，请给它一个 Star！
</p>
