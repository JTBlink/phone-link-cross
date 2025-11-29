# Qt + libimobiledevice 集成示例

> 📱 完整的 Qt 应用程序，演示如何使用 libimobiledevice 管理 iOS 设备

## 项目概述

phone-linkc 项目展示了如何在 Qt 应用程序中集成 libimobiledevice，实现对 iOS 设备的发现、连接和信息获取功能。

### 功能特性

- ✅ **设备自动发现**: 实时检测连接的 iOS 设备
- ✅ **设备连接管理**: 建立和管理与设备的连接
- ✅ **设备信息获取**: 获取详细的设备硬件和软件信息
- ✅ **友好用户界面**: 直观的 Qt 图形界面
- ✅ **模拟模式支持**: 在没有 libimobiledevice 时提供模拟功能
- ✅ **跨平台支持**: Windows、macOS、Linux

## 项目结构

```
phone-linkc/
├── CMakeLists.txt          # CMake 构建配置（包含 libimobiledevice 检测）
├── main.cpp                # 应用程序入口
├── mainwindow.h/.cpp/.ui   # 主窗口界面
├── devicemanager.h/.cpp    # 设备管理器（核心业务逻辑）
├── deviceinfo.h/.cpp       # 设备信息管理
└── phone-linkc_zh_CN.ts    # 中文翻译文件
```

## 核心类设计

### 1. DeviceManager - 设备管理器

```cpp
class DeviceManager : public QObject
{
    Q_OBJECT

public:
    // 设备管理
    void startDiscovery();                      // 开始设备发现
    void stopDiscovery();                       // 停止设备发现
    bool connectToDevice(const QString &udid);  // 连接到指定设备
    void disconnectFromDevice();                // 断开设备连接
    
    // 状态查询
    QStringList getConnectedDevices() const;    // 获取已连接设备列表
    QString getCurrentDevice() const;           // 获取当前连接的设备
    bool isConnected() const;                   // 检查是否已连接

signals:
    void deviceFound(const QString &udid, const QString &name);
    void deviceLost(const QString &udid);
    void deviceConnected(const QString &udid);
    void deviceDisconnected();
    void errorOccurred(const QString &error);
};
```

**关键特性**:
- 使用定时器自动检测设备连接状态
- 支持真实 libimobiledevice 和模拟模式
- RAII 风格的资源管理
- 信号槽机制实现异步通信

### 2. DeviceInfoManager - 设备信息管理

```cpp
struct DeviceInfo {
    QString udid;              // 设备唯一标识符
    QString name;              // 设备名称
    QString model;             // 设备型号
    QString productVersion;    // iOS 版本
    QString buildVersion;      // 构建版本
    QString serialNumber;      // 序列号
    QString deviceClass;       // 设备类别
    QString productType;       // 产品类型
    qint64 totalCapacity;      // 总存储容量
    qint64 availableCapacity;  // 可用存储容量
    QString wifiAddress;       // WiFi MAC 地址
    QString activationState;   // 激活状态
};

class DeviceInfoManager : public QObject
{
    Q_OBJECT
    
public:
    DeviceInfo getDeviceInfo(const QString &udid);
    QVariantMap getDetailedInfo(const QString &udid);
};
```

### 3. MainWindow - 主界面

**界面布局**:
- 左侧：设备列表 + 操作按钮
- 右侧：设备详细信息显示
- 底部：状态栏显示当前状态

**核心功能**:
- 实时显示发现的设备
- 支持设备连接/断开操作
- 详细的设备信息展示
- 错误处理和用户提示

## 构建说明

### 前置要求

1. **Qt 6.5+**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install qt6-base-dev qt6-tools-dev
   
   # macOS (Homebrew)
   brew install qt6
   
   # Windows
   # 下载 Qt Online Installer
   ```

2. **libimobiledevice (可选)**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libimobiledevice-dev libplist-dev libusbmuxd-dev
   
   # macOS (Homebrew)
   brew install libimobiledevice libplist libusbmuxd
   
   # Windows
   # 使用 vcpkg 或预编译二进制文件
   vcpkg install libimobiledevice libplist libusbmuxd
   ```

### 构建步骤

#### 使用构建脚本 (推荐)

```bash
# Linux/macOS
./build.sh

# Windows
build.bat
```

#### 手动构建

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. 编译
cmake --build . --config Release

# 4. 运行
./phone-linkc  # Linux/macOS
# 或
Release/phone-linkc.exe  # Windows
```

### CMake 配置特性

```cmake
# 自动检测 libimobiledevice
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(IMOBILEDEVICE libimobiledevice-1.0)
    pkg_check_modules(PLIST libplist-2.0)
    pkg_check_modules(USBMUXD libusbmuxd-2.0)
endif()

# 条件编译支持
if(IMOBILEDEVICE_FOUND AND PLIST_FOUND)
    target_compile_definitions(phone-linkc PRIVATE HAVE_LIBIMOBILEDEVICE)
    # 链接库...
else()
    message(WARNING "libimobiledevice not found. iOS device features will be disabled.")
endif()
```

## 使用示例

### 1. 设备发现和连接

```cpp
// 创建设备管理器
DeviceManager *manager = new DeviceManager(this);

// 连接信号
connect(manager, &DeviceManager::deviceFound, [](const QString &udid, const QString &name) {
    qDebug() << "发现设备:" << name << "UDID:" << udid;
});

connect(manager, &DeviceManager::deviceConnected, [](const QString &udid) {
    qDebug() << "已连接到设备:" << udid;
});

// 开始设备发现
manager->startDiscovery();

// 连接到设备
manager->connectToDevice("your-device-udid-here");
```

### 2. 获取设备信息

```cpp
// 创建信息管理器
DeviceInfoManager *infoManager = new DeviceInfoManager(this);

// 获取设备信息
DeviceInfo info = infoManager->getDeviceInfo("device-udid");

// 显示信息
qDebug() << "设备名称:" << info.name;
qDebug() << "iOS 版本:" << info.productVersion;
qDebug() << "总容量:" << info.totalCapacity / (1024*1024*1024) << "GB";
```

### 3. 错误处理

```cpp
connect(manager, &DeviceManager::errorOccurred, [](const QString &error) {
    QMessageBox::warning(nullptr, "设备错误", 
        QString("设备操作失败: %1").arg(error));
});
```

## 运行效果

### 有 libimobiledevice 支持时
- 自动检测真实的 iOS 设备
- 获取完整的设备硬件和软件信息
- 支持设备连接状态管理
- 实时响应设备插拔事件

### 模拟模式（无 libimobiledevice）
- 模拟设备发现过程
- 提供示例设备信息
- 完整的 UI 交互流程
- 用于开发和测试目的

### 界面截图说明

```
┌─────────────────────────────────────────────────────────────┐
│ iOS 设备管理器 - libimobiledevice 示例                        │
├─────────────────────────────────────────────────────────────┤
│ 设备列表        │ 设备信息                                    │
│ ┌─────────────┐  │ ========== iOS 设备信息 ==========      │
│ │ iPhone 15   │  │                                        │
│ │ 00000000... │  │ 设备名称: iPhone 15                     │
│ └─────────────┘  │ UDID: 00000000-0000000000000000        │
│                  │ 型号: A2482                           │
│ [连接设备]        │ 系统版本: 17.1.1 (21B91)               │
│ [刷新列表]        │ 序列号: F2LMQDXQ0D6Y                   │
│ [获取信息]        │ 设备类型: iPhone                        │
│                  │ 产品类型: iPhone15,2                   │
│                  │ 总容量: 128 GB                         │
│                  │ 可用容量: 64 GB                         │
│                  │ WiFi地址: aa:bb:cc:dd:ee:ff            │
│                  │ 激活状态: Activated                     │
│                  │                                        │
│                  │ ========== libimobiledevice 状态 =====│
│                  │ 连接状态: 已连接                        │
│                  │ 支持库: libimobiledevice (真实设备支持)  │
├─────────────────────────────────────────────────────────────┤
│ 状态: 已连接到设备: 00000000-0000000000000000               │
└─────────────────────────────────────────────────────────────┘
```

## 扩展功能建议

### 1. 应用管理

```cpp
class AppManager : public QObject
{
    Q_OBJECT
    
public:
    QStringList getInstalledApps(const QString &udid);
    bool installApp(const QString &udid, const QString &ipaPath);
    bool uninstallApp(const QString &udid, const QString &bundleId);
    
signals:
    void installProgress(int percentage);
    void installCompleted(bool success);
};
```

### 2. 文件管理

```cpp
class FileManager : public QObject
{
    Q_OBJECT
    
public:
    QStringList listFiles(const QString &udid, const QString &path);
    bool uploadFile(const QString &udid, const QString &localPath, const QString &remotePath);
    bool downloadFile(const QString &udid, const QString &remotePath, const QString &localPath);
    
signals:
    void transferProgress(qint64 bytesTransferred, qint64 totalBytes);
    void transferCompleted(bool success);
};
```

### 3. 备份功能

```cpp
class BackupManager : public QObject
{
    Q_OBJECT
    
public:
    bool createBackup(const QString &udid, const QString &backupPath);
    bool restoreBackup(const QString &udid, const QString &backupPath);
    QStringList getBackupList();
    
signals:
    void backupProgress(int percentage);
    void backupCompleted(bool success);
};
```

## 故障排除

### 常见问题

1. **libimobiledevice 未找到**
   ```
   解决方案: 安装 libimobiledevice 开发包，或使用模拟模式
   ```

2. **设备未被发现**
   ```
   检查项:
   - 设备是否已信任计算机
   - USB 驱动是否正确安装
   - 设备是否处于正常状态（非 DFU 模式）
   ```

3. **编译错误**
   ```
   检查项:
   - Qt 版本是否为 6.5+
   - CMake 版本是否满足要求
   - 依赖库是否正确安装
   ```

### 调试技巧

1. **启用详细日志**
   ```cpp
   // 在 main.cpp 中添加
   QLoggingCategory::setFilterRules("*.debug=true");
   ```

2. **检查 libimobiledevice 状态**
   ```bash
   # 命令行工具测试
   idevice_id -l        # 列出设备
   ideviceinfo         # 获取设备信息
   ```

3. **网络连接测试**
   ```cpp
   // 检查 usbmuxd 服务状态
   #ifdef HAVE_LIBIMOBILEDEVICE
   if (usbmuxd_get_device_list(&device_list, &count) < 0) {
       qWarning() << "usbmuxd 服务未运行";
   }
   #endif
   ```

## 总结

这个示例项目展示了如何在 Qt 应用程序中成功集成 libimobiledevice，提供了：

- 🏗️ **完整的项目结构**: 模块化设计，易于扩展
- 🔧 **灵活的构建系统**: 支持有无 libimobiledevice 的构建
- 🖥️ **用户友好界面**: 直观的设备管理体验
- 📚 **详细的文档**: 完整的使用说明和扩展指南
- 🔍 **调试支持**: 完整的日志和错误处理机制

通过这个示例，开发者可以快速上手 libimobiledevice 的使用，并在此基础上开发更复杂的 iOS 设备管理应用。
