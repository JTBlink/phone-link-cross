# Qt 快速安装指南

> 🚀 一键安装 Qt 开发环境，使用阿里云镜像加速下载

## 快速开始

### Windows 用户

1. 打开命令提示符（推荐以管理员身份运行）
2. 运行以下命令：

```cmd
curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-windows.bat && install-qt-windows.bat
```

或分步执行：
```cmd
curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-windows.bat
install-qt-windows.bat
```

### macOS & Linux 用户

打开终端，运行：

```bash
curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-unix.sh && chmod +x install-qt-unix.sh && ./install-qt-unix.sh
```

或分步执行：
```bash
# 下载脚本
curl -O https://raw.githubusercontent.com/your-repo/phone-link-cross/main/doc/scripts/install-qt-unix.sh

# 设置可执行权限
chmod +x install-qt-unix.sh

# 运行安装脚本
./install-qt-unix.sh
```

## 安装完成后

### 验证安装

```bash
# 检查 qmake 版本
qmake --version

# 检查 Qt Creator（如果安装了）
qtcreator --version  # Linux
open "/Applications/Qt Creator.app"  # macOS
```

### 创建第一个项目

1. 启动 Qt Creator
2. 选择 "New Project" > "Application" > "Qt Widgets Application"
3. 选择合适的 Qt 版本和编译器
4. 点击 "Run" 按钮编译并运行

## 常见问题

### Q: 脚本提示权限不足怎么办？
**A**: 
- **Windows**: 以管理员身份运行命令提示符
- **macOS/Linux**: 使用 `sudo` 运行脚本或确保有写入权限

### Q: 下载速度很慢怎么办？
**A**: 脚本已经配置了阿里云镜像源，如果仍然很慢：
- 检查网络连接
- 尝试更换网络环境
- 确认防火墙没有阻止下载

### Q: 安装完成后找不到 qmake 命令？
**A**: 
- 重启终端/命令提示符
- 手动添加 Qt bin 目录到 PATH 环境变量
- 运行脚本时选择自动配置环境变量

### Q: 支持哪些系统架构？
**A**: 
- **Windows**: x86, x64
- **macOS**: Intel x64, Apple Silicon (M1/M2)
- **Linux**: x64, ARM64

## 推荐安装组件

### 必选组件
- ✅ Qt Core, GUI, Widgets
- ✅ Qt Creator IDE
- ✅ CMake

### 可选但推荐
- 🔧 Qt Designer
- 📚 Qt Assistant (帮助文档)
- 🔨 平台特定编译器 (MSVC/GCC/Clang)

## 镜像源信息

- **阿里云镜像**: https://mirrors.aliyun.com/qt/
- **自动配置**: 脚本会自动配置镜像源
- **手动配置**: 在安装器中添加 `--mirror https://mirrors.aliyun.com/qt` 参数

## 需要帮助？

- 📖 [完整文档](README.md)
- 🌐 [Qt 官方文档](https://doc.qt.io/)
- 💬 [Qt 社区论坛](https://forum.qt.io/)
- 🔧 [阿里云镜像说明](https://mirrors.aliyun.com/help/qt)

---

> 💡 **提示**: 首次安装建议选择最新的 Qt 6.x LTS 版本，获得最佳的开发体验和长期支持。
