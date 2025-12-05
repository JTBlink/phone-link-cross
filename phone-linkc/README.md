# iOS 设备管理器 (phone-linkc)

> 基于 Qt 6 和 libimobiledevice 的跨平台 iOS 设备管理应用

## 简介

这是一个完整的 Qt 6 应用程序，展示如何使用 libimobiledevice 库来管理 iOS 设备。该项目采用**动态库加载技术**，支持在有或无 libimobiledevice 环境下运行，提供设备发现、连接管理和设备信息获取等核心功能。

### 主要特性

- ✅ **实时设备发现**: 自动检测连接的 iOS 设备（支持事件订阅）
- ✅ **设备连接管理**: 建立和管理设备连接
- ✅ **详细设备信息**: 获取完整的硬件和软件信息
- ✅ **用户友好界面**: 现代化 Qt 图形界面
- ✅ **动态库加载**: Windows 平台智能加载 DLL，无需静态链接
- ✅ **模拟模式**: 无需真实设备即可测试（开发友好）
- ✅ **跨平台支持**: Windows、macOS、Linux 完整支持
- ✅ **智能依赖管理**: 自动检测和配置 libimobiledevice

## 快速开始

### 1. 环境准备

**必需依赖:**
- Qt 6.5+
- CMake 3.19+
- C++17 编译器

**可选依赖 (推荐):**
- libimobiledevice (用于真实 iOS 设备支持)
- libplist
- libusbmuxd

### 2. 安装依赖

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

### 3. 编译和运行

#### 使用构建脚本 (推荐)

**Linux/macOS:**
```bash
./build.sh
```

**Windows:**
```cmd
build.bat
```

构建脚本会自动：
- 检测系统中的 Qt 安装
- 查找常见的 Qt 路径（包括 ~/Qt 目录）
- 配置合适的 CMake 参数
- 编译并可选择运行项目

#### 手动编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build . --config Release

# 运行
./phone-linkc          # Linux/macOS
# 或
Release/phone-linkc.exe  # Windows
```

## 使用说明

### 界面布局

```
┌─────────────────────────────────────────────────────────────┐
│ iOS 设备管理器 - libimobiledevice 示例                        │
├─────────────────────────────────────────────────────────────┤
│ 设备列表        │ 设备详细信息                               │
│ ┌─────────────┐  │                                        │
│ │ iPhone 15   │  │ 设备名称: iPhone 15                     │
│ │ 00000000... │  │ iOS版本: 17.1.1                       │
│ └─────────────┘  │ 存储容量: 128 GB                       │
│                  │ ...                                   │
│ [连接设备]        │                                        │
│ [刷新列表]        │                                        │
│ [获取信息]        │                                        │
├─────────────────────────────────────────────────────────────┤
│ 状态栏: 找到 1 台设备                                        │
└─────────────────────────────────────────────────────────────┘
```

### 基本操作

1. **设备发现**: 应用启动后自动开始扫描 iOS 设备
2. **选择设备**: 在左侧列表中点击设备名称
3. **连接设备**: 点击"连接设备"按钮建立连接
4. **获取信息**: 点击"获取信息"按钮查看详细设备信息
5. **刷新列表**: 点击"刷新列表"手动更新设备列表

### 功能说明

#### 真实设备模式 (需要 libimobiledevice)
- 检测真实连接的 iOS 设备
- 获取完整的设备硬件信息
- 支持设备配对和信任管理

#### 模拟模式 (无 libimobiledevice)
- 模拟设备发现过程
- 提供示例设备数据
- 用于开发和界面测试

## 项目结构

```
phone-linkc/
├── CMakeLists.txt          # CMake 构建配置
├── build.sh               # Linux/macOS 构建脚本
├── build.bat              # Windows 构建脚本
├── README.md              # 项目说明文档
├── main.cpp               # 应用程序入口
├── mainwindow.h/.cpp/.ui  # 主窗口实现
├── devicemanager.h/.cpp   # 设备管理器
├── deviceinfo.h/.cpp      # 设备信息管理
└── phone-linkc_zh_CN.ts   # 中文本地化
```

## 核心 API 示例

### 设备管理

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

// 连接到设备
manager->connectToDevice("device-udid");
```

### 设备信息获取

```cpp
#include "deviceinfo.h"

// 创建信息管理器
DeviceInfoManager *infoManager = new DeviceInfoManager(this);

// 获取设备信息
DeviceInfo info = infoManager->getDeviceInfo("device-udid");

qDebug() << "设备名称:" << info.name;
qDebug() << "iOS版本:" << info.productVersion;
qDebug() << "存储容量:" << info.totalCapacity / (1024*1024*1024) << "GB";
```

## 扩展功能

这个项目可以作为基础，扩展出更多功能：

- **应用管理**: 安装/卸载应用
- **文件传输**: 上传/下载文件
- **设备备份**: 创建和恢复备份
- **屏幕镜像**: 实时屏幕显示
- **性能监控**: CPU、内存使用情况

参考文档中的扩展示例了解更多信息。

## 故障排除

### 常见问题

1. **Qt 找不到**
   ```bash
   # 确保 Qt 已正确安装并添加到 PATH
   which qmake  # Linux/macOS
   where qmake  # Windows
   ```

2. **libimobiledevice 找不到**
   ```bash
   # 检查是否安装
   pkg-config --exists libimobiledevice-1.0
   
   # 如果未安装，应用会自动启用模拟模式
   ```

3. **设备无法发现**
   - 确保设备已解锁
   - 信任此计算机
   - 检查 USB 连接
   - 确保 iTunes/3uTools 等工具未占用设备

4. **编译错误**
   - 检查 Qt 版本 (需要 6.5+)
   - 检查 CMake 版本 (需要 3.19+)
   - 确保 C++17 支持

### 调试模式

启用详细日志输出:

```cpp
// 在 main.cpp 中添加
QLoggingCategory::setFilterRules("*.debug=true");
```

## 许可证

本项目基于 MIT 许可证，详见 LICENSE 文件。

## 参考资源

- [libimobiledevice 官网](https://libimobiledevice.org/)
- [Qt 官方文档](https://doc.qt.io/)
- [项目完整文档](../doc/examples/qt-libimobiledevice-example.md)
- [最佳实践指南](../doc/examples/libimobiledevice-best-practices.md)

## 贡献

欢迎提交 Issue 和 Pull Request！

---

📱 享受 iOS 设备管理的便利！
