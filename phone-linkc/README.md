# iOS 设备管理器 (phone-linkc)

> 基于 Qt 6 和 libimobiledevice 的跨平台 iOS 设备管理应用

## 简介

这是一个完整的 Qt 6 应用程序，展示如何使用 libimobiledevice 库来管理 iOS 设备。该项目采用**动态库加载技术**，支持在有或无 libimobiledevice 环境下运行，提供设备发现、连接管理、设备信息获取以及**照片管理**等核心功能。

### 主要特性

- ✅ **实时设备发现**: 自动检测连接的 iOS 设备（支持事件订阅）
- ✅ **设备连接管理**: 建立和管理设备连接
- ✅ **详细设备信息**: 获取完整的硬件和软件信息
- ✅ **照片管理**:
    - 📁 **相册浏览**: 树形结构查看设备上的相册（/DCIM）
    - 🖼️ **照片预览**: 异步加载照片缩略图，流畅的滚动体验
    - 💾 **照片导出**: 支持导出照片到本地（开发中）
- ✅ **用户友好界面**: 现代化 Qt 图形界面，自适应 FlowLayout 布局
- ✅ **动态库加载**: Windows 平台智能加载 DLL，无需静态链接，增强兼容性
- ✅ **模拟模式**: 无需真实设备即可测试（开发友好）
- ✅ **跨平台支持**: Windows、macOS、Linux 完整支持
- ✅ **智能依赖管理**: 自动检测和配置 libimobiledevice

## 技术方案细节

### 核心架构

项目采用分层架构设计：

1.  **UI 层 (`src/ui`)**:
    -   使用 Qt Widgets 构建现代化界面
    -   `PhotoPage`: 负责照片展示，实现了自定义 `FlowLayout` 以适应不同窗口大小
    -   `PhotoThumbnail`: 自定义 Widget 用于显示照片缩略图和选中状态
    -   异步加载机制: 使用 `QTimer` 和队列机制，将耗时的图片读取和解码分散到事件循环中，避免阻塞主线程

2.  **业务逻辑层 (`src/core`)**:
    -   `DeviceManager`: 负责设备的发现和连接管理
    -   `PhotoManager`: 封装了具体的照片操作接口
    -   `DeviceInfoManager`: 负责获取和解析设备信息

3.  **平台适配层 (`src/platform`)**:
    -   `LibimobiledeviceDynamic`: 核心组件，实现了 Windows 下 libimobiledevice 及其依赖库（libplist, libusbmuxd 等）的动态加载
    -   封装了 C 风格的 API 为 C++ 友好的接口

### 功能实现原理

#### 1. 设备发现与事件订阅

项目使用 libimobiledevice 提供的事件订阅机制来实现热插拔检测，而非轮询，资源占用极低。

-   **核心 API**: `idevice_event_subscribe` / `idevice_event_unsubscribe`
-   **实现机制**: `DeviceManager` 注册回调函数接收底层设备事件（ADD/REMOVE），通过 Qt 的信号槽机制 (`deviceFound`, `deviceLost`) 将事件转发到 UI 线程，确保线程安全。

#### 2. 设备连接与动态库加载

项目采用运行时动态加载 (`QLibrary`/`LoadLibrary`) 的方式调用 `libimobiledevice`，而非编译时静态链接。这使得应用在未安装驱动的机器上也能运行（仅功能受限），极大提高了兼容性。

-   **核心类**: `LibimobiledeviceDynamic` (单例模式)
-   **加载流程**:
    1.  检测系统环境变量和预定义路径。
    2.  按顺序加载依赖库：`libplist` -> `libusbmuxd` -> `libimobiledevice`。
    3.  使用 `resolve` 解析符号并映射到函数指针。
-   **连接流程**:
    1.  `idevice_new`: 创建设备句柄。
    2.  `lockdownd_client_new_with_handshake`: 建立受信连接（需用户在手机上点击“信任”）。
    3.  `lockdownd_start_service`: 启动特定服务（如 `com.apple.afc` 用于文件访问）。

#### 3. 照片管理实现细节

-   **文件系统访问**: 通过 libimobiledevice 的 AFC (Apple File Conduit) 服务访问 iOS 设备的 `/DCIM` 目录。
-   **相册解析**: 递归扫描 `/DCIM` 下的文件夹（如 `100APPLE`），将其映射为相册结构。
-   **缩略图生成**:
    -   目前通过读取原始照片文件 (`readPhotoData`) 并使用 `QImage` 进行解码和缩放生成缩略图。
    -   **异步队列**: 实现了 `startThumbnailLoading` 和 `loadNextThumbnail`，防止一次性加载大量图片导致界面卡死。
-   **HEIC 支持**: 依赖 Qt 的 Imageformats 插件。如果系统缺失 HEIC 解码器，缩略图可能无法正常显示（显示为灰色占位符）。

### 已知问题与限制

1.  **HEIC 格式支持**: 
    -   iPhone 拍摄的照片默认为 HEIC 格式。
    -   Windows 上的 Qt 默认可能不支持 HEIC 解码，导致缩略图显示失败。
    -   **解决方案**: 需要安装 Qt 的 HEIF 插件或将 HEIF 解码库集成到项目中。
2.  **大文件读取性能**:
    -   目前生成缩略图是读取整个原始图片文件。对于高清照片或视频，即使在异步队列中读取，也可能产生轻微的 IO 延迟。
    -   **优化方向**: 尝试读取文件头部元数据中的缩略图（如果存在），或集成更高效的缩略图生成库。
3.  **视频预览**: 
    -   目前视频文件仅显示占位符，不支持预览播放。
4.  **相册名称**: 
    -   显示的相册名称为文件系统目录名（如 `100APPLE`），而非 iOS 相册应用中显示的逻辑相册名（如“最近项目”、“收藏”）。这是由于 AFC 协议限制，无法直接访问 Photos 数据库。

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
│ 设备列表        │ 📷 图库 (所有照片)                        │
│ ┌─────────────┐  │                                        │
│ │ iPhone 15   │  │ [照片1] [照片2] [照片3] [照片4]        │
│ │ 00000000... │  │                                        │
│ └─────────────┘  │ [照片5] [照片6] ...                    │
│                  │                                        │
│ 📁 我的相簿      │                                        │
│  ├─ 100APPLE     │                                        │
│  └─ 101APPLE     │                                        │
│                  │ 状态: 正在加载缩略图...                    │
├─────────────────────────────────────────────────────────────┤
│ 状态栏: 找到 2155 张照片                                     │
└─────────────────────────────────────────────────────────────┘
```

### 基本操作

1. **设备发现**: 应用启动后自动开始扫描 iOS 设备
2. **选择设备**: 在左侧列表中点击设备名称
3. **连接设备**: 点击"连接设备"按钮建立连接
4. **浏览照片**: 连接成功后，点击左侧相册树查看不同文件夹中的照片
5. **刷新列表**: 点击"刷新"手动同步照片列表

## 项目结构

```
phone-linkc/
├── CMakeLists.txt          # CMake 构建配置
├── build.sh               # Linux/macOS 构建脚本
├── build.bat              # Windows 构建脚本
├── README.md              # 项目说明文档
├── main.cpp               # 应用程序入口
├── src/
│   ├── core/              # 核心业务逻辑
│   │   ├── device/        # 设备管理
│   │   └── photo/         # 照片管理
│   ├── platform/          # 平台相关 (动态库加载)
│   └── ui/                # 用户界面
│       ├── photopage.*    # 照片页面与缩略图实现
│       ├── flowlayout.*   # 流式布局实现
│       └── ...
└── phone-linkc_zh_CN.ts   # 中文本地化
```

## 贡献

欢迎提交 Issue 和 Pull Request！特别是针对 HEIC 解码支持和性能优化的改进。

---

📱 享受 iOS 设备管理的便利！
