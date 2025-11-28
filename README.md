# phone-link-cross

A cross-platform phone link application.

## 项目文档

### Qt 镜像安装指南

本项目提供了完整的Qt开发环境安装解决方案，使用阿里云镜像加速下载：

- 📖 **[完整安装指南](doc/README.md)** - 详细的Qt安装文档和配置说明
- 🚀 **[快速开始](doc/QUICKSTART.md)** - 一键安装Qt开发环境
- ⚙️ **[配置示例](doc/examples/qt-mirror-config.md)** - 各种环境下的配置示例

### 自动化安装脚本

支持Windows、macOS、Linux三大平台：

| 平台 | 脚本文件 | 说明 |
|------|----------|------|
| Windows | [install-qt-windows.bat](doc/scripts/install-qt-windows.bat) | Windows批处理脚本 |
| macOS & Linux | [install-qt-unix.sh](doc/scripts/install-qt-unix.sh) | Unix通用Shell脚本 |

### 快速安装

#### Windows
```cmd
curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-windows.bat
install-qt-windows.bat
```

#### macOS & Linux
```bash
curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-unix.sh
chmod +x install-qt-unix.sh
./install-qt-unix.sh
```

## 功能特性

- ✅ 跨平台支持 (Windows, macOS, Linux)
- ✅ 自动检测系统架构 (x86, x64, ARM64)
- ✅ 阿里云镜像加速下载
- ✅ 自动配置环境变量
- ✅ 支持多种Linux发行版
- ✅ 完整的错误处理和日志记录

## 支持的系统

### Windows
- Windows 10/11 (x86/x64)
- Windows Server 2019/2022

### macOS
- macOS 10.15+ (Intel)
- macOS 11.0+ (Apple Silicon M1/M2)

### Linux
- Ubuntu 18.04+
- CentOS/RHEL 7+
- Fedora 30+
- openSUSE Leap 15+
- Arch Linux
- Debian 10+

## 许可证

本项目基于 MIT 许可证开源。详见 [LICENSE](LICENSE) 文件。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 联系我们

如果您在使用过程中遇到问题，请通过以下方式联系我们：

- 📋 [提交Issue](../../issues)
- 💬 [讨论区](../../discussions)

---

> 💡 **提示**: 首次使用建议先阅读 [快速开始指南](doc/QUICKSTART.md)
