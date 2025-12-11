# libimobiledevice API接口明细说明

> 📚 **完整开发参考** - phone-linkc项目专用libimobiledevice API文档

## 目录

- [概述](#概述)
- [目录结构](#目录结构)
- [版本兼容性](#版本兼容性)
- [动态库加载机制](#动态库加载机制)
  - [LibimobiledeviceDynamic类](#libimobiledevicedynamic类)
  - [使用动态加载的最佳实践](#使用动态加载的最佳实践)
  - [库文件搜索路径](#库文件搜索路径)
  - [部署优势](#部署优势)
  - [动态加载实现细节](#动态加载实现细节)
- [核心API模块](#核心api模块)
  - [1. 设备管理 API (libimobiledevice)](#1-设备管理-api-libimobiledevice)
    - [1.1 设备枚举与连接](#11-设备枚举与连接)
    - [1.2 设备事件监听](#12-设备事件监听)
    - [1.3 设备信息获取](#13-设备信息获取)
  - [2. Lockdown服务 API (lockdownd)](#2-lockdown服务-api-lockdownd)
    - [2.1 客户端连接](#21-客户端连接)
    - [2.2 设备属性操作](#22-设备属性操作)
    - [2.3 服务管理](#23-服务管理)
  - [3. 屏幕截图 API (screenshotr)](#3-屏幕截图-api-screenshotr)
    - [3.1 截图服务](#31-截图服务)
    - [3.2 屏幕镜像实现](#32-屏幕镜像实现)
  - [4. 应用安装 API (installation_proxy)](#4-应用安装-api-installation_proxy)
    - [4.1 应用管理服务](#41-应用管理服务)
    - [4.2 应用安装与卸载](#42-应用安装与卸载)
    - [4.3 应用信息获取](#43-应用信息获取)
  - [5. 文件传输 API (afc)](#5-文件传输-api-afc)
    - [5.1 文件系统访问](#51-文件系统访问)
    - [5.2 文件操作进阶](#52-文件操作进阶)
    - [5.3 应用沙箱访问](#53-应用沙箱访问)
  - [6. 系统日志 API (syslog_relay)](#6-系统日志-api-syslog_relay)
    - [6.1 日志监控](#61-日志监控)
    - [6.2 日志过滤与分析](#62-日志过滤与分析)
  - [7. 移动备份 API (mobilebackup2)](#7-移动备份-api-mobilebackup2)
    - [7.1 备份服务](#71-备份服务)
    - [7.2 备份操作示例](#72-备份操作示例)
  - [8. 通知代理 API (notification_proxy)](#8-通知代理-api-notification_proxy)
    - [8.1 通知服务](#81-通知服务)
    - [8.2 通知事件处理](#82-通知事件处理)
- [属性列表 (plist) API](#属性列表-plist-api)
  - [plist_t 数据类型操作](#plist_t-数据类型操作)
  - [高级plist操作](#高级plist操作)
- [错误处理](#错误处理)
  - [错误代码定义](#错误代码定义)
  - [错误处理最佳实践](#错误处理最佳实践)
  - [故障排除指南](#故障排除指南)
- [线程安全注意事项](#线程安全注意事项)
  - [API线程安全性](#api线程安全性)
  - [最佳实践](#最佳实践)
- [性能优化建议](#性能优化建议)
  - [1. 连接复用](#1-连接复用)
  - [2. 异步操作](#2-异步操作)
  - [3. 内存管理](#3-内存管理)
  - [4. 批量操作](#4-批量操作)
- [高级API模块](#高级api模块)
  - [7. 移动备份 API (mobilebackup2)](#7-移动备份-api-mobilebackup2)
  - [8. 春天板服务 API (springboard)](#8-春天板服务-api-springboard)
  - [9. 诊断中继 API (diagnostics_relay)](#9-诊断中继-api-diagnostics_relay)
  - [10. 通知代理 API (notification_proxy)](#10-通知代理-api-notification_proxy)
- [phone-linkc项目集成](#phone-linkc项目集成)
  - [项目结构中的使用模式](#项目结构中的使用模式)
- [调试和诊断](#调试和诊断)
  - [启用调试输出](#启用调试输出)
  - [常见问题诊断](#常见问题诊断)
  - [性能监控和分析](#性能监控和分析)
- [实用示例集合](#实用示例集合)
  - [设备管理完整示例](#设备管理完整示例)
  - [应用管理完整示例](#应用管理完整示例)
  - [文件传输完整示例](#文件传输完整示例)

## 概述

本文档详细说明了libimobiledevice库的核心API接口，包括函数原型、参数说明、返回值和使用示例。该文档专门针对phone-linkc项目进行优化，提供实际项目中的最佳实践。

> 💡 **提示**: phone-linkc项目采用动态库加载方式，无需在编译时链接静态库，提高了部署的灵活性和兼容性。

## 目录结构

```
libimobiledevice/
├── libimobiledevice-1.0.dll           # 核心库文件
├── plist.dll                   # 属性列表处理库
├── usbmuxd.dll                # USB复用守护进程库
├── idevice_id.exe             # 设备ID查询工具
├── ideviceinfo.exe            # 设备信息查询工具
├── idevicescreenshot.exe      # 屏幕截图工具
├── ideviceinstaller.exe       # 应用安装工具
├── idevicesyslog.exe          # 系统日志工具
├── usbmuxd.exe                # USB复用守护进程
├── doc/
│   ├── README.md              # 基础说明文档
│   ├── API-REFERENCE.md       # API接口明细（本文档）
│   ├── FUNCTION-GUIDE.md      # 功能使用指南
│   └── EXAMPLES.md            # 使用示例集合
├── include/
│   ├── libimobiledevice/      # 核心头文件
│   └── plist/                 # plist处理头文件
└── [其他依赖库文件...]
```

## 版本信息

- **libimobiledevice版本**: v1.4.0+
- **libplist版本**: v2.7.0+
- **支持的iOS版本**: iOS 7.0 - iOS 18.x
- **平台支持**: Windows 10/11, macOS 10.12+, Linux
- **编译器要求**: 支持C11标准的编译器
- **Qt版本**: Qt 5.15+ 或 Qt 6.2+（用于phone-linkc项目）

## libimobiledevice v1.4.0 核心功能

### 1. 事件处理 API

**v1.4.0 提供基于上下文的事件订阅机制，支持更好的资源管理和线程安全性。**

#### API 函数
```c
// 订阅设备事件
idevice_error_t idevice_events_subscribe(idevice_subscription_context_t *context,
                                          idevice_event_cb_t callback,
                                          void *user_data);

// 取消订阅设备事件
idevice_error_t idevice_events_unsubscribe(idevice_subscription_context_t context);
```

#### 功能特性
- ✅ 支持多个独立的事件订阅
- ✅ 明确的上下文管理，避免资源泄漏
- ✅ 更好的线程安全性
- ✅ 更灵活的事件处理机制
- ✅ 支持 USB 和网络设备事件

#### 使用示例

```cpp
// 使用上下文管理订阅
idevice_subscription_context_t subscription_ctx = nullptr;

void deviceEventCallback(const idevice_event_t *event, void *user_data) {
    // 处理事件...
    // 可以通过 event->conn_type 判断连接类型（USB 或网络）
    if (event->event == IDEVICE_DEVICE_ADD) {
        qDebug() << "设备连接:" << event->udid
                 << "连接类型:" << (event->conn_type == CONNECTION_USBMUXD ? "USB" : "网络");
    } else if (event->event == IDEVICE_DEVICE_REMOVE) {
        qDebug() << "设备断开:" << event->udid;
    }
}

// 订阅事件（可以有多个独立订阅）
if (idevice_events_subscribe(&subscription_ctx, deviceEventCallback, nullptr) == IDEVICE_E_SUCCESS) {
    qDebug() << "事件订阅成功";
}

// 取消订阅（明确的上下文管理）
if (subscription_ctx) {
    idevice_events_unsubscribe(subscription_ctx);
    subscription_ctx = nullptr;
}
```

#### phone-linkc 项目实现

```cpp
// 在 DeviceManager 类中
class DeviceManager {
private:
    idevice_subscription_context_t m_subscriptionContext;
    
public:
    bool startEventSubscription() {
        LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
        
        // 使用 v1.4.0+ API
        idevice_error_t ret = lib.idevice_events_subscribe(
            &m_subscriptionContext,
            deviceEventCallback,
            this
        );
        
        return (ret == IDEVICE_E_SUCCESS);
    }
    
    void stopEventSubscription() {
        if (m_subscriptionContext) {
            LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
            lib.idevice_events_unsubscribe(m_subscriptionContext);
            m_subscriptionContext = nullptr;
        }
    }
};
```

#### 2. 网络设备支持增强

**v1.4.0 大幅改进了对 WiFi 连接 iOS 设备的支持。**

##### 新增 API

```c
// 获取扩展设备列表（包含连接类型信息）
idevice_error_t idevice_get_device_list_extended(idevice_info_t **devices, int *count);
idevice_error_t idevice_device_list_extended_free(idevice_info_t *devices);

// 使用选项创建设备连接
idevice_error_t idevice_new_with_options(idevice_t *device,
                                         const char *udid,
                                         enum idevice_options options);
```

##### 设备信息结构

```c
struct idevice_info {
    char *udid;                              // 设备 UDID
    enum idevice_connection_type conn_type;  // 连接类型
    void* conn_data;                         // 连接特定数据
};

enum idevice_connection_type {
    CONNECTION_USBMUXD = 1,  // USB 连接
    CONNECTION_NETWORK       // 网络连接
};

enum idevice_options {
    IDEVICE_LOOKUP_USBMUX = 1 << 1,          // 查找 USB 设备
    IDEVICE_LOOKUP_NETWORK = 1 << 2,         // 查找网络设备
    IDEVICE_LOOKUP_PREFER_NETWORK = 1 << 3   // 优先使用网络连接
};
```

##### 使用示例

```cpp
// 获取所有设备（包括网络设备）
idevice_info_t *devices = nullptr;
int count = 0;

if (idevice_get_device_list_extended(&devices, &count) == IDEVICE_E_SUCCESS) {
    for (int i = 0; i < count; i++) {
        qDebug() << "设备 UDID:" << devices[i]->udid;
        qDebug() << "连接类型:" << (devices[i]->conn_type == CONNECTION_USBMUXD
                                    ? "USB" : "网络");
    }
    
    idevice_device_list_extended_free(devices);
}

// 连接到网络设备
idevice_t device = nullptr;
idevice_error_t err = idevice_new_with_options(&device,
                                               udid.toUtf8().constData(),
                                               IDEVICE_LOOKUP_NETWORK | IDEVICE_LOOKUP_USBMUX);
if (err == IDEVICE_E_SUCCESS) {
    qDebug() << "成功连接设备（自动选择最佳连接方式）";
    // 使用设备...
    idevice_free(device);
}
```

#### 3. libplist 内存管理 API 统一

### libplist v2.3.0+ 重要变更

#### 内存管理函数变更

在 libplist v2.3.0 及更高版本中，内存管理 API 发生了重要变更：

**旧版 API (已废弃)**:
```c
void plist_to_xml_free(char *plist_xml);  // ❌ 已在 v2.3.0+ 中移除
```

**新版 API (推荐使用)**:
```c
void plist_mem_free(void* ptr);  // ✅ v2.3.0+ 统一内存释放函数
```

#### 迁移对照表

| 操作 | 旧版 API | 新版 API | 适用版本 |

### 依赖库变更

#### 新增依赖库（v1.4.0）

| 库文件 | 用途 | 说明 |
|--------|------|------|
| `libimobiledevice-glue-1.0.dll` | 辅助工具库 | 提供通用辅助函数和工具 |
| `libbrotlicommon.dll` | Brotli 压缩 | 通用 Brotli 函数 |
| `libbrotlidec.dll` | Brotli 解压 | 解压缩功能 |
| `libbrotlienc.dll` | Brotli 压缩 | 压缩功能 |
| `libtatsu.dll` | TATSU 协议 | Apple TATSU 服务器通信 |
| `libcrypto-3-x64.dll` | OpenSSL 加密 | 升级到 OpenSSL 3.x |
| `libssl-3-x64.dll` | OpenSSL SSL/TLS | 升级到 OpenSSL 3.x |

#### 更新的依赖库

| 旧版本 (v1.3.17) | 新版本 (v1.4.0) | 变更说明 |
|------------------|-----------------|----------|
| `libplist.dll` | `libplist-2.0.dll` | API 版本升级，增加版本号 |
| `usbmuxd.dll` | `libusbmuxd-2.0.dll` | 重命名并升级到 v2.0 |
| OpenSSL 1.1.x | OpenSSL 3.x | 主要版本升级 |

#### 部署注意事项

1. **完整部署所有依赖**: 确保部署时包含所有新增的 DLL 文件
2. **DLL 搜索路径**: 所有依赖库应放在同一目录或系统 PATH 中
3. **版本匹配**: 不要混用不同版本的库文件
4. **OpenSSL 升级**: OpenSSL 3.x 有一些 API 变更，但 libimobiledevice 已适配

### 新增命令行工具

v1.4.0 版本新增了多个实用的命令行工具：

| 工具 | 功能 | 使用场景 |
|------|------|----------|
| `idevicebackup.exe` | 设备备份 (旧版) | 兼容旧版备份格式 |
| `idevicebackup2.exe` | 设备备份 (新版) | 使用新版备份协议 |
| `idevicebtlogger.exe` | 蓝牙日志 | 捕获蓝牙通信日志 |
| `idevicecrashreport.exe` | 崩溃报告 | 获取设备崩溃日志 |
| `idevicedevmodectl.exe` | 开发者模式控制 | 管理开发者模式设置 |
| `idevicedebugserverproxy.exe` | 调试服务器代理 | Xcode 调试支持 |

### 性能优化

v1.4.0 带来了多项性能改进：

#### 1. 文件传输性能
- **大文件传输**: 提升约 15-20%
- **批量小文件**: 提升约 10-15%
- **优化方式**: 改进缓冲区管理和数据分块策略

#### 2. 连接管理
- **连接建立**: 减少 SSL 握手时间
- **连接复用**: 更好的连接池管理
- **网络设备**: 优化 WiFi 连接稳定性

#### 3. 内存使用
- **内存占用**: 减少约 10-15%
- **内存泄漏**: 修复多个内存泄漏问题
- **对象池**: 改进的对象复用机制

### 向后兼容性

#### 完全兼容的 API
以下 API 在 v1.4.0 中保持完全兼容：
- ✅ `idevice_new()` / `idevice_free()`
- ✅ `idevice_get_device_list()` / `idevice_device_list_free()`
- ✅ `lockdownd_*` 系列函数
- ✅ `afc_*` 文件传输函数
- ✅ `instproxy_*` 应用管理函数
- ✅ `screenshotr_*` 截图函数
- ✅ `mobilebackup2_*` 备份函数

#### 已弃用但可用的 API
这些 API 被标记为已弃用，但仍然可用：
- ⚠️ `idevice_event_subscribe()` → 推荐使用 `idevice_events_subscribe()`
- ⚠️ `idevice_event_unsubscribe()` → 推荐使用 `idevice_events_unsubscribe()`
- ⚠️ `plist_to_xml_free()` → 推荐使用 `plist_mem_free()`

#### phone-linkc 兼容性策略

phone-linkc 采用动态加载机制，确保：
1. **自动检测**: 运行时检测可用的 API 版本
2. **优雅降级**: 优先使用新 API，自动回退到旧 API
3. **无缝升级**: 用户无需修改代码即可升级到新版本
4. **版本共存**: 同时支持 v1.3.17 和 v1.4.0

```cpp
// 自动适配示例
bool DeviceManager::initialize() {
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    if (!lib.initialize()) {
        qWarning() << "libimobiledevice 初始化失败";
        return false;
    }
    
    // 检测可用功能
    bool hasNetworkSupport = (lib.idevice_get_device_list_extended != nullptr);
    bool hasNewEventAPI = (lib.idevice_events_subscribe != nullptr);
    
    qDebug() << "网络设备支持:" << (hasNetworkSupport ? "是" : "否");
    qDebug() << "新事件 API:" << (hasNewEventAPI ? "是" : "否");
    
    return true;
}
```

### 升级建议

#### 推荐升级场景
建议在以下情况下升级到 v1.4.0：
- ✅ 需要支持网络连接的 iOS 设备
- ✅ 需要同时管理多个设备事件订阅
- ✅ 需要更好的性能和稳定性
- ✅ 需要支持 iOS 18.x 设备

#### 保持旧版本场景
可以继续使用 v1.3.17 如果：
- ⚠️ 只需要 USB 连接支持
- ⚠️ 部署环境限制无法更新依赖库
- ⚠️ 现有代码稳定且无升级需求

#### 升级步骤

1. **备份现有版本**
   ```bash
   # 备份当前的 libimobiledevice 目录
   cp -r thirdparty/libimobiledevice thirdparty/libimobiledevice.v1.3.17
   ```

2. **更新库文件**
   - 下载 v1.4.0 版本
   - 替换所有 DLL 文件
   - 更新头文件

3. **测试兼容性**
   ```cpp
   // 运行测试确保兼容性
   bool testLibimobiledevice() {
       LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
       
       if (!lib.initialize()) {
           qCritical() << "初始化失败";
           return false;
       }
       
       // 测试基本功能
       char **devices = nullptr;
       int count = 0;
       if (lib.idevice_get_device_list(&devices, &count) == IDEVICE_E_SUCCESS) {
           qDebug() << "找到" << count << "个设备";
           lib.idevice_device_list_free(devices);
           return true;
       }
       
       return false;
   }
   ```

4. **逐步迁移代码**
   - 优先更新事件处理代码
   - 添加网络设备支持（可选）
   - 更新内存管理调用

5. **验证和部署**
   - 在测试环境充分测试
   - 监控日志和错误报告
   - 逐步推广到生产环境

|------|---------|---------|----------|
| 释放 XML 字符串 | `plist_to_xml_free(xml)` | `plist_mem_free(xml)` | v2.3.0+ |
| 释放二进制数据 | `free(bin)` | `plist_mem_free(bin)` | v2.3.0+ |
| 释放字符串值 | `free(str)` | `plist_mem_free(str)` | v2.3.0+ |
| 释放数据缓冲区 | `free(data)` | `plist_mem_free(data)` | v2.3.0+ |
| 释放 plist 节点 | `plist_free(node)` | `plist_free(node)` | 所有版本 |

#### 代码迁移示例

**旧版代码** (libplist v2.2.0 及更早):
```cpp
char *xml = NULL;
uint32_t length = 0;
plist_to_xml(plist, &xml, &length);

// 使用 XML 数据...

plist_to_xml_free(xml);  // 旧版专用释放函数
```

**新版代码** (libplist v2.3.0+):
```cpp
char *xml = NULL;
uint32_t length = 0;
plist_to_xml(plist, &xml, &length);

// 使用 XML 数据...

plist_mem_free(xml);  // 统一内存释放函数
```

#### phone-linkc 项目适配

phone-linkc 项目已完全适配新版 API，主要变更包括：

1. **头文件更新** ([`plist_dynamic.h`](../phone-linkc/src/platform/plist_dynamic.h)):
   ```cpp
   // 旧版
   typedef void (*plist_to_xml_free_func)(char *plist_xml);
   
   // 新版
   typedef void (*plist_mem_free_func)(void* ptr);
   ```

2. **动态加载更新** ([`libimobiledevice_dynamic.cpp`](../phone-linkc/src/platform/libimobiledevice_dynamic.cpp)):
   ```cpp
   // 旧版
   success &= loadAndTrack("plist_to_xml_free", plist_to_xml_free, m_plistLib);
   
   // 新版
   success &= loadAndTrack("plist_mem_free", plist_mem_free, m_plistLib);
   ```

3. **使用代码更新** ([`contactmanager.cpp`](../phone-linkc/src/core/contact/contactmanager.cpp)):
   ```cpp
   // 旧版
   if (plist_xml && lib.plist_to_xml_free) {
       lib.plist_to_xml_free(plist_xml);
   }
   
   // 新版
   if (plist_xml && lib.plist_mem_free) {
       lib.plist_mem_free(plist_xml);
   }
   ```

#### 兼容性建议

为了保持与不同版本的兼容性，建议：

1. **优先使用新版 API**: 如果使用 libplist v2.3.0+，统一使用 `plist_mem_free()`
2. **运行时检测**: 通过动态加载可以在运行时检测可用的函数
3. **渐进式迁移**: 先更新头文件和函数指针，再逐步更新调用代码

#### 其他 API 变更

除了内存管理，libplist v2.3.0+ 还引入了：

- 改进的错误处理 (`plist_err_t` 返回类型)
- 更严格的类型检查
- 性能优化
- 更好的 Unicode 支持

详细的变更日志请参考 [libplist GitHub Releases](https://github.com/libimobiledevice/libplist/releases)。


> ⚠️ **重要提示**: libplist v2.3.0+ 中 `plist_to_xml_free()` 已被 `plist_mem_free()` 取代，phone-linkc 已适配新版本 API。

## 动态库加载机制

phone-linkc项目采用运行时动态加载libimobiledevice库的方式，无需在编译时链接静态库，提高了部署的灵活性和兼容性。

### 动态加载器设计

#### LibimobiledeviceDynamic类
```cpp
class LibimobiledeviceDynamic {
public:
    static LibimobiledeviceDynamic& instance();
    bool initialize();
    bool isInitialized() const;
    
    // 核心函数指针
    idevice_error_t (*idevice_new)(idevice_t *device, const char *udid);
    idevice_error_t (*idevice_free)(idevice_t device);
    idevice_error_t (*idevice_get_device_list)(char ***devices, int *count);
    idevice_error_t (*idevice_device_list_free)(char **devices);
    
    // Lockdown服务函数指针
    lockdownd_error_t (*lockdownd_client_new_with_handshake)(idevice_t device, lockdownd_client_t *client, const char *label);
    lockdownd_error_t (*lockdownd_client_free)(lockdownd_client_t client);
    lockdownd_error_t (*lockdownd_get_value)(lockdownd_client_t client, const char *domain, const char *key, plist_t *value);
    
    // plist函数指针
    plist_type (*plist_get_node_type)(plist_t node);
    void (*plist_get_string_val)(plist_t node, char **val);
    void (*plist_get_uint_val)(plist_t node, uint64_t *val);
    void (*plist_free)(plist_t plist);
    
    // 事件处理函数指针
    idevice_error_t (*idevice_event_subscribe)(idevice_event_cb_t callback, void *user_data);
    idevice_error_t (*idevice_event_unsubscribe)(void);
    
private:
    LibimobiledeviceDynamic() = default;
    ~LibimobiledeviceDynamic();
    
    bool loadLibrary(const QString& libraryName);
    void* loadFunction(const QString& functionName);
    
    QLibrary* m_imobiledeviceLib;
    QLibrary* m_plistLib;
    bool m_initialized;
};
```

### 使用动态加载的最佳实践

#### 1. 初始化检查
```cpp
void DeviceManager::initializeLibimobiledevice() {
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (loader.initialize()) {
        qDebug() << "libimobiledevice 动态库已加载";
        // 可以安全使用iOS设备功能
    } else {
        qDebug() << "libimobiledevice 动态库加载失败，无法连接 iOS 设备";
        // 应用仍可运行，但iOS功能不可用
    }
}
```

#### 2. 函数调用模式
```cpp
// 传统直接调用方式（需要编译时链接）
idevice_error_t error = idevice_get_device_list(&devices, &count);

// 动态加载调用方式（运行时加载）
LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
if (loader.isInitialized() && loader.idevice_get_device_list) {
    idevice_error_t error = loader.idevice_get_device_list(&devices, &count);
    // 处理结果...
}
```

#### 3. 错误处理和回退机制
```cpp
DeviceInfo DeviceInfoManager::getDeviceInfo(const QString &udid) {
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        // 回退到模拟数据
        return getSimulatedDeviceInfo(udid);
    }
    
    // 使用真实的libimobiledevice功能
    DeviceInfo info;
    // ... 实际实现
    return info;
}

DeviceInfo DeviceInfoManager::getSimulatedDeviceInfo(const QString &udid) {
    DeviceInfo info;
    info.udid = udid;
    info.name = "模拟 iPhone";
    info.model = "A2482";
    info.productVersion = "17.1.1";
    // ... 模拟数据
    return info;
}
```

### 库文件搜索路径

动态加载器会在以下路径搜索libimobiledevice库文件：

#### Windows平台
1. 应用程序目录下的`thirdparty/libimobiledevice/`
2. 系统PATH环境变量中的路径
3. 注册表中的iTunes安装路径

#### macOS平台
1. Homebrew标准路径：`/opt/homebrew/lib`（Apple Silicon）或`/usr/local/lib`（Intel）
2. MacPorts路径：`/opt/local/lib`
3. 系统库路径：`/usr/lib`, `/usr/local/lib`

#### Linux平台
1. 系统标准路径：`/usr/lib`, `/usr/local/lib`
2. pkg-config指定的路径
3. LD_LIBRARY_PATH环境变量路径

### 部署优势

#### 1. 简化分发
- 无需用户预安装libimobiledevice
- 减少依赖冲突
- 支持便携式部署

#### 2. 优雅降级
- 库不可用时应用仍可运行
- 提供清晰的错误提示
- 支持功能子集运行

#### 3. 版本兼容性
- 支持多个libimobiledevice版本
- 运行时检测可用功能
- 向后兼容性更好

### 动态加载实现细节

#### 库加载顺序
```cpp
bool LibimobiledeviceDynamic::initialize() {
    // 1. 加载plist库（libimobiledevice的依赖）
    if (!loadPlistLibrary()) {
        qWarning() << "无法加载plist库";
        return false;
    }
    
    // 2. 加载libimobiledevice主库
    if (!loadImobiledeviceLibrary()) {
        qWarning() << "无法加载libimobiledevice库";
        return false;
    }
    
    // 3. 解析函数符号
    if (!loadAllFunctions()) {
        qWarning() << "无法解析所有必需的函数";
        return false;
    }
    
    m_initialized = true;
    return true;
}
```

#### 函数指针解析
```cpp
bool LibimobiledeviceDynamic::loadAllFunctions() {
    // 核心设备管理函数
    LOAD_FUNCTION(idevice_new);
    LOAD_FUNCTION(idevice_free);
    LOAD_FUNCTION(idevice_get_device_list);
    LOAD_FUNCTION(idevice_device_list_free);
    LOAD_FUNCTION(idevice_event_subscribe);
    LOAD_FUNCTION(idevice_event_unsubscribe);
    
    // Lockdown服务函数
    LOAD_FUNCTION(lockdownd_client_new_with_handshake);
    LOAD_FUNCTION(lockdownd_client_free);
    LOAD_FUNCTION(lockdownd_get_value);
    
    // plist处理函数
    LOAD_FUNCTION(plist_get_node_type);
    LOAD_FUNCTION(plist_get_string_val);
    LOAD_FUNCTION(plist_get_uint_val);
    LOAD_FUNCTION(plist_free);
    
    return true; // 所有函数都成功加载
}

#define LOAD_FUNCTION(name) \
    do { \
        name = reinterpret_cast<decltype(name)>(m_imobiledeviceLib->resolve(#name)); \
        if (!name) { \
            qWarning() << "无法解析函数:" << #name; \
            return false; \
        } \
    } while(0)
```

## 核心API模块

### 1. 设备管理 API (libimobiledevice)

#### 1.1 设备枚举与连接

##### idevice_get_device_list()
```c
idevice_error_t idevice_get_device_list(char ***devices, int *count);
```

**功能描述**: 获取当前连接的所有iOS设备列表

**参数说明**:
- `devices`: 输出参数，设备UDID字符串数组指针
- `count`: 输出参数，设备数量

**返回值**:
- `IDEVICE_E_SUCCESS (0)`: 成功
- `IDEVICE_E_INVALID_ARG (-1)`: 参数无效
- `IDEVICE_E_NO_DEVICE (-3)`: 没有找到设备
- `IDEVICE_E_UNKNOWN_ERROR (-256)`: 未知错误

**phone-linkc项目中的使用示例**:
```cpp
// 在DeviceManager类中扫描设备的实现
void DeviceManager::scanCurrentDevices() {
    char **device_list = nullptr;
    int device_count = 0;
    
    // 获取设备列表
    if (idevice_get_device_list(&device_list, &device_count) != IDEVICE_E_SUCCESS) {
        qDebug() << "获取设备列表失败";
        return;
    }

    QStringList currentDevices;
    
    // 处理找到的设备
    for (int i = 0; i < device_count; i++) {
        QString udid = QString::fromUtf8(device_list[i]);
        currentDevices << udid;
        
        // 检查是否是新设备
        if (!m_knownDevices.contains(udid)) {
            QString deviceName = getDeviceName(udid);
            qDebug() << "发现新设备:" << udid << "名称:" << deviceName;
            emit deviceFound(udid, deviceName);
        }
    }
    
    // 检查丢失的设备
    for (const QString &knownUdid : m_knownDevices) {
        if (!currentDevices.contains(knownUdid)) {
            qDebug() << "设备断开连接:" << knownUdid;
            emit deviceLost(knownUdid);
        }
    }
    
    m_knownDevices = currentDevices;
    
    // 清理设备列表 - 重要！防止内存泄漏
    idevice_device_list_free(device_list);
}
```

**注意事项**:
- 必须调用`idevice_device_list_free()`释放内存
- 设备UDID是40位十六进制字符串
- 在Windows上需要安装iTunes驱动程序

##### idevice_new()
```c
idevice_error_t idevice_new(idevice_t *device, const char *udid);
```

**功能描述**: 创建设备连接实例

**参数说明**:
- `device`: 输出参数，设备句柄指针
- `udid`: 设备UDID字符串，NULL表示连接第一个可用设备

**返回值**:
- `IDEVICE_E_SUCCESS (0)`: 连接成功
- `IDEVICE_E_NO_DEVICE (-3)`: 设备不存在或无法访问
- `IDEVICE_E_TIMEOUT (-10)`: 连接超时
- `IDEVICE_E_SSL_ERROR (-6)`: SSL连接错误
- `IDEVICE_E_INVALID_ARG (-1)`: 参数无效

**phone-linkc项目中的使用示例**:
```cpp
// 在DeviceManager类中初始化设备连接
bool DeviceManager::initializeConnection(const QString &udid) {
    // 创建设备连接
    if (idevice_new(&m_device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
        qWarning() << "创建设备连接失败:" << udid;
        return false;
    }
    
    // 创建 lockdown 客户端
    if (lockdownd_client_new_with_handshake(m_device, &m_lockdown, "phone-linkc") != LOCKDOWN_E_SUCCESS) {
        qWarning() << "创建 lockdown 客户端失败:" << udid;
        idevice_free(m_device);
        m_device = nullptr;
        return false;
    }
    
    return true;
}

// 获取设备名称的临时连接示例
QString DeviceManager::getDeviceName(const QString &udid) {
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    plist_t value = nullptr;
    QString name = "Unknown Device";
    
    // 创建临时连接获取设备名称
    if (idevice_new(&device, udid.toUtf8().constData()) == IDEVICE_E_SUCCESS) {
        if (lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") == LOCKDOWN_E_SUCCESS) {
            if (lockdownd_get_value(lockdown, nullptr, "DeviceName", &value) == LOCKDOWN_E_SUCCESS) {
                if (value && plist_get_node_type(value) == PLIST_STRING) {
                    char *str_value = nullptr;
                    plist_get_string_val(value, &str_value);
                    if (str_value) {
                        name = QString::fromUtf8(str_value);
                        free(str_value); // 释放字符串内存
                    }
                }
                if (value) plist_free(value);
            }
            lockdownd_client_free(lockdown);
        }
        idevice_free(device);
    }
    
    return name;
}
```

**注意事项**:
- 每个`idevice_new()`调用必须对应一个`idevice_free()`调用
- 建议使用RAII模式管理设备生命周期
- 连接失败时应检查设备是否已信任此电脑

##### idevice_free()
```c
idevice_error_t idevice_free(idevice_t device);
```

**功能描述**: 释放设备连接资源

**参数说明**:
- `device`: 要释放的设备句柄

**返回值**:
- `IDEVICE_E_SUCCESS`: 释放成功

#### 1.2 设备事件监听

##### idevice_event_subscribe()
```c
idevice_error_t idevice_event_subscribe(idevice_event_cb_t callback, void *user_data);
```

**功能描述**: 订阅iOS设备连接/断开事件通知

**参数说明**:
- `callback`: 事件回调函数指针
- `user_data`: 传递给回调函数的用户数据

**返回值**:
- `IDEVICE_E_SUCCESS (0)`: 订阅成功
- `IDEVICE_E_INVALID_ARG (-1)`: 参数无效

**回调函数原型**:
```c
typedef void (*idevice_event_cb_t)(const idevice_event_t *event, void *user_data);

typedef struct {
    idevice_event_type_t event;  // 事件类型
    const char *udid;           // 设备UDID
    int conn_type;              // 连接类型
} idevice_event_t;

// 事件类型
typedef enum {
    IDEVICE_DEVICE_ADD = 1,      // 设备连接
    IDEVICE_DEVICE_REMOVE,       // 设备断开
    IDEVICE_DEVICE_PAIRED        // 设备配对
} idevice_event_type_t;
```

**phone-linkc项目中的实现**:
```cpp
// 在DeviceManager类中启动事件订阅
void DeviceManager::startEventSubscription() {
    if (m_eventContext) {
        return; // 已经订阅了
    }
    
    idevice_error_t ret = idevice_event_subscribe(deviceEventCallback, this);
    if (ret == IDEVICE_E_SUCCESS) {
        m_eventContext = (void*)1; // 用非空值标记订阅状态
        qDebug() << "成功订阅 USB 设备事件通知 - 纯事件驱动模式";
    } else {
        qDebug() << "订阅 USB 设备事件失败";
        m_eventContext = nullptr;
    }
}

// 设备事件回调函数
void DeviceManager::deviceEventCallback(const idevice_event_t* event, void* user_data) {
    DeviceManager* manager = static_cast<DeviceManager*>(user_data);
    
    if (!manager || !event) {
        return;
    }
    
    QString udid = QString::fromUtf8(event->udid);
    
    switch (event->event) {
        case IDEVICE_DEVICE_ADD:
            qDebug() << "USB 事件：设备连接" << udid;
            if (!manager->m_knownDevices.contains(udid)) {
                QString deviceName = manager->getDeviceName(udid);
                manager->m_knownDevices << udid;
                emit manager->deviceFound(udid, deviceName);
            }
            break;
            
        case IDEVICE_DEVICE_REMOVE:
            qDebug() << "USB 事件：设备断开" << udid;
            if (manager->m_knownDevices.contains(udid)) {
                manager->m_knownDevices.removeAll(udid);
                emit manager->deviceLost(udid);
                
                // 如果是当前连接的设备断开了
                if (udid == manager->m_currentUdid) {
                    manager->disconnectFromDevice();
                }
            }
            break;
            
        case IDEVICE_DEVICE_PAIRED:
            qDebug() << "USB 事件：设备配对" << udid;
            if (!manager->m_knownDevices.contains(udid)) {
                QString deviceName = manager->getDeviceName(udid);
                manager->m_knownDevices << udid;
                emit manager->deviceFound(udid, deviceName);
            }
            break;
    }
}
```

##### idevice_event_unsubscribe()
```c
idevice_error_t idevice_event_unsubscribe(void);
```

**功能描述**: 取消设备事件订阅

**phone-linkc项目中的实现**:
```cpp
void DeviceManager::stopEventSubscription() {
    if (m_eventContext) {
        idevice_event_unsubscribe();
        m_eventContext = nullptr;
        qDebug() << "已停止 USB 设备事件订阅";
    }
}
```

#### 1.3 设备信息获取

##### idevice_get_udid()
```c
idevice_error_t idevice_get_udid(idevice_t device, char **udid);
```

**功能描述**: 获取设备UDID

**参数说明**:
- `device`: 设备句柄
- `udid`: 输出参数，设备UDID字符串指针

**返回值**:
- `IDEVICE_E_SUCCESS (0)`: 获取成功
- `IDEVICE_E_INVALID_ARG (-1)`: 参数无效

**使用示例**:
```cpp
char *udid = nullptr;
idevice_error_t error = idevice_get_udid(device, &udid);
if (error == IDEVICE_E_SUCCESS && udid) {
    qDebug() << "设备UDID:" << udid;
    free(udid); // 必须释放内存
}
```

##### idevice_get_device_list_extended()
```c
idevice_error_t idevice_get_device_list_extended(idevice_info_t **device_list, int *count);
```

**功能描述**: 获取扩展设备信息列表（包含连接类型等）

**参数说明**:
- `device_list`: 输出参数，设备信息结构体数组指针
- `count`: 输出参数，设备数量

**设备信息结构体**:
```c
typedef struct {
    char *udid;                    // 设备UDID
    idevice_connection_type_t conn_type;  // 连接类型
} idevice_info_t;

typedef enum {
    CONNECTION_USBMUXD = 1,        // USB连接
    CONNECTION_NETWORK             // 网络连接
} idevice_connection_type_t;
```

### 2. Lockdown服务 API (lockdownd)

#### 2.1 客户端连接

##### lockdownd_client_new()
```c
lockdownd_error_t lockdownd_client_new(idevice_t device, 
                                       lockdownd_client_t *client, 
                                       const char *label);
```

**功能描述**: 创建lockdown服务客户端

**参数说明**:
- `device`: 设备句柄
- `client`: 输出参数，客户端句柄
- `label`: 客户端标识符（建议使用应用名称）

**返回值**:
- `LOCKDOWN_E_SUCCESS`: 创建成功
- `LOCKDOWN_E_INVALID_ARG`: 参数无效
- `LOCKDOWN_E_SSL_ERROR`: SSL连接错误

**使用示例**:
```cpp
lockdownd_client_t client = NULL;
lockdownd_error_t error = lockdownd_client_new(device, &client, "phone-linkc");
if (error == LOCKDOWN_E_SUCCESS) {
    // 可以进行lockdown操作
    lockdownd_client_free(client);
}
```

#### 2.2 设备属性操作

##### lockdownd_get_value()
```c
lockdownd_error_t lockdownd_get_value(lockdownd_client_t client,
                                     const char *domain,
                                     const char *key,
                                     plist_t *value);
```

**功能描述**: 获取设备属性值

**参数说明**:
- `client`: lockdown客户端句柄
- `domain`: 属性域名（NULL表示根域）
- `key`: 属性键名
- `value`: 输出参数，属性值

**常用属性键**:
- `DeviceName`: 设备名称
- `ProductType`: 产品类型
- `ProductVersion`: iOS版本
- `SerialNumber`: 序列号
- `UniqueDeviceID`: 设备UDID
- `HardwareModel`: 硬件型号
- `BuildVersion`: 构建版本

**使用示例**:
```cpp
// 获取设备名称
plist_t device_name = NULL;
lockdownd_error_t error = lockdownd_get_value(client, NULL, "DeviceName", &device_name);
if (error == LOCKDOWN_E_SUCCESS && device_name) {
    char *name_str = NULL;
    plist_get_string_val(device_name, &name_str);
    qDebug() << "设备名称:" << name_str;
    free(name_str);
    plist_free(device_name);
}

// 获取iOS版本
plist_t ios_version = NULL;
lockdownd_get_value(client, NULL, "ProductVersion", &ios_version);
if (ios_version) {
    char *version_str = NULL;
    plist_get_string_val(ios_version, &version_str);
    qDebug() << "iOS版本:" << version_str;
    free(version_str);
    plist_free(ios_version);
}
```

##### lockdownd_set_value()
```c
lockdownd_error_t lockdownd_set_value(lockdownd_client_t client,
                                     const char *domain,
                                     const char *key,
                                     plist_t value);
```

**功能描述**: 设置设备属性值

**参数说明**:
- `client`: lockdown客户端句柄
- `domain`: 属性域名
- `key`: 属性键名
- `value`: 要设置的属性值

**使用示例**:
```cpp
// 设置设备名称（需要设备已信任此电脑）
bool setDeviceName(lockdownd_client_t client, const QString& newName) {
    if (!client || newName.isEmpty()) {
        return false;
    }
    
    plist_t name_value = plist_new_string(newName.toUtf8().constData());
    if (!name_value) {
        return false;
    }
    
    lockdownd_error_t error = lockdownd_set_value(client, NULL, "DeviceName", name_value);
    plist_free(name_value);
    
    return error == LOCKDOWN_E_SUCCESS;
}
```

#### 2.3 服务管理

##### lockdownd_start_service()
```c
lockdownd_error_t lockdownd_start_service(lockdownd_client_t client,
                                          const char *service_name,
                                          lockdownd_service_descriptor_t *service);
```

**功能描述**: 启动指定的设备服务

**参数说明**:
- `client`: lockdown客户端句柄
- `service_name`: 服务名称（如"com.apple.afc"）
- `service`: 输出参数，服务描述符

**返回值**:
- `LOCKDOWN_E_SUCCESS`: 服务启动成功
- `LOCKDOWN_E_INVALID_SERVICE`: 服务名称无效
- `LOCKDOWN_E_START_SERVICE_FAILED`: 服务启动失败

**常用服务名称**:
- `com.apple.afc`: 文件传输服务
- `com.apple.mobile.screenshotr`: 屏幕截图服务
- `com.apple.mobile.installation_proxy`: 应用安装服务
- `com.apple.syslog_relay`: 系统日志服务
- `com.apple.mobile.notification_proxy`: 通知代理服务
- `com.apple.springboardservices`: SpringBoard服务
- `com.apple.mobile.diagnostics_relay`: 诊断中继服务

**使用示例**:
```cpp
// 启动AFC服务并检查是否成功
bool startAFCService(idevice_t device, lockdownd_client_t lockdown, uint16_t *port) {
    lockdownd_service_descriptor_t service = NULL;
    lockdownd_error_t error = lockdownd_start_service(lockdown, "com.apple.afc", &service);
    
    if (error != LOCKDOWN_E_SUCCESS || !service) {
        qWarning() << "启动AFC服务失败:" << error;
        return false;
    }
    
    *port = service->port;
    
    // 必须释放服务描述符
    lockdownd_service_descriptor_free(service);
    return true;
}
```

##### lockdownd_client_free()
```c
lockdownd_error_t lockdownd_client_free(lockdownd_client_t client);
```

**功能描述**: 释放lockdown客户端资源

**参数说明**:
- `client`: 要释放的客户端句柄

**返回值**:
- `LOCKDOWN_E_SUCCESS`: 释放成功

##### lockdownd_service_descriptor_free()
```c
void lockdownd_service_descriptor_free(lockdownd_service_descriptor_t service);
```

**功能描述**: 释放服务描述符资源

**参数说明**:
- `service`: 要释放的服务描述符

**服务检查示例**:
```cpp
// 检查服务是否可用
bool isServiceAvailable(idevice_t device, const QString& serviceName) {
    lockdownd_client_t lockdown = NULL;
    
    if (lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") != LOCKDOWN_E_SUCCESS) {
        return false;
    }
    
    lockdownd_service_descriptor_t service = NULL;
    lockdownd_error_t error = lockdownd_start_service(lockdown, serviceName.toUtf8().constData(), &service);
    
    bool available = (error == LOCKDOWN_E_SUCCESS && service != NULL);
    
    if (service) {
        lockdownd_service_descriptor_free(service);
    }
    
    lockdownd_client_free(lockdown);
    
    return available;
}
```

### 3. 屏幕截图 API (screenshotr)

#### 3.1 截图服务

##### screenshotr_client_start_service()
```c
screenshotr_error_t screenshotr_client_start_service(idevice_t device,
                                                    screenshotr_client_t *client,
                                                    const char *label);
```

**功能描述**: 启动屏幕截图服务

**参数说明**:
- `device`: 设备句柄
- `client`: 输出参数，截图客户端句柄
- `label`: 客户端标识符

**返回值**:
- `SCREENSHOTR_E_SUCCESS`: 启动成功
- `SCREENSHOTR_E_INVALID_ARG`: 参数无效

##### screenshotr_take_screenshot()
```c
screenshotr_error_t screenshotr_take_screenshot(screenshotr_client_t client,
                                               char **imgdata,
                                               uint64_t *imgsize);
```

**功能描述**: 获取设备屏幕截图

**参数说明**:
- `client`: 截图客户端句柄
- `imgdata`: 输出参数，图像数据指针
- `imgsize`: 输出参数，图像数据大小

**返回值**:
- `SCREENSHOTR_E_SUCCESS`: 截图成功

**使用示例**:
```cpp
screenshotr_client_t screenshotr = NULL;
screenshotr_error_t error = screenshotr_client_start_service(device, &screenshotr, "phone-linkc");
if (error == SCREENSHOTR_E_SUCCESS) {
    char *imgdata = NULL;
    uint64_t imgsize = 0;
    
    error = screenshotr_take_screenshot(screenshotr, &imgdata, &imgsize);
    if (error == SCREENSHOTR_E_SUCCESS) {
        // 将图像数据转换为QImage
        QImage screenshot = QImage::fromData((uchar*)imgdata, imgsize, "PNG");
        
        // 显示或保存截图
        QLabel *imageLabel = new QLabel();
        imageLabel->setPixmap(QPixmap::fromImage(screenshot));
        
        free(imgdata);
    }
    screenshotr_client_free(screenshotr);
}
```

##### screenshotr_client_free()
```c
screenshotr_error_t screenshotr_client_free(screenshotr_client_t client);
```

**功能描述**: 释放截图服务客户端资源

**参数说明**:
- `client`: 截图客户端句柄

**返回值**:
- `SCREENSHOTR_E_SUCCESS`: 释放成功

#### 3.2 屏幕镜像实现

屏幕镜像功能需要通过持续截图实现，推荐使用独立线程处理，避免阻塞主线程。

**基础屏幕镜像实现**:

```cpp
// 屏幕镜像工作线程
class ScreenMirrorWorker : public QThread {
    Q_OBJECT
    
private:
    idevice_t device_;
    bool running_;
    int targetFps_;
    
public:
    explicit ScreenMirrorWorker(idevice_t device, QObject *parent = nullptr)
        : QThread(parent), device_(device), running_(false), targetFps_(30) {
    }
    
    void setTargetFps(int fps) {
        targetFps_ = qMax(1, qMin(60, fps)); // 限制在1-60fps之间
    }
    
    void stopMirroring() {
        running_ = false;
        wait(); // 等待线程结束
    }
    
protected:
    void run() override {
        running_ = true;
        
        screenshotr_client_t screenshotr = nullptr;
        screenshotr_error_t error = screenshotr_client_start_service(device_, &screenshotr, "phone-linkc");
        
        if (error != SCREENSHOTR_E_SUCCESS) {
            emit errorOccurred(QString("启动截图服务失败: %1").arg(error));
            return;
        }
        
        // 计算帧间隔
        const int frameInterval = 1000 / targetFps_;
        QElapsedTimer frameTimer;
        
        while (running_) {
            frameTimer.start();
            
            char *imgdata = nullptr;
            uint64_t imgsize = 0;
            
            error = screenshotr_take_screenshot(screenshotr, &imgdata, &imgsize);
            
            if (error == SCREENSHOTR_E_SUCCESS && imgdata) {
                // 创建QImage并转换为RGB格式以提高性能
                QImage screenshot = QImage::fromData(reinterpret_cast<const uchar*>(imgdata), 
                                                  static_cast<int>(imgsize), "PNG");
                
                if (!screenshot.isNull()) {
                    emit frameReady(screenshot);
                }
                
                free(imgdata);
            } else {
                qWarning() << "截图失败:" << error;
                // 连续失败多次则停止镜像
                static int failureCount = 0;
                if (++failureCount > 5) {
                    emit errorOccurred("连续截图失败，停止屏幕镜像");
                    break;
                }
            }
            
            // 控制帧率
            int elapsed = frameTimer.elapsed();
            if (elapsed < frameInterval) {
                msleep(frameInterval - elapsed);
            }
        }
        
        screenshotr_client_free(screenshotr);
    }
    
signals:
    void frameReady(const QImage& frame);
    void errorOccurred(const QString& message);
};

// 主窗口中的屏幕镜像控制
class MainWindow : public QMainWindow {
    Q_OBJECT
    
private:
    ScreenMirrorWorker *mirrorWorker_;
    QLabel *screenLabel_;
    
public slots:
    void startScreenMirroring() {
        if (mirrorWorker_ && mirrorWorker_->isRunning()) {
            // 已经在镜像，停止
            stopScreenMirroring();
            return;
        }
        
        idevice_t device = deviceManager_->getCurrentDevice();
        if (!device) {
            QMessageBox::warning(this, "错误", "未连接设备");
            return;
        }
        
        mirrorWorker_ = new ScreenMirrorWorker(device, this);
        
        connect(mirrorWorker_, &ScreenMirrorWorker::frameReady, 
                this, [this](const QImage& frame) {
                    // 调整图像大小以适应显示区域
                    QPixmap pixmap = QPixmap::fromImage(frame);
                    if (screenLabel_) {
                        screenLabel_->setPixmap(pixmap.scaled(
                            screenLabel_->size(), 
                            Qt::KeepAspectRatio, 
                            Qt::SmoothTransformation));
                    }
                });
        
        connect(mirrorWorker_, &ScreenMirrorWorker::errorOccurred,
                this, &MainWindow::onMirrorError);
        
        mirrorWorker_->start();
        statusBar()->showMessage("屏幕镜像已启动");
    }
    
    void stopScreenMirroring() {
        if (mirrorWorker_) {
            mirrorWorker_->stopMirroring();
            mirrorWorker_->deleteLater();
            mirrorWorker_ = nullptr;
            statusBar()->showMessage("屏幕镜像已停止");
        }
    }
    
    void onMirrorError(const QString& message) {
        qWarning() << "屏幕镜像错误:" << message;
        stopScreenMirroring();
        statusBar()->showMessage("屏幕镜像出错: " + message);
    }
};
```

**高级屏幕镜像功能**:

```cpp
// 增强的屏幕镜像工作器，支持性能监控和图像处理
class EnhancedScreenMirrorWorker : public ScreenMirrorWorker {
    Q_OBJECT
    
private:
    // 性能监控
    QElapsedTimer performanceTimer_;
    qint64 totalFrames_;
    qint64 totalProcessingTime_;
    qint64 minFrameTime_;
    qint64 maxFrameTime_;
    
    // 图像处理
    bool enableProcessing_;
    int brightness_;
    int contrast_;
    bool enableGrayscale_;
    QImage::Format targetFormat_;
    
public:
    explicit EnhancedScreenMirrorWorker(idevice_t device, QObject *parent = nullptr)
        : ScreenMirrorWorker(device, parent)
        , totalFrames_(0), totalProcessingTime_(0)
        , minFrameTime_(LLONG_MAX), maxFrameTime_(0)
        , enableProcessing_(false), brightness_(0), contrast_(0)
        , enableGrayscale_(false), targetFormat_(QImage::Format_RGB32) {
        
        performanceTimer_.start();
    }
    
    // 图像处理设置
    void setImageProcessing(bool enable, int brightness = 0, int contrast = 0, bool grayscale = false) {
        enableProcessing_ = enable;
        brightness_ = brightness;
        contrast_ = contrast;
        enableGrayscale_ = grayscale;
    }
    
    void setTargetFormat(QImage::Format format) {
        targetFormat_ = format;
    }
    
    // 获取性能统计
    struct PerformanceStats {
        qint64 avgFrameTime;
        qint64 minFrameTime;
        qint64 maxFrameTime;
        qint64 avgFps;
        qint64 totalFrames;
        qint64 totalTime;
    };
    
    PerformanceStats getPerformanceStats() const {
        PerformanceStats stats;
        if (totalFrames_ > 0) {
            stats.avgFrameTime = totalProcessingTime_ / totalFrames_;
            stats.minFrameTime = minFrameTime_;
            stats.maxFrameTime = maxFrameTime_;
            stats.avgFps = totalFrames_ * 1000 / totalProcessingTime_;
            stats.totalFrames = totalFrames_;
            stats.totalTime = totalProcessingTime_;
        }
        return stats;
    }
    
protected:
    void run() override {
        running_ = true;
        
        screenshotr_client_t screenshotr = nullptr;
        screenshotr_error_t error = screenshotr_client_start_service(device_, &screenshotr, "phone-linkc");
        
        if (error != SCREENSHOTR_E_SUCCESS) {
            emit errorOccurred(QString("启动截图服务失败: %1").arg(error));
            return;
        }
        
        const int frameInterval = 1000 / targetFps_;
        QElapsedTimer frameTimer;
        
        while (running_) {
            frameTimer.start();
            
            char *imgdata = nullptr;
            uint64_t imgsize = 0;
            
            error = screenshotr_take_screenshot(screenshotr, &imgdata, &imgsize);
            
            if (error == SCREENSHOTR_E_SUCCESS && imgdata) {
                // 创建QImage
                QImage screenshot = QImage::fromData(reinterpret_cast<const uchar*>(imgdata), 
                                                  static_cast<int>(imgsize), "PNG");
                
                if (!screenshot.isNull()) {
                    // 转换格式（如果需要）
                    if (screenshot.format() != targetFormat_) {
                        screenshot = screenshot.convertToFormat(targetFormat_);
                    }
                    
                    // 应用图像处理
                    if (enableProcessing_) {
                        processImage(screenshot);
                    }
                    
                    // 更新性能统计
                    qint64 frameTime = frameTimer.elapsed();
                    updatePerformanceStats(frameTime);
                    
                    emit frameReady(screenshot);
                    emit performanceUpdated(getPerformanceStats());
                }
                
                free(imgdata);
            } else {
                qWarning() << "截图失败:" << error;
                static int failureCount = 0;
                if (++failureCount > 5) {
                    emit errorOccurred("连续截图失败，停止屏幕镜像");
                    break;
                }
            }
            
            // 控制帧率
            int elapsed = frameTimer.elapsed();
            if (elapsed < frameInterval) {
                msleep(frameInterval - elapsed);
            }
        }
        
        screenshotr_client_free(screenshotr);
    }
    
private:
    void processImage(QImage& image) {
        // 调整亮度和对比度
        if (brightness_ != 0 || contrast_ != 0) {
            for (int y = 0; y < image.height(); ++y) {
                QRgb* scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));
                for (int x = 0; x < image.width(); ++x) {
                    int r = qRed(scanLine[x]);
                    int g = qGreen(scanLine[x]);
                    int b = qBlue(scanLine[x]);
                    
                    // 应用亮度和对比度
                    r = qBound(0, (r - 128) * (contrast_ + 100) / 100 + 128 + brightness_, 255);
                    g = qBound(0, (g - 128) * (contrast_ + 100) / 100 + 128 + brightness_, 255);
                    b = qBound(0, (b - 128) * (contrast_ + 100) / 100 + 128 + brightness_, 255);
                    
                    scanLine[x] = qRgb(r, g, b);
                }
            }
        }
        
        // 转换为灰度
        if (enableGrayscale_) {
            for (int y = 0; y < image.height(); ++y) {
                QRgb* scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));
                for (int x = 0; x < image.width(); ++x) {
                    int gray = qGray(scanLine[x]);
                    scanLine[x] = qRgb(gray, gray, gray);
                }
            }
        }
    }
    
    void updatePerformanceStats(qint64 frameTime) {
        totalFrames_++;
        totalProcessingTime_ += frameTime;
        minFrameTime_ = qMin(minFrameTime_, frameTime);
        maxFrameTime_ = qMax(maxFrameTime_, frameTime);
    }
    
signals:
    void performanceUpdated(const PerformanceStats& stats);
};
```

### 4. 应用安装 API (installation_proxy)

#### 4.1 应用管理服务

##### instproxy_client_start_service()
```c
instproxy_error_t instproxy_client_start_service(idevice_t device,
                                                instproxy_client_t *client,
                                                const char *label);
```

**功能描述**: 启动应用安装代理服务

##### instproxy_browse()
```c
instproxy_error_t instproxy_browse(instproxy_client_t client,
                                  plist_t client_options,
                                  plist_t *result);
```

**功能描述**: 浏览已安装的应用程序

**参数说明**:
- `client`: 安装代理客户端句柄
- `client_options`: 浏览选项（可为NULL）
- `result`: 输出参数，应用程序列表

**使用示例**:
```cpp
instproxy_client_t instproxy = NULL;
instproxy_error_t error = instproxy_client_start_service(device, &instproxy, "phone-linkc");
if (error == INSTPROXY_E_SUCCESS) {
    plist_t apps = NULL;
    error = instproxy_browse(instproxy, NULL, &apps);
    if (error == INSTPROXY_E_SUCCESS) {
        // 解析应用程序列表
        uint32_t app_count = plist_array_get_size(apps);
        for (uint32_t i = 0; i < app_count; i++) {
            plist_t app = plist_array_get_item(apps, i);
            
            // 获取应用信息
            plist_t bundle_id_node = plist_dict_get_item(app, "CFBundleIdentifier");
            plist_t app_name_node = plist_dict_get_item(app, "CFBundleDisplayName");
            
            if (bundle_id_node && app_name_node) {
                char *bundle_id = NULL, *app_name = NULL;
                plist_get_string_val(bundle_id_node, &bundle_id);
                plist_get_string_val(app_name_node, &app_name);
                
                qDebug() << "应用:" << app_name << "包名:" << bundle_id;
                
                free(bundle_id);
                free(app_name);
            }
        }
        plist_free(apps);
    }
    instproxy_client_free(instproxy);
}
```

##### instproxy_install()
```c
instproxy_error_t instproxy_install(instproxy_client_t client,
                                   const char *pkg_path,
                                   plist_t client_options,
                                   instproxy_status_cb_t status_cb,
                                   void *user_data);
```

**功能描述**: 安装应用程序

**参数说明**:
- `client`: 安装代理客户端句柄
- `pkg_path`: IPA文件路径
- `client_options`: 安装选项
- `status_cb`: 状态回调函数
- `user_data`: 用户数据

### 5. 文件传输 API (afc)

#### 5.1 文件系统访问

##### afc_client_start_service()
```c
afc_error_t afc_client_start_service(idevice_t device,
                                    afc_client_t *client,
                                    const char *label);
```

**功能描述**: 启动AFC文件传输服务

##### afc_read_directory()
```c
afc_error_t afc_read_directory(afc_client_t client,
                              const char *path,
                              char ***list);
```

**功能描述**: 读取目录内容

**使用示例**:
```cpp
afc_client_t afc = NULL;
afc_error_t error = afc_client_start_service(device, &afc, "phone-linkc");
if (error == AFC_E_SUCCESS) {
    char **list = NULL;
    error = afc_read_directory(afc, "/", &list);
    if (error == AFC_E_SUCCESS) {
        int i = 0;
        while (list[i]) {
            qDebug() << "文件/目录:" << list[i];
            i++;
        }
        afc_dictionary_free(list);
    }
    afc_client_free(afc);
}
```

##### afc_file_open()
```c
afc_error_t afc_file_open(afc_client_t client,
                         const char *filename,
                         afc_file_mode_t file_mode,
                         uint64_t *handle);
```

**功能描述**: 打开文件

**文件模式**:
- `AFC_FOPEN_RDONLY`: 只读
- `AFC_FOPEN_WRONLY`: 只写
- `AFC_FOPEN_RDWR`: 读写
- `AFC_FOPEN_APPEND`: 追加

##### afc_file_write()
```c
afc_error_t afc_file_write(afc_client_t client,
                          uint64_t handle,
                          const char *data,
                          uint32_t length,
                          uint32_t *bytes_written);
```

**功能描述**: 写入文件数据

##### afc_file_read()
```c
afc_error_t afc_file_read(afc_client_t client,
                         uint64_t handle,
                         char *data,
                         uint32_t length,
                         uint32_t *bytes_read);
```

**功能描述**: 读取文件数据

#### 5.2 文件操作进阶

##### afc_get_file_info()
```c
afc_error_t afc_get_file_info(afc_client_t client,
                              const char *path,
                              char ***file_info);
```

**功能描述**: 获取文件详细信息

**参数说明**:
- `client`: AFC客户端句柄
- `path`: 文件路径
- `file_info`: 输出参数，文件信息键值对数组

**返回值**:
- `AFC_E_SUCCESS`: 获取成功

**使用示例**:
```cpp
// 获取文件大小和修改时间
qint64 getFileSize(afc_client_t afc, const QString& filePath) {
    char **info = nullptr;
    qint64 size = 0;
    
    if (afc_get_file_info(afc, filePath.toUtf8().constData(), &info) == AFC_E_SUCCESS) {
        for (int i = 0; info[i]; i += 2) {
            if (QString(info[i]) == "st_size" && info[i+1]) {
                size = QString(info[i+1]).toLongLong();
                break;
            }
        }
        afc_dictionary_free(info);
    }
    
    return size;
}

QDateTime getFileModificationTime(afc_client_t afc, const QString& filePath) {
    char **info = nullptr;
    QDateTime time;
    
    if (afc_get_file_info(afc, filePath.toUtf8().constData(), &info) == AFC_E_SUCCESS) {
        for (int i = 0; info[i]; i += 2) {
            if (QString(info[i]) == "st_mtime" && info[i+1]) {
                time = QDateTime::fromSecsSinceEpoch(QString(info[i+1]).toLongLong());
                break;
            }
        }
        afc_dictionary_free(info);
    }
    
    return time;
}
```

##### afc_make_directory()
```c
afc_error_t afc_make_directory(afc_client_t client,
                              const char *path);
```

**功能描述**: 创建目录

**参数说明**:
- `client`: AFC客户端句柄
- `path`: 要创建的目录路径

**使用示例**:
```cpp
// 递归创建目录结构
bool createDirectoryRecursively(afc_client_t afc, const QString& path) {
    QStringList components = path.split('/', Qt::SkipEmptyParts);
    QString currentPath;
    
    for (const QString& component : components) {
        if (!currentPath.isEmpty()) {
            currentPath += "/";
        }
        currentPath += component;
        
        // 检查目录是否存在
        char **list = nullptr;
        QString parentPath = currentPath.left(currentPath.lastIndexOf('/'));
        if (!parentPath.isEmpty()) {
            if (afc_read_directory(afc, parentPath.toUtf8().constData(), &list) == AFC_E_SUCCESS) {
                bool exists = false;
                for (int i = 0; list[i]; i++) {
                    if (QString(list[i]) == component) {
                        exists = true;
                        break;
                    }
                }
                afc_dictionary_free(list);
                
                if (!exists) {
                    // 创建目录
                    if (afc_make_directory(afc, currentPath.toUtf8().constData()) != AFC_E_SUCCESS) {
                        return false;
                    }
                }
            } else {
                // 父目录不存在，尝试创建
                return false;
            }
        } else {
            // 根目录或一级目录
            if (afc_make_directory(afc, currentPath.toUtf8().constData()) != AFC_E_SUCCESS) {
                // 可能已存在，忽略错误
            }
        }
    }
    
    return true;
}
```

##### afc_remove_path()
```c
afc_error_t afc_remove_path(afc_client_t client,
                           const char *path);
```

**功能描述**: 删除文件或目录（递归删除目录）

**参数说明**:
- `client`: AFC客户端句柄
- `path`: 要删除的路径

**使用示例**:
```cpp
// 安全删除文件或目录
bool removePathSafely(afc_client_t afc, const QString& path) {
    // 先检查是否为目录
    char **info = nullptr;
    bool isDirectory = false;
    
    if (afc_get_file_info(afc, path.toUtf8().constData(), &info) == AFC_E_SUCCESS) {
        for (int i = 0; info[i]; i += 2) {
            if (QString(info[i]) == "st_ifmt" && info[i+1]) {
                isDirectory = (QString(info[i+1]) == "S_IFDIR");
                break;
            }
        }
        afc_dictionary_free(info);
    }
    
    if (isDirectory) {
        // 先清空目录
        char **list = nullptr;
        if (afc_read_directory(afc, path.toUtf8().constData(), &list) == AFC_E_SUCCESS) {
            for (int i = 0; list[i]; i++) {
                QString item = QString(list[i]);
                if (item != "." && item != "..") {
                    QString itemPath = path + "/" + item;
                    if (!removePathSafely(afc, itemPath)) {
                        afc_dictionary_free(list);
                        return false;
                    }
                }
            }
            afc_dictionary_free(list);
        }
    }
    
    // 删除文件或空目录
    return afc_remove_path(afc, path.toUtf8().constData()) == AFC_E_SUCCESS;
}
```

##### afc_rename_path()
```c
afc_error_t afc_rename_path(afc_client_t client,
                           const char *old_path,
                           const char *new_path);
```

**功能描述**: 重命名文件或目录

**参数说明**:
- `client`: AFC客户端句柄
- `old_path`: 原路径
- `new_path`: 新路径

#### 5.3 应用沙箱访问

iOS应用使用沙箱机制，需要通过house_arrest服务访问应用专用目录。

##### house_arrest_client_start_service()
```c
house_arrest_error_t house_arrest_client_start_service(idevice_t device,
                                                      house_arrest_client_t *client,
                                                      const char *label);
```

**功能描述**: 启动house_arrest服务（应用沙箱访问）

##### house_arrest_send_request()
```c
house_arrest_error_t house_arrest_send_request(house_arrest_client_t client,
                                              const char *bundle_id,
                                              plist_t *dict);
```

**功能描述**: 请求访问应用沙箱

**参数说明**:
- `client`: house_arrest客户端句柄
- `bundle_id`: 应用Bundle ID
- `dict`: 输出参数，返回的响应信息

**使用示例**:
```cpp
// 访问应用文档目录
bool accessAppDocuments(const QString& udid, const QString& bundleId) {
    idevice_t device = getDeviceConnection(udid);
    if (!device) {
        return false;
    }
    
    // 启动house_arrest服务
    house_arrest_client_t house_arrest = nullptr;
    if (house_arrest_client_start_service(device, &house_arrest, "phone-linkc") != HOUSE_ARREST_E_SUCCESS) {
        return false;
    }
    
    // 请求访问应用沙箱
    plist_t dict = nullptr;
    if (house_arrest_send_request(house_arrest, bundleId.toUtf8().constData(), &dict) != HOUSE_ARREST_E_SUCCESS) {
        house_arrest_client_free(house_arrest);
        return false;
    }
    
    // 检查是否成功
    bool success = false;
    if (dict) {
        plist_t status = plist_dict_get_item(dict, "Status");
        if (status) {
            char *statusStr = nullptr;
            plist_get_string_val(status, &statusStr);
            success = (QString(statusStr) == "Complete");
            free(statusStr);
        }
        plist_free(dict);
    }
    
    if (success) {
        // 现在可以通过AFC服务访问应用沙箱
        afc_client_t afc = nullptr;
        if (house_arrest_get_afc_client(house_arrest, &afc) == HOUSE_ARREST_E_SUCCESS) {
            // 现在可以访问应用文档目录
            char **list = nullptr;
            if (afc_read_directory(afc, "Documents", &list) == AFC_E_SUCCESS) {
                qDebug() << "应用文档目录内容:";
                for (int i = 0; list[i]; i++) {
                    qDebug() << "  " << list[i];
                }
                afc_dictionary_free(list);
            }
            
            afc_client_free(afc);
        }
    }
    
    house_arrest_client_free(house_arrest);
    return success;
}

// 更完整的应用文件管理器
class AppFileManager {
private:
    QString currentUdid_;
    QString currentBundleId_;
    house_arrest_client_t house_arrest_;
    afc_client_t afc_;
    bool connected_;
    
public:
    AppFileManager() : house_arrest_(nullptr), afc_(nullptr), connected_(false) {}
    
    ~AppFileManager() {
        disconnect();
    }
    
    bool connect(const QString& udid, const QString& bundleId) {
        disconnect(); // 清理之前连接
        
        idevice_t device = getDeviceConnection(udid);
        if (!device) {
            return false;
        }
        
        // 启动house_arrest服务
        if (house_arrest_client_start_service(device, &house_arrest_, "phone-linkc") != HOUSE_ARREST_E_SUCCESS) {
            return false;
        }
        
        // 请求访问应用沙箱
        plist_t dict = nullptr;
        if (house_arrest_send_request(house_arrest_, bundleId.toUtf8().constData(), &dict) != HOUSE_ARREST_E_SUCCESS) {
            house_arrest_client_free(house_arrest_);
            house_arrest_ = nullptr;
            return false;
        }
        
        // 检查是否成功
        bool success = false;
        if (dict) {
            plist_t status = plist_dict_get_item(dict, "Status");
            if (status) {
                char *statusStr = nullptr;
                plist_get_string_val(status, &statusStr);
                success = (QString(statusStr) == "Complete");
                free(statusStr);
            }
            plist_free(dict);
        }
        
        if (!success) {
            house_arrest_client_free(house_arrest_);
            house_arrest_ = nullptr;
            return false;
        }
        
        // 获取AFC客户端
        if (house_arrest_get_afc_client(house_arrest_, &afc_) != HOUSE_ARREST_E_SUCCESS) {
            house_arrest_client_free(house_arrest_);
            house_arrest_ = nullptr;
            return false;
        }
        
        currentUdid_ = udid;
        currentBundleId_ = bundleId;
        connected_ = true;
        
        return true;
    }
    
    void disconnect() {
        if (afc_) {
            afc_client_free(afc_);
            afc_ = nullptr;
        }
        
        if (house_arrest_) {
            house_arrest_client_free(house_arrest_);
            house_arrest_ = nullptr;
        }
        
        connected_ = false;
    }
    
    QStringList listDirectory(const QString& path) {
        QStringList files;
        
        if (!connected_) {
            return files;
        }
        
        char **list = nullptr;
        if (afc_read_directory(afc_, path.toUtf8().constData(), &list) == AFC_E_SUCCESS) {
            for (int i = 0; list[i]; i++) {
                QString filename = QString(list[i]);
                if (filename != "." && filename != "..") {
                    files << filename;
                }
            }
            afc_dictionary_free(list);
        }
        
        return files;
    }
    
    bool uploadFile(const QString& localPath, const QString& remotePath) {
        if (!connected_) {
            return false;
        }
        
        QFile localFile(localPath);
        if (!localFile.open(QIODevice::ReadOnly)) {
            return false;
        }
        
        uint64_t handle = 0;
        if (afc_file_open(afc_, remotePath.toUtf8().constData(), AFC_FOPEN_WRONLY, &handle) != AFC_E_SUCCESS) {
            return false;
        }
        
        const int BUFFER_SIZE = 65536;
        char buffer[BUFFER_SIZE];
        
        while (!localFile.atEnd()) {
            qint64 bytesRead = localFile.read(buffer, BUFFER_SIZE);
            if (bytesRead <= 0) break;
            
            uint32_t bytesWritten = 0;
            if (afc_file_write(afc_, handle, buffer, bytesRead, &bytesWritten) != AFC_E_SUCCESS ||
                bytesWritten != static_cast<uint32_t>(bytesRead)) {
                afc_file_close(afc_, handle);
                return false;
            }
        }
        
        afc_file_close(afc_, handle);
        return true;
    }
    
    bool downloadFile(const QString& remotePath, const QString& localPath) {
        if (!connected_) {
            return false;
        }
        
        QFile localFile(localPath);
        if (!localFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        
        uint64_t handle = 0;
        if (afc_file_open(afc_, remotePath.toUtf8().constData(), AFC_FOPEN_RDONLY, &handle) != AFC_E_SUCCESS) {
            return false;
        }
        
        const int BUFFER_SIZE = 65536;
        char buffer[BUFFER_SIZE];
        
        while (true) {
            uint32_t bytesRead = 0;
            if (afc_file_read(afc_, handle, buffer, BUFFER_SIZE, &bytesRead) != AFC_E_SUCCESS || bytesRead == 0) {
                break;
            }
            
            localFile.write(buffer, bytesRead);
        }
        
        afc_file_close(afc_, handle);
        return true;
    }
};
```

### 6. 系统日志 API (syslog_relay)

#### 6.1 日志监控

##### syslog_relay_client_start_service()
```c
syslog_relay_error_t syslog_relay_client_start_service(idevice_t device,
                                                      syslog_relay_client_t *client,
                                                      const char *label);
```

**功能描述**: 启动系统日志中继服务

##### syslog_relay_receive()
```c
syslog_relay_error_t syslog_relay_receive(syslog_relay_client_t client,
                                         char **data,
                                         uint32_t *size);
```

**功能描述**: 接收系统日志消息

**使用示例**:
```cpp
syslog_relay_client_t syslog = NULL;
syslog_relay_error_t error = syslog_relay_client_start_service(device, &syslog, "phone-linkc");
if (error == SYSLOG_RELAY_E_SUCCESS) {
    // 持续监控日志
    while (true) {
        char *data = NULL;
        uint32_t size = 0;
        
        error = syslog_relay_receive(syslog, &data, &size);
        if (error == SYSLOG_RELAY_E_SUCCESS) {
            QString logMessage = QString::fromUtf8(data, size);
            qDebug() << "系统日志:" << logMessage;
            free(data);
        }
        
        // 检查退出条件
        if (shouldStop()) break;
    }
    syslog_relay_client_free(syslog);
}
```

#### 6.2 日志过滤与分析

iOS系统日志包含大量信息，实际应用中需要过滤和分析。以下是一个完整的日志监控系统实现。

**高级日志监控实现**:

```cpp
// 日志条目结构
struct LogEntry {
    QDateTime timestamp;    // 时间戳
    QString level;          // 日志级别 (Error, Warning, Notice, Info, Debug)
    QString process;        // 进程名称
    QString processId;      // 进程ID
    QString message;        // 日志消息
    QString subsystem;      // 子系统
    QString category;       // 类别
    QString rawText;        // 原始日志文本
    
    bool isValid() const {
        return !timestamp.isNull() && !level.isEmpty() && !process.isEmpty();
    }
};

// 高级日志监控器
class AdvancedLogMonitor : public QObject {
    Q_OBJECT
    
private:
    idevice_t device_;
    QThread* logThread_;
    bool monitoring_;
    
    // 过滤条件
    QStringList levelFilters_;
    QStringList processFilters_;
    QStringList subsystemFilters_;
    QStringList messageFilters_;
    
    // 统计数据
    QMap<QString, int> levelCount_;
    QMap<QString, int> processCount_;
    QDateTime startTime_;
    int totalLogCount_;
    
public:
    AdvancedLogMonitor(QObject* parent = nullptr) 
        : QObject(parent), device_(nullptr), logThread_(nullptr), monitoring_(false), totalLogCount_(0) {
    }
    
    ~AdvancedLogMonitor() {
        stopMonitoring();
    }
    
    // 设置日志级别过滤器
    void setLevelFilters(const QStringList& levels) {
        levelFilters_ = levels;
    }
    
    // 设置进程过滤器
    void setProcessFilters(const QStringList& processes) {
        processFilters_ = processes;
    }
    
    // 设置消息过滤器（正则表达式）
    void setMessageFilters(const QStringList& patterns) {
        messageFilters_ = patterns;
    }
    
    // 清除所有过滤器
    void clearFilters() {
        levelFilters_.clear();
        processFilters_.clear();
        subsystemFilters_.clear();
        messageFilters_.clear();
    }
    
    // 开始监控
    bool startMonitoring(idevice_t device) {
        if (monitoring_) {
            return true;
        }
        
        device_ = device;
        if (!device_) {
            emit errorOccurred("无效的设备句柄");
            return false;
        }
        
        // 重置统计数据
        levelCount_.clear();
        processCount_.clear();
        startTime_ = QDateTime::currentDateTime();
        totalLogCount_ = 0;
        
        // 创建工作线程
        logThread_ = QThread::create([this]() {
            runLogMonitoring();
        });
        
        connect(logThread_, &QThread::finished, logThread_, &QThread::deleteLater);
        logThread_->start();
        
        monitoring_ = true;
        emit monitoringStarted();
        return true;
    }
    
    // 停止监控
    void stopMonitoring() {
        if (!monitoring_) {
            return;
        }
        
        monitoring_ = false;
        
        if (logThread_) {
            logThread_->quit();
            logThread_->wait(3000);
            if (logThread_->isRunning()) {
                logThread_->terminate();
                logThread_->wait(1000);
            }
        }
        
        emit monitoringStopped();
        emit statisticsReady(getStatistics());
    }
    
    // 获取统计信息
    QVariantMap getStatistics() const {
        QVariantMap stats;
        
        stats["startTime"] = startTime_;
        stats["totalLogCount"] = totalLogCount_;
        stats["duration"] = startTime_.secsTo(QDateTime::currentDateTime());
        stats["levelCounts"] = QVariantMap::fromMap(levelCount_);
        stats["processCounts"] = QVariantMap::fromMap(processCount_);
        
        if (totalLogCount_ > 0) {
            QMap<QString, int> levelPercents;
            for (auto it = levelCount_.begin(); it != levelCount_.end(); ++it) {
                levelPercents[it.key()] = it.value() * 100 / totalLogCount_;
            }
            stats["levelPercents"] = QVariantMap::fromMap(levelPercents);
        }
        
        return stats;
    }

signals:
    void logEntryReceived(const LogEntry& entry);
    void monitoringStarted();
    void monitoringStopped();
    void errorOccurred(const QString& error);
    void statisticsReady(const QVariantMap& stats);

private:
    void runLogMonitoring() {
        syslog_relay_client_t syslog = nullptr;
        if (syslog_relay_client_start_service(device_, &syslog, "phone-linkc") != SYSLOG_RELAY_E_SUCCESS) {
            emit errorOccurred("无法启动系统日志服务");
            return;
        }
        
        while (monitoring_) {
            char *data = nullptr;
            uint32_t size = 0;
            
            // 带超时的接收，避免无限阻塞
            syslog_relay_error_t error = syslog_relay_receive_with_timeout(syslog, &data, &size, 1000);
            
            if (error == SYSLOG_RELAY_E_SUCCESS && data && size > 0) {
                QString logData = QString::fromUtf8(data, size);
                free(data);
                
                // 解析日志条目
                LogEntry entry = parseLogEntry(logData);
                if (entry.isValid()) {
                    // 应用过滤器
                    if (shouldFilterEntry(entry)) {
                        updateStatistics(entry);
                        emit logEntryReceived(entry);
                    }
                }
            } else if (error == SYSLOG_RELAY_E_MUX_ERROR) {
                emit errorOccurred("系统日志服务连接中断");
                break;
            }
        }
        
        syslog_relay_client_free(syslog);
    }
    
    LogEntry parseLogEntry(const QString& rawLog) {
        LogEntry entry;
        entry.rawText = rawLog.trimmed();
        
        // iOS日志格式示例:
        // 2023-11-15 14:30:45.123 MyAwesomeApp[1234:5678] Error: Something went wrong
        // 或者:
        // Nov 15 14:30:45 iPhone MyAwesomeApp[1234] <Error>: Something went wrong
        
        QRegularExpression regex(
            R"((\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\.\d+)\s+(\w+)\[(\d+)\](:\d+)?\s+<(\w+)>:\s+(.+))");
        
        QRegularExpressionMatch match = regex.match(entry.rawText);
        
        if (match.hasMatch()) {
            // 解析时间戳
            entry.timestamp = QDateTime::fromString(match.captured(1), "yyyy-MM-dd hh:mm:ss.zzz");
            
            // 解析进程信息
            entry.process = match.captured(2);
            entry.processId = match.captured(3);
            
            // 解析级别和消息
            entry.level = match.captured(5);
            entry.message = match.captured(6);
            
            // 尝试从进程名称中提取子系统信息
            if (entry.process.contains(".")) {
                QStringList parts = entry.process.split(".");
                if (parts.size() >= 2) {
                    entry.subsystem = parts[0];
                    entry.category = parts[1];
                }
            }
        } else {
            // 尝试另一种常见格式
            QRegularExpression regex2(
                R"((\w+\s+\d+\s+\d{2}:\d{2}:\d{2})\s+(\w+)\s+(\w+)\[(\d+)\]\s+<(\w+)>:\s+(.+))");
            
            match = regex2.match(entry.rawText);
            if (match.hasMatch()) {
                // 解析时间戳（需要加上当前年份）
                QString timeStr = match.captured(1);
                QDateTime dt = QDateTime::fromString(QString("%1 %2").arg(QDate::currentDate().year()).arg(timeStr), "yyyy MMM d hh:mm:ss");
                entry.timestamp = dt;
                
                entry.process = match.captured(3);
                entry.processId = match.captured(4);
                entry.level = match.captured(5);
                entry.message = match.captured(6);
            }
        }
        
        return entry;
    }
    
    bool shouldFilterEntry(const LogEntry& entry) {
        // 检查级别过滤器
        if (!levelFilters_.isEmpty() && !levelFilters_.contains(entry.level, Qt::CaseInsensitive)) {
            return false;
        }
        
        // 检查进程过滤器
        if (!processFilters_.isEmpty()) {
            bool match = false;
            for (const QString& filter : processFilters_) {
                if (entry.process.contains(filter, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                return false;
            }
        }
        
        // 检查消息过滤器
        if (!messageFilters_.isEmpty()) {
            bool match = false;
            for (const QString& pattern : messageFilters_) {
                QRegularExpression regex(pattern);
                if (regex.isValid() && entry.message.contains(regex)) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                return false;
            }
        }
        
        return true;
    }
    
    void updateStatistics(const LogEntry& entry) {
        totalLogCount_++;
        
        // 更新级别计数
        levelCount_[entry.level]++;
        
        // 更新进程计数
        processCount_[entry.process]++;
    }
};

// 日志可视化分析器
class LogVisualizer : public QObject {
    Q_OBJECT
    
private:
    QMap<QDateTime, int> timelineData_;      // 时间线数据
    QMap<QString, int> levelTimeline_[24];   // 按小时的级别分布
    QMap<QString, QMap<int, int>> levelByHour_; // 按小时的级别统计
    
public:
    // 添加日志条目到可视化数据
    void addLogEntry(const LogEntry& entry) {
        // 更新时间线数据（按分钟聚合）
        QDateTime minuteKey = entry.timestamp.addSecs(-entry.timestamp.time().second());
        timelineData_[minuteKey]++;
        
        // 更新按小时的级别分布
        int hour = entry.timestamp.time().hour();
        levelTimeline_[hour][entry.level]++;
        levelByHour_[entry.level][hour]++;
    }
    
    // 获取时间线数据（用于图表显示）
    QVariantMap getTimelineData() const {
        QVariantMap result;
        
        // 转换为图表友好格式
        QList<QVariant> timestamps;
        QList<QVariant> counts;
        
        for (auto it = timelineData_.begin(); it != timelineData_.end(); ++it) {
            timestamps.append(it.key().toMSecsSinceEpoch());
            counts.append(it.value());
        }
        
        result["timestamps"] = timestamps;
        result["counts"] = counts;
        
        return result;
    }
    
    // 获取按小时的级别分布（用于堆叠柱状图）
    QVariantMap getHourlyLevelDistribution() const {
        QVariantMap result;
        
        // 准备数据结构
        QMap<QString, QList<QVariant>> levelData;
        QStringList levels = {"Error", "Warning", "Notice", "Info", "Debug"};
        
        // 初始化每个级别的24小时数据
        for (const QString& level : levels) {
            levelData[level] = QList<QVariant>(24, 0);
        }
        
        // 填充实际数据
        for (int hour = 0; hour < 24; hour++) {
            for (auto it = levelTimeline_[hour].begin(); it != levelTimeline_[hour].end(); ++it) {
                QString level = it.key();
                int count = it.value();
                
                if (levelData.contains(level)) {
                    levelData[level][hour] = count;
                }
            }
        }
        
        // 转换为输出格式
        for (auto it = levelData.begin(); it != levelData.end(); ++it) {
            result[it.key()] = it.value();
        }
        
        return result;
    }
    
    // 清除所有数据
    void clear() {
        timelineData_.clear();
        for (int i = 0; i < 24; i++) {
            levelTimeline_[i].clear();
        }
        levelByHour_.clear();
    }
};
```

## 属性列表 (plist) API

### plist_t 数据类型操作

#### 基础操作函数

##### plist_new_dict()
```c
plist_t plist_new_dict(void);
```
**功能**: 创建新的字典类型plist

##### plist_new_array()
```c
plist_t plist_new_array(void);
```
**功能**: 创建新的数组类型plist

##### plist_new_string()
```c
plist_t plist_new_string(const char *val);
```
**功能**: 创建字符串类型plist节点

##### plist_get_string_val()
```c
void plist_get_string_val(plist_t node, char **val);
```
**功能**: 获取字符串值

##### plist_dict_get_item()
```c
plist_t plist_dict_get_item(plist_t node, const char *key);
```
**功能**: 从字典中获取指定键的值

##### plist_dict_set_item()
```c
void plist_dict_set_item(plist_t node, const char *key, plist_t item);
```
**功能**: 向字典中设置键值对

##### plist_free()
```c
void plist_free(plist_t plist);
```
**功能**: 释放plist节点及其子节点的内存

**注意**: 此函数用于释放 plist_t 节点，不要用于释放 API 分配的字符串或数据缓冲区。

##### plist_mem_free()
```c
void plist_mem_free(void* ptr);
```
**功能**: 释放 libplist API 分配的内存

**适用场景**:
- `plist_to_xml()` 分配的 XML 字符串
- `plist_to_bin()` 分配的二进制数据
- `plist_get_key_val()` 返回的键名
- `plist_get_string_val()` 返回的字符串值
- `plist_get_data_val()` 返回的数据缓冲区

**版本说明**:
- libplist v2.3.0+ 中引入，替代旧的 `plist_to_xml_free()` 函数
- 更通用，可释放多种 API 分配的内存
- phone-linkc 已适配新版本 API

**参数说明**:
- `ptr`: 要释放的内存指针

**重要提示**:
- 不要使用标准 `free()` 释放 libplist 分配的内存（可能导致跨模块释放问题）
- 不要使用此函数释放 `plist_t` 节点（应使用 `plist_free()`）

### 使用示例

#### 基础示例

```cpp
// 创建配置字典
plist_t options = plist_new_dict();
plist_t app_type = plist_new_string("User");
plist_dict_set_item(options, "ApplicationType", app_type);

// 从字典读取值
plist_t value = plist_dict_get_item(device_info, "DeviceName");
if (value) {
    char *device_name = NULL;
    plist_get_string_val(value, &device_name);
    qDebug() << "设备名称:" << device_name;
    free(device_name);
}

// 释放资源
plist_free(options);
```

### 高级plist操作

#### 复杂数据结构处理

```cpp
// 创建复杂配置选项
plist_t createAdvancedAppInstallOptions() {
    // 主配置字典
    plist_t options = plist_new_dict();
    
    // 设置应用类型
    plist_dict_set_item(options, "ApplicationType", plist_new_string("User"));
    
    // 创建权限数组
    plist_t permissions = plist_new_array();
    plist_array_append_item(permissions, plist_new_string("photos"));
    plist_array_append_item(permissions, plist_new_string("camera"));
    plist_dict_set_item(options, "RequestedPermissions", permissions);
    
    // 设置元数据
    plist_t metadata = plist_new_dict();
    plist_dict_set_item(metadata, "BundleID", plist_new_string("com.example.app"));
    plist_dict_set_item(metadata, "Version", plist_new_string("1.0.0"));
    plist_dict_set_item(metadata, "ShortVersion", plist_new_string("1.0"));
    plist_dict_set_item(options, "Metadata", metadata);
    
    // 添加标志
    plist_t flags = plist_new_uint(1);  // ITUNES_FLAGS_INSTALL
    plist_dict_set_item(options, "iTunesFlags", flags);
    
    return options;
}
```

#### plist序列化和反序列化

```cpp
// 将plist保存到文件（新版本 API）
bool savePlistToFile(plist_t plist, const QString& filePath) {
    if (!plist) {
        return false;
    }
    
    char *buffer = NULL;
    uint32_t length = 0;
    
    // 转换为XML格式 - 使用新版 API
    plist_err_t err = plist_to_xml(plist, &buffer, &length);
    
    if (err != PLIST_ERR_SUCCESS || !buffer) {
        qWarning() << "plist_to_xml 失败，错误代码:" << err;
        return false;
    }
    
    QFile file(filePath);
    bool success = false;
    if (file.open(QIODevice::WriteOnly)) {
        success = (file.write(buffer, length) == length);
        file.close();
    }
    
    // 使用新的内存释放函数
    plist_mem_free(buffer);  // libplist v2.3.0+
    
    return success;
}

// 从文件加载plist
plist_t loadPlistFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return NULL;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    plist_t plist = NULL;
    plist_from_xml(data.constData(), data.length(), &plist);
    
    return plist;
}
```

#### phone-linkc项目中的实用函数

```cpp
// 辅助函数：从锁中获取字符串值（更新版本）
QString getLockdowndStringValue(lockdownd_client_t client, const char* domain, const char* key) {
    plist_t value = NULL;
    QString result;
    
    if (lockdownd_get_value(client, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (plist_get_node_type(value) == PLIST_STRING) {
            char *str_value = NULL;
            plist_get_string_val(value, &str_value);
            if (str_value) {
                result = QString::fromUtf8(str_value);
                // 使用 plist_mem_free 释放 plist API 分配的字符串
                plist_mem_free(str_value);  // libplist v2.3.0+
            }
        }
        plist_free(value);  // 释放 plist 节点本身
    }
    
    return result;
}

// 辅助函数：从锁中获取整数值
uint64_t getLockdowndUIntValue(lockdownd_client_t client, const char* domain, const char* key) {
    plist_t value = NULL;
    uint64_t result = 0;
    
    if (lockdownd_get_value(client, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (plist_get_node_type(value) == PLIST_UINT) {
            plist_get_uint_val(value, &result);
        }
        plist_free(value);
    }
    
    return result;
}

// 辅助函数：从锁中获取日期值
QDateTime getLockdowndDateValue(lockdownd_client_t client, const char* domain, const char* key) {
    plist_t value = NULL;
    QDateTime result;
    
    if (lockdownd_get_value(client, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (plist_get_node_type(value) == PLIST_DATE) {
            int32_t secs = 0, usecs = 0;
            plist_get_date_val(value, &secs, &usecs);
            result = QDateTime::fromSecsSinceEpoch(secs);
        }
        plist_free(value);
    }
    
    return result;
}
```

## 错误处理

### 错误代码定义

#### 通用错误代码
- `IDEVICE_E_SUCCESS = 0`: 操作成功
- `IDEVICE_E_INVALID_ARG = -1`: 参数无效
- `IDEVICE_E_UNKNOWN_ERROR = -2`: 未知错误
- `IDEVICE_E_NO_DEVICE = -3`: 设备未找到
- `IDEVICE_E_NOT_ENOUGH_DATA = -4`: 数据不足
- `IDEVICE_E_BAD_HEADER = -5`: 头部错误
- `IDEVICE_E_SSL_ERROR = -6`: SSL错误

#### Lockdown错误代码
- `LOCKDOWN_E_SUCCESS = 0`: 成功
- `LOCKDOWN_E_INVALID_ARG = -1`: 参数无效
- `LOCKDOWN_E_INVALID_CONF = -2`: 配置无效
- `LOCKDOWN_E_PLIST_ERROR = -3`: plist错误
- `LOCKDOWN_E_PAIRING_FAILED = -4`: 配对失败
- `LOCKDOWN_E_SSL_ERROR = -5`: SSL错误
- `LOCKDOWN_E_DICT_ERROR = -6`: 字典错误
- `LOCKDOWN_E_NOT_ENOUGH_DATA = -7`: 数据不足
- `LOCKDOWN_E_MUX_ERROR = -8`: 多路复用错误

### 错误处理最佳实践

```cpp
// 统一错误处理函数
QString getErrorMessage(idevice_error_t error) {
    switch (error) {
        case IDEVICE_E_SUCCESS:
            return "操作成功";
        case IDEVICE_E_INVALID_ARG:
            return "参数无效";
        case IDEVICE_E_NO_DEVICE:
            return "设备未找到";
        case IDEVICE_E_TIMEOUT:
            return "操作超时";
        case IDEVICE_E_SSL_ERROR:
            return "SSL连接错误";
        default:
            return QString("未知错误: %1").arg(error);
    }
}

// 使用示例
idevice_error_t error = idevice_new(&device, udid);
if (error != IDEVICE_E_SUCCESS) {
    qWarning() << "设备连接失败:" << getErrorMessage(error);
    return false;
}
```

### 故障排除指南

#### 常见连接问题

**问题1: 设备连接失败 - IDEVICE_E_SSL_ERROR**

```cpp
// 解决方案：重置信任关系并重新尝试连接
bool resetAndReconnect(const QString& udid) {
    // 1. 释放现有连接
    idevice_free(device);
    device = nullptr;
    
    // 2. 重启usbmuxd服务（需要管理员权限）
    #ifdef Q_OS_WIN
    QProcess::execute("net stop usbmuxd");
    QProcess::execute("net start usbmuxd");
    #endif
    
    // 3. 检查设备是否已解锁并信任此电脑
    if (!QMessageBox::question(nullptr, "设备连接问题", 
                              "请检查设备是否已解锁并信任此电脑，然后重试")) {
        return false;
    }
    
    // 4. 尝试重新连接
    return idevice_new(&device, udid.toUtf8().constData()) == IDEVICE_E_SUCCESS;
}
```

**问题2: lockdownd服务连接失败**

```cpp
// 解决方案：尝试不同的客户端创建方法
bool tryAlternativeLockdowndConnection(idevice_t device) {
    lockdownd_client_t client = nullptr;
    
    // 方法1：使用握手方式创建客户端
    if (lockdownd_client_new_with_handshake(device, &client, "phone-linkc") == LOCKDOWN_E_SUCCESS) {
        return client;
    }
    
    // 方法2：尝试不使用握手方式
    if (lockdownd_client_new(device, &client, "phone-linkc") == LOCKDOWN_E_SUCCESS) {
        // 手动启动会话
        uint16_t port = 0;
        if (lockdownd_start_session(client, NULL, NULL, &port) == LOCKDOWN_E_SUCCESS) {
            return client;
        }
    }
    
    return nullptr;
}
```

**问题3: 服务启动失败**

```cpp
// 解决方案：检查服务可用性并诊断原因
QString diagnoseServiceFailure(idevice_t device, const QString& serviceName) {
    lockdownd_client_t lockdown = nullptr;
    
    // 1. 检查基本连接
    if (lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") != LOCKDOWN_E_SUCCESS) {
        return "无法建立lockdownd连接，可能设备未信任此电脑";
    }
    
    // 2. 检查设备iOS版本
    QString iosVersion = getLockdowndStringValue(lockdown, nullptr, "ProductVersion");
    if (!iosVersion.isEmpty()) {
        qDebug() << "设备iOS版本:" << iosVersion;
        
        // 特定服务的iOS版本要求
        if (serviceName == "com.apple.mobile.screenshotr" && 
            (iosVersion.startsWith("3.") || iosVersion.startsWith("4.0"))) {
            return "截图服务需要iOS 4.1或更高版本";
        }
    }
    
    // 3. 尝试启动服务并分析错误
    lockdownd_service_descriptor_t service = nullptr;
    lockdownd_error_t error = lockdownd_start_service(lockdown, 
                                                     serviceName.toUtf8().constData(), 
                                                     &service);
    
    if (service) {
        lockdownd_service_descriptor_free(service);
        return "服务已正常启动，可能是服务客户端初始化问题";
    }
    
    // 4. 根据错误代码分析
    switch (error) {
        case LOCKDOWN_E_INVALID_SERVICE:
            return QString("无效的服务名称: %1").arg(serviceName);
        case LOCKDOWN_E_START_SERVICE_FAILED:
            return QString("服务启动失败，可能设备不支持此服务或处于不允许的状态");
        case LOCKDOWN_E_MUX_ERROR:
            return "多路复用连接错误，可能usbmuxd服务异常";
        default:
            return QString("未知错误代码: %1").arg(error);
    }
    
    lockdownd_client_free(lockdown);
    return "诊断完成，但未发现明确问题";
}
```

#### 常见性能问题

**问题1: 文件传输速度慢**

```cpp
// 解决方案：优化文件传输缓冲区和并发策略
class OptimizedFileTransfer {
private:
    static const int OPTIMAL_BUFFER_SIZE = 65536;  // 64KB缓冲区
    static const int MAX_CONCURRENT_OPERATIONS = 4;  // 最大并发操作数
    
public:
    // 优化的文件上传实现
    static bool optimizedFileUpload(afc_client_t afc, 
                                  const QString& localPath, 
                                  const QString& remotePath) {
        QFile localFile(localPath);
        if (!localFile.open(QIODevice::ReadOnly)) {
            return false;
        }
        
        uint64_t handle = 0;
        QByteArray remotePathBytes = remotePath.toUtf8();
        
        if (afc_file_open(afc, remotePathBytes.constData(), 
                          AFC_FOPEN_WRONLY, &handle) != AFC_E_SUCCESS) {
            return false;
        }
        
        // 使用较大的缓冲区
        char buffer[OPTIMAL_BUFFER_SIZE];
        qint64 totalSize = localFile.size();
        qint64 transferred = 0;
        
        while (!localFile.atEnd()) {
            qint64 bytesRead = localFile.read(buffer, OPTIMAL_BUFFER_SIZE);
            if (bytesRead <= 0) break;
            
            uint32_t bytesWritten = 0;
            if (afc_file_write(afc, handle, buffer, bytesRead, &bytesWritten) != AFC_E_SUCCESS ||
                bytesWritten != static_cast<uint32_t>(bytesRead)) {
                afc_file_close(afc, handle);
                return false;
            }
            
            transferred += bytesWritten;
            
            // 发送进度更新（避免频繁更新UI）
            if (transferred % (1024 * 1024) == 0) {  // 每MB更新一次
                emit uploadProgress(transferred, totalSize);
            }
        }
        
        afc_file_close(afc, handle);
        return true;
    }
};
```

**问题2: 设备扫描慢或扫描不到设备**

```cpp
// 解决方案：使用事件驱动模式替代轮询
class DeviceScanner {
private:
    idevice_subscription_context_t eventContext;
    bool scanningEnabled;
    
public:
    // 启用事件驱动的设备监听
    bool startEventDrivenScanning() {
        if (idevice_event_subscribe(deviceEventCallback, this) != IDEVICE_E_SUCCESS) {
            return false;
        }
        
        eventContext = reinterpret_cast<idevice_subscription_context_t>(1);
        scanningEnabled = true;
        
        // 初始扫描一次
        performInitialScan();
        
        return true;
    }
    
    // 设备事件回调
    static void deviceEventCallback(const idevice_event_t* event, void* user_data) {
        DeviceScanner* scanner = static_cast<DeviceScanner*>(user_data);
        scanner->handleDeviceEvent(event);
    }
    
    // 处理设备事件
    void handleDeviceEvent(const idevice_event_t* event) {
        if (!scanningEnabled) return;
        
        QString udid = QString::fromUtf8(event->udid);
        
        switch (event->event) {
            case IDEVICE_DEVICE_ADD:
                emit deviceConnected(udid);
                break;
                
            case IDEVICE_DEVICE_REMOVE:
                emit deviceDisconnected(udid);
                break;
                
            case IDEVICE_DEVICE_PAIRED:
                emit devicePaired(udid);
                break;
        }
    }
    
    // 停止事件驱动扫描
    void stopEventDrivenScanning() {
        if (eventContext) {
            idevice_event_unsubscribe();
            eventContext = nullptr;
            scanningEnabled = false;
        }
    }
};
```

#### 平台特定问题

**Windows平台问题**

```cpp
// Windows特定解决方案
class WindowsSpecificSolutions {
public:
    // 检查iTunes和Apple Mobile Device Support
    static bool checkAppleComponents() {
        // 检查注册表项
        QSettings appleReg("HKEY_LOCAL_MACHINE\\SOFTWARE\\Apple Inc.", 
                          QSettings::NativeFormat);
        
        QStringList requiredComponents = {"Apple Mobile Device Support", "iTunes"};
        for (const QString& component : requiredComponents) {
            appleReg.beginGroup(component);
            if (!appleReg.contains("InstallDir")) {
                qWarning() << "Apple组件未安装或损坏:" << component;
                return false;
            }
            appleReg.endGroup();
        }
        
        return true;
    }
    
    // 修复驱动问题
    static bool repairDrivers() {
        // 尝试重启Apple Mobile Device服务
        QProcess process;
        process.start("net", QStringList() << "stop" << "Apple Mobile Device Service");
        process.waitForFinished(5000);
        
        process.start("net", QStringList() << "start" << "Apple Mobile Device Service");
        process.waitForFinished(5000);
        
        return process.exitCode() == 0;
    }
    
    // 检查usbmuxd服务状态
    static bool checkUsbmuxdService() {
        QProcess process;
        process.start("sc", QStringList() << "query" << "usbmuxd");
        process.waitForFinished();
        
        QString output = process.readAllStandardOutput();
        if (output.contains("RUNNING")) {
            return true;
        }
        
        // 尝试启动服务
        process.start("net", QStringList() << "start" << "usbmuxd");
        process.waitForFinished();
        
        return process.exitCode() == 0;
    }
};
```

**macOS平台问题**

```cpp
// macOS特定解决方案
class MacOSSpecificSolutions {
public:
    // 检查homebrew安装的libimobiledevice
    static bool checkHomebrewInstallation() {
        QProcess process;
        process.start("brew", QStringList() << "list" << "libimobiledevice");
        process.waitForFinished();
        
        return process.exitCode() == 0;
    }
    
    // 检查Xcode命令行工具
    static bool checkXcodeTools() {
        QProcess process;
        process.start("xcode-select", QStringList() << "-p");
        process.waitForFinished();
        
        return process.exitCode() == 0;
    }
    
    // 修复权限问题
    static bool fixPermissions() {
        // 修复usbmuxd权限
        QProcess::execute("sudo", QStringList() << "chown" << "root:wheel" << "/var/db/lockdown");
        QProcess::execute("sudo", QStringList() << "chmod" << "755" << "/var/db/lockdown");
        
        return true;
    }
};
```

#### 调试和日志收集

```cpp
// 调试辅助工具
class LibimobiledeviceDebugger {
public:
    // 收集环境信息
    static QString collectEnvironmentInfo() {
        QString info;
        
        // 1. 系统信息
        info += "=== 系统信息 ===
";
        info += QString("操作系统: %1
").arg(QSysInfo::prettyProductName());
        info += QString("内核版本: %1
").arg(QSysInfo::kernelVersion());
        info += QString("架构: %1
").arg(QSysInfo::currentCpuArchitecture());
        
        // 2. libimobiledevice版本
        info += "
=== libimobiledevice信息 ===
";
        info += "版本: " + getLibimobiledeviceVersion() + "
";
        
        // 3. 设备列表
        info += "
=== 已连接设备 ===
";
        info += getConnectedDevicesInfo();
        
        // 4. 服务状态
        info += "
=== 服务状态 ===
";
        info += getServiceStatus();
        
        return info;
    }
    
    // 启用详细日志
    static void enableVerboseLogging(bool enable = true) {
        #ifdef HAVE_LIBIMOBILEDEVICE
        // 设置libimobiledevice日志级别
        idevice_set_debug_level(enable ? 2 : 1);
        
        // 在macOS上启用系统日志
        #ifdef Q_OS_MAC
        if (enable) {
            QProcess::execute("log", QStringList() << "stream" << "--predicate" << "process == 'usbmuxd'");
        }
        #endif
        #endif
    }
    
    // 测试基本功能
    static bool testBasicFunctionality() {
        #ifdef HAVE_LIBIMOBILEDEVICE
        // 测试获取设备列表
        char **device_list = nullptr;
        int count = 0;
        
        if (idevice_get_device_list(&device_list, &count) != IDEVICE_E_SUCCESS) {
            qDebug() << "测试失败: 无法获取设备列表";
            return false;
        }
        
        qDebug() << QString("测试成功: 找到 %1 个设备").arg(count);
        
        idevice_device_list_free(device_list);
        return true;
        #else
        qDebug() << "测试失败: libimobiledevice未编译进项目";
        return false;
        #endif
    }
    
    // 生成诊断报告
    static void generateDiagnosticReport(const QString& filePath) {
        QString report = collectEnvironmentInfo();
        
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << report;
            file.close();
            
            qDebug() << "诊断报告已生成:" << filePath;
        }
    }
};
```

## 线程安全注意事项

### API线程安全性
- libimobiledevice的大多数API **不是线程安全的**
- 每个设备连接应在单独的线程中处理
- 同一设备的多个操作需要串行执行

### 最佳实践

```cpp
// 设备操作包装类
class DeviceOperation : public QObject {
    Q_OBJECT
private:
    QMutex deviceMutex_;
    idevice_t device_;
    
public slots:
    void performOperation(const QString& operation) {
        QMutexLocker locker(&deviceMutex_);
        
        // 在锁保护下执行设备操作
        if (operation == "screenshot") {
            takeScreenshot();
        } else if (operation == "getInfo") {
            getDeviceInfo();
        }
    }
    
private:
    void takeScreenshot() {
        // 截图操作实现
    }
    
    void getDeviceInfo() {
        // 获取设备信息实现
    }
};
```

## 性能优化建议

### 1. 连接复用
```cpp
// 保持设备连接，避免频繁重连
class DeviceManager {
private:
    static QMap<QString, idevice_t> deviceCache_;
    
public:
    static idevice_t getDevice(const QString& udid) {
        if (deviceCache_.contains(udid)) {
            return deviceCache_[udid];
        }
        
        idevice_t device = NULL;
        if (idevice_new(&device, udid.toStdString().c_str()) == IDEVICE_E_SUCCESS) {
            deviceCache_[udid] = device;
        }
        return device;
    }
};
```

### 2. 异步操作
```cpp
// 使用QFuture进行异步操作
QFuture<QImage> takeScreenshotAsync(const QString& udid) {
    return QtConcurrent::run([udid]() -> QImage {
        idevice_t device = DeviceManager::getDevice(udid);
        if (!device) return QImage();
        
        screenshotr_client_t screenshotr = NULL;
        if (screenshotr_client_start_service(device, &screenshotr, "phone-linkc") == SCREENSHOTR_E_SUCCESS) {
            char *imgdata = NULL;
            uint64_t imgsize = 0;
            
            if (screenshotr_take_screenshot(screenshotr, &imgdata, &imgsize) == SCREENSHOTR_E_SUCCESS) {
                QImage image = QImage::fromData((uchar*)imgdata, imgsize);
                free(imgdata);
                screenshotr_client_free(screenshotr);
                return image;
            }
            screenshotr_client_free(screenshotr);
        }
        return QImage();
    });
}
```

### 3. 内存管理
```cpp
// RAII资源管理
template<typename T, void(*Deleter)(T)>
class AutoResource {
private:
    T resource_;
    
public:
    AutoResource(T resource) : resource_(resource) {}
    ~AutoResource() { if (resource_) Deleter(resource_); }
    
    T get() const { return resource_; }
    T release() { T temp = resource_; resource_ = NULL; return temp; }
};

// 使用别名简化
using AutoDevice = AutoResource<idevice_t, idevice_free>;
using AutoLockdown = AutoResource<lockdownd_client_t, lockdownd_client_free>;

// 使用示例
AutoDevice device(device_ptr);
AutoLockdown lockdown(lockdown_ptr);
// 作用域结束时自动释放资源
```

### 4. 批量操作
```cpp
// 批量应用操作优化器
class BatchAppOperations {
private:
    struct BatchOperation {
        enum Type { Install, Uninstall, Update };
        Type type;
        QString bundleId;
        QString filePath;  // 用于安装/更新
        QString version;   // 用于更新
    };
    
    QList<BatchOperation> operations_;
    QMap<QString, bool> results_;
    
public:
    // 添加批量操作
    void addInstall(const QString& filePath) {
        BatchOperation op;
        op.type = BatchOperation::Install;
        op.filePath = filePath;
        operations_ << op;
    }
    
    void addUninstall(const QString& bundleId) {
        BatchOperation op;
        op.type = BatchOperation::Uninstall;
        op.bundleId = bundleId;
        operations_ << op;
    }
    
    // 执行批量操作
    QMap<QString, bool> executeBatch(idevice_t device) {
        if (!device) {
            qWarning() << "无效的设备句柄";
            return results_;
        }
        
        instproxy_client_t instproxy = nullptr;
        if (instproxy_client_start_service(device, &instproxy, "phone-linkc") != INSTPROXY_E_SUCCESS) {
            qWarning() << "无法启动应用代理服务";
            return results_;
        }
        
        AutoInstProxy autoProxy(instproxy);
        
        // 为安装操作创建选项
        plist_t installOptions = createBatchInstallOptions();
        
        for (int i = 0; i < operations_.size(); i++) {
            const BatchOperation& op = operations_[i];
            
            switch (op.type) {
                case BatchOperation::Install:
                    results_[op.filePath] = performInstall(instproxy, op.filePath, installOptions);
                    break;
                    
                case BatchOperation::Uninstall:
                    results_[op.bundleId] = performUninstall(instproxy, op.bundleId);
                    break;
            }
            
            // 发送进度更新
            emit batchProgress(i + 1, operations_.size(), 
                               QString("已完成 %1/%2 个操作")
                               .arg(i + 1)
                               .arg(operations_.size()));
        }
        
        plist_free(installOptions);
        return results_;
    }
    
signals:
    void batchProgress(int current, int total, const QString& message);
    
private:
    // RAII类，自动释放instproxy_client
    class AutoInstProxy {
        instproxy_client_t client_;
    public:
        AutoInstProxy(instproxy_client_t client) : client_(client) {}
        ~AutoInstProxy() { if (client_) instproxy_client_free(client_); }
        instproxy_client_t get() { return client_; }
    };
    
    // 创建批量安装选项，减少重复设置
    plist_t createBatchInstallOptions() {
        plist_t options = plist_new_dict();
        plist_dict_set_item(options, "ApplicationType", plist_new_string("User"));
        plist_dict_set_item(options, "SkipUninstall", plist_new_bool(true));
        plist_dict_set_item(options, "iTunesMetadata", plist_new_bool(false));
        return options;
    }
    
    // 执行安装操作
    bool performInstall(instproxy_client_t client, const QString& filePath, plist_t options) {
        // 实现细节省略
        return true;
    }
    
    // 执行卸载操作
    bool performUninstall(instproxy_client_t client, const QString& bundleId) {
        // 实现细节省略
        return true;
    }
};
```

## 高级API模块

### 7. 移动备份 API (mobilebackup2)

#### 7.1 备份服务

##### mobilebackup2_client_start_service()
```c
mobilebackup2_error_t mobilebackup2_client_start_service(idevice_t device,
                                                        mobilebackup2_client_t *client,
                                                        const char *label);
```

**功能描述**: 启动移动备份服务（iOS 4.0+）

##### mobilebackup2_send_request()
```c
mobilebackup2_error_t mobilebackup2_send_request(mobilebackup2_client_t client,
                                               const char *request,
                                               const char *target_identifier,
                                               const char *source_identifier,
                                               plist_t options);
```

**功能描述**: 发送备份请求

**常用备份请求类型**:
- `"Backup"`: 创建备份
- `"Restore"`: 恢复备份
- `"Info"`: 获取备份信息
- `"List"`: 列出可用备份

#### 7.2 备份操作示例

```cpp
// 备份管理器实现
class BackupManager {
private:
    DeviceManager* deviceManager_;
    QString backupDirectory_;
    
public:
    BackupManager(DeviceManager* deviceManager, QObject* parent = nullptr)
        : QObject(parent), deviceManager_(deviceManager) {
        
        // 设置默认备份目录
        backupDirectory_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups";
        QDir().mkpath(backupDirectory_);
    }
    
    // 创建设备备份
    bool createBackup(const QString& udid, const QString& backupName = QString()) {
        idevice_t device = deviceManager_->getDeviceConnection(udid);
        if (!device) {
            return false;
        }
        
        mobilebackup2_client_t mb2 = nullptr;
        if (mobilebackup2_client_start_service(device, &mb2, "phone-linkc") != MOBILEBACKUP2_E_SUCCESS) {
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 生成备份名称（如果未提供）
        QString finalBackupName = backupName;
        if (finalBackupName.isEmpty()) {
            finalBackupName = QString("%1_%2")
                .arg(udid.left(8)) // 使用UDID前8位
                .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        }
        
        // 创建备份目录
        QString backupPath = backupDirectory_ + "/" + finalBackupName;
        QDir().mkpath(backupPath);
        
        // 设置备份选项
        plist_t options = plist_new_dict();
        plist_dict_set_item(options, "ForceFullBackup", plist_new_bool(true));
        plist_dict_set_item(options, "BackupSystemFiles", plist_new_bool(true));
        
        // 发送备份请求
        if (mobilebackup2_send_request(mb2, "Backup", 
                                     udid.toUtf8().constData(),
                                     nullptr, 
                                     options) != MOBILEBACKUP2_E_SUCCESS) {
            plist_free(options);
            mobilebackup2_client_free(mb2);
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 处理备份响应
        plist_t response = nullptr;
        if (mobilebackup2_receive_message(mb2, &response) != MOBILEBACKUP2_E_SUCCESS) {
            plist_free(options);
            mobilebackup2_client_free(mb2);
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 验证响应
        bool success = false;
        if (response) {
            plist_t status = plist_dict_get_item(response, "Status");
            if (status) {
                char *statusStr = nullptr;
                plist_get_string_val(status, &statusStr);
                success = (QString(statusStr) == "Success");
                free(statusStr);
            }
            plist_free(response);
        }
        
        plist_free(options);
        mobilebackup2_client_free(mb2);
        deviceManager_->releaseDeviceConnection(udid);
        
        if (success) {
            emit backupCompleted(backupPath);
            qDebug() << "备份成功完成:" << backupPath;
        } else {
            emit backupFailed("备份过程中发生错误");
        }
        
        return success;
    }
    
    // 列出所有备份
    QStringList listBackups(const QString& udid = QString()) {
        QStringList backups;
        QDir backupDir(backupDirectory_);
        
        QStringList entries = backupDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        
        for (const QString& entry : entries) {
            // 如果指定了UDID，只匹配该设备的备份
            if (!udid.isEmpty() && !entry.startsWith(udid.left(8))) {
                continue;
            }
            
            backups.append(entry);
        }
        
        return backups;
    }
    
    // 恢复备份
    bool restoreBackup(const QString& udid, const QString& backupName, bool eraseDevice = false) {
        idevice_t device = deviceManager_->getDeviceConnection(udid);
        if (!device) {
            return false;
        }
        
        mobilebackup2_client_t mb2 = nullptr;
        if (mobilebackup2_client_start_service(device, &mb2, "phone-linkc") != MOBILEBACKUP2_E_SUCCESS) {
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 检查备份是否存在
        QString backupPath = backupDirectory_ + "/" + backupName;
        QDir backupDir(backupPath);
        if (!backupDir.exists()) {
            emit restoreFailed("备份不存在: " + backupName);
            mobilebackup2_client_free(mb2);
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 设置恢复选项
        plist_t options = plist_new_dict();
        plist_dict_set_item(options, "RestoreSystemFiles", plist_new_bool(true));
        plist_dict_set_item(options, "CopyUserSettings", plist_new_bool(true));
        plist_dict_set_item(options, "EraseBeforeRestore", plist_new_bool(eraseDevice));
        
        // 发送恢复请求
        if (mobilebackup2_send_request(mb2, "Restore",
                                     udid.toUtf8().constData(),
                                     backupPath.toUtf8().constData(),
                                     options) != MOBILEBACKUP2_E_SUCCESS) {
            plist_free(options);
            mobilebackup2_client_free(mb2);
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 处理恢复响应
        plist_t response = nullptr;
        if (mobilebackup2_receive_message(mb2, &response) != MOBILEBACKUP2_E_SUCCESS) {
            plist_free(options);
            mobilebackup2_client_free(mb2);
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 验证响应
        bool success = false;
        if (response) {
            plist_t status = plist_dict_get_item(response, "Status");
            if (status) {
                char *statusStr = nullptr;
                plist_get_string_val(status, &statusStr);
                success = (QString(statusStr) == "Success");
                free(statusStr);
            }
            plist_free(response);
        }
        
        plist_free(options);
        mobilebackup2_client_free(mb2);
        deviceManager_->releaseDeviceConnection(udid);
        
        if (success) {
            emit restoreCompleted(backupName);
            qDebug() << "恢复成功完成:" << backupName;
        } else {
            emit restoreFailed("恢复过程中发生错误");
        }
        
        return success;
    }
    
    // 删除备份
    bool deleteBackup(const QString& backupName) {
        QString backupPath = backupDirectory_ + "/" + backupName;
        QDir backupDir(backupPath);
        
        if (!backupDir.exists()) {
            return false;
        }
        
        return backupDir.removeRecursively();
    }
    
signals:
    void backupCompleted(const QString& backupPath);
    void backupFailed(const QString& errorMessage);
    void restoreCompleted(const QString& backupName);
    void restoreFailed(const QString& errorMessage);
    void restoreProgress(int percentage);
};

### 8. 春天板服务 API (springboard)

#### 8.1 SpringBoard服务

##### sbservices_client_start_service()
```c
sbservices_error_t sbservices_client_start_service(idevice_t device,
                                                  sbservices_client_t *client,
                                                  const char *label);
```

**功能描述**: 启动SpringBoard服务

##### sbservices_get_icon_state()
```c
sbservices_error_t sbservices_get_icon_state(sbservices_client_t client,
                                            plist_t *state,
                                            const char *format_version);
```

**功能描述**: 获取主屏幕图标布局

##### sbservices_get_icon_pngdata()
```c
sbservices_error_t sbservices_get_icon_pngdata(sbservices_client_t client,
                                              const char *bundleid,
                                              char **pngdata,
                                              uint64_t *pngsize);
```

**功能描述**: 获取应用图标PNG数据

**使用示例**:
```cpp
sbservices_client_t sbservices = nullptr;
sbservices_error_t error = sbservices_client_start_service(device, &sbservices, "phone-linkc");
if (error == SBSERVICES_E_SUCCESS) {
    char *pngdata = nullptr;
    uint64_t pngsize = 0;
    
    error = sbservices_get_icon_pngdata(sbservices, "com.apple.MobileSafari", &pngdata, &pngsize);
    if (error == SBSERVICES_E_SUCCESS && pngdata) {
        // 转换为QImage
        QImage icon = QImage::fromData(reinterpret_cast<const uchar*>(pngdata), 
                                     static_cast<int>(pngsize), "PNG");
        free(pngdata);
    }
    
    sbservices_client_free(sbservices);
}
```

### 9. 诊断中继 API (diagnostics_relay)

#### 9.1 诊断服务

##### diagnostics_relay_client_start_service()
```c
diagnostics_relay_error_t diagnostics_relay_client_start_service(idevice_t device,
                                                                diagnostics_relay_client_t *client,
                                                                const char *label);
```

##### diagnostics_relay_restart()
```c
diagnostics_relay_error_t diagnostics_relay_restart(diagnostics_relay_client_t client,
                                                   int flags);
```

**功能描述**: 重启设备

##### diagnostics_relay_shutdown()
```c
diagnostics_relay_error_t diagnostics_relay_shutdown(diagnostics_relay_client_t client,
                                                    int flags);
```

**功能描述**: 关闭设备

##### diagnostics_relay_sleep()
```c
diagnostics_relay_error_t diagnostics_relay_sleep(diagnostics_relay_client_t client);
```

**功能描述**: 让设备进入睡眠模式

### 10. 通知代理 API (notification_proxy)

#### 10.1 通知服务

##### np_client_start_service()
```c
np_error_t np_client_start_service(idevice_t device,
                                  np_client_t *client,
                                  const char *label);
```

##### np_observe_notification()
```c
np_error_t np_observe_notification(np_client_t client, const char *notification);
```

**功能描述**: 订阅设备通知

**常用通知类型**:
- `NP_SYNC_WILL_START`: 同步即将开始
- `NP_SYNC_DID_START`: 同步已开始
- `NP_SYNC_DID_FINISH`: 同步已完成
- `NP_BACKUP_DOMAIN_CHANGED`: 备份域改变
- `NP_APP_INSTALLED`: 应用已安装
- `NP_APP_UNINSTALLED`: 应用已卸载

##### np_get_notification()
```c
np_error_t np_get_notification(np_client_t client, char **notification);
```

**功能描述**: 获取通知消息

**使用示例**:
```cpp
np_client_t np = nullptr;
np_error_t error = np_client_start_service(device, &np, "phone-linkc");
if (error == NP_E_SUCCESS) {
    // 订阅应用安装通知
    np_observe_notification(np, NP_APP_INSTALLED);
    np_observe_notification(np, NP_APP_UNINSTALLED);
    
    // 监听通知
    while (running) {
        char *notification = nullptr;
        error = np_get_notification(np, &notification);
        if (error == NP_E_SUCCESS && notification) {
            qDebug() << "收到通知:" << notification;
            
            if (strcmp(notification, NP_APP_INSTALLED) == 0) {
                emit appInstalled();
            } else if (strcmp(notification, NP_APP_UNINSTALLED) == 0) {
                emit appUninstalled();
            }
            
            free(notification);
        }
    }
    
    np_client_free(np);
}
```

##### np_set_notify_callback()
```c
np_error_t np_set_notify_callback(np_client_t client,
                                np_notify_cb_t notify_cb,
                                void *user_data);
```

**功能描述**: 设置通知回调函数（替代轮询方式）

**参数说明**:
- `client`: 通知代理客户端句柄
- `notify_cb`: 通知回调函数
- `user_data`: 用户数据

#### 8.2 通知事件处理

以下是完整的通知事件处理系统实现，可用于实时监控设备状态变化。

**高级通知处理器**:

```cpp
// 设备通知管理器
class DeviceNotificationManager : public QObject {
    Q_OBJECT
    
private:
    QMap<QString, np_client_t> notificationClients_;  // 按设备UDID索引的客户端
    QSet<QString> subscribedNotifications_;          // 已订阅的通知列表
    QMutex clientMutex_;                             // 线程安全锁
    
public:
    DeviceNotificationManager(QObject* parent = nullptr) : QObject(parent) {
    }
    
    ~DeviceNotificationManager() {
        // 清理所有通知客户端
        QMutexLocker locker(&clientMutex_);
        for (auto it = notificationClients_.begin(); it != notificationClients_.end(); ++it) {
            np_client_free(it.value());
        }
        notificationClients_.clear();
    }
    
    // 为设备启用通知监听
    bool enableNotifications(const QString& udid, idevice_t device) {
        QMutexLocker locker(&clientMutex_);
        
        // 检查是否已存在客户端
        if (notificationClients_.contains(udid)) {
            return true;
        }
        
        // 创建通知客户端
        np_client_t np = nullptr;
        np_error_t error = np_client_start_service(device, &np, "phone-linkc");
        if (error != NP_E_SUCCESS) {
            qWarning() << "无法启动通知代理服务:" << error;
            return false;
        }
        
        // 设置通知回调
        if (np_set_notify_callback(np, notificationCallback, this) != NP_E_SUCCESS) {
            np_client_free(np);
            return false;
        }
        
        // 订阅关键通知
        const char* notifications[] = {
            NP_SYNC_WILL_START,
            NP_SYNC_DID_START,
            NP_SYNC_DID_FINISH,
            NP_BACKUP_DOMAIN_CHANGED,
            NP_APP_INSTALLED,
            NP_APP_UNINSTALLED,
            NP_PHONE_NUMBER_CHANGED,
            NP_DEVICE_NAME_CHANGED,
            NP_TIMEZONE_CHANGED,
            NP_TRUSTED_HOST_CHANGED,
            NULL
        };
        
        for (int i = 0; notifications[i]; i++) {
            if (np_observe_notification(np, notifications[i]) == NP_E_SUCCESS) {
                subscribedNotifications_.insert(notifications[i]);
            }
        }
        
        notificationClients_[udid] = np;
        
        qDebug() << "已为设备" << udid << "启用通知监听";
        return true;
    }
    
    // 禁用设备通知
    bool disableNotifications(const QString& udid) {
        QMutexLocker locker(&clientMutex_);
        
        if (!notificationClients_.contains(udid)) {
            return false;
        }
        
        np_client_free(notificationClients_[udid]);
        notificationClients_.remove(udid);
        
        qDebug() << "已为设备" << udid << "禁用通知监听";
        return true;
    }
    
    // 获取已订阅的通知列表
    QStringList getSubscribedNotifications() const {
        return QStringList(subscribedNotifications_.begin(), subscribedNotifications_.end());
    }

signals:
    void syncStarted(const QString& udid);
    void syncFinished(const QString& udid);
    void appInstalled(const QString& udid, const QString& bundleId);
    void appUninstalled(const QString& udid, const QString& bundleId);
    void deviceNameChanged(const QString& udid, const QString& oldName, const QString& newName);
    void phoneNumberChanged(const QString& udid, const QString& phoneNumber);
    void backupDomainChanged(const QString& udid);
    void timezoneChanged(const QString& udid, const QString& timezone);
    void trustedHostChanged(const QString& udid, const QString& host);

private:
    // 静态通知回调函数
    static void notificationCallback(const char* notification, void* user_data) {
        DeviceNotificationManager* manager = static_cast<DeviceNotificationManager*>(user_data);
        if (manager) {
            manager->handleNotification(QString::fromUtf8(notification));
        }
    }
    
    // 处理通知事件
    void handleNotification(const QString& notification) {
        // 注意：需要从通知中提取设备UDID，这可能需要在实现中维护设备映射
        // 这里简化处理，使用默认逻辑
        
        qDebug() << "收到设备通知:" << notification;
        
        if (notification == NP_SYNC_WILL_START) {
            emit syncStarted("unknown_udid");
        } else if (notification == NP_SYNC_DID_FINISH) {
            emit syncFinished("unknown_udid");
        } else if (notification == NP_APP_INSTALLED) {
            // 实际应用中需要获取Bundle ID
            emit appInstalled("unknown_udid", "unknown_bundle_id");
        } else if (notification == NP_APP_UNINSTALLED) {
            emit appUninstalled("unknown_udid", "unknown_bundle_id");
        } else if (notification == NP_BACKUP_DOMAIN_CHANGED) {
            emit backupDomainChanged("unknown_udid");
        } else if (notification == NP_PHONE_NUMBER_CHANGED) {
            emit phoneNumberChanged("unknown_udid", "unknown_phone");
        } else if (notification == NP_DEVICE_NAME_CHANGED) {
            emit deviceNameChanged("unknown_udid", "old_name", "new_name");
        } else if (notification == NP_TIMEZONE_CHANGED) {
            emit timezoneChanged("unknown_udid", "new_timezone");
        } else if (notification == NP_TRUSTED_HOST_CHANGED) {
            emit trustedHostChanged("unknown_udid", "host_name");
        }
    }
};

// 增强版通知管理器，支持多设备和事件历史
class AdvancedDeviceNotificationManager : public DeviceNotificationManager {
    Q_OBJECT
    
private:
    QMap<QString, QString> udidToClientMap_;  // 设备UDID到客户端的映射
    QList<NotificationEvent> eventHistory_;    // 事件历史记录
    
    struct NotificationEvent {
        QDateTime timestamp;
        QString udid;
        QString notification;
        QVariantMap details;
        
        QString toString() const {
            return QString("[%1] %2: %3")
                .arg(timestamp.toString("yyyy-MM-dd hh:mm:ss"))
                .arg(udid)
                .arg(notification);
        }
    };
    
public:
    // 添加事件到历史记录
    void addEventToHistory(const QString& udid, const QString& notification, 
                          const QVariantMap& details = QVariantMap()) {
        NotificationEvent event;
        event.timestamp = QDateTime::currentDateTime();
        event.udid = udid;
        event.notification = notification;
        event.details = details;
        
        eventHistory_.append(event);
        
        // 限制历史记录数量
        const int MAX_HISTORY = 1000;
        if (eventHistory_.size() > MAX_HISTORY) {
            eventHistory_.removeFirst();
        }
        
        // 发送事件通知
        emit notificationEventAdded(event);
    }
    
    // 获取事件历史
    QList<NotificationEvent> getEventHistory(const QString& udid = QString()) const {
        if (udid.isEmpty()) {
            return eventHistory_;
        }
        
        QList<NotificationEvent> filtered;
        for (const NotificationEvent& event : eventHistory_) {
            if (event.udid == udid) {
                filtered.append(event);
            }
        }
        
        return filtered;
    }
    
    // 清除事件历史
    void clearEventHistory() {
        eventHistory_.clear();
        emit eventHistoryCleared();
    }
    
    // 导出事件历史到文件
    bool exportEventHistory(const QString& filePath) const {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        
        QTextStream out(&file);
        out << "Timestamp,Device,Notification,Details
";
        
        for (const NotificationEvent& event : eventHistory_) {
            out << event.timestamp.toString(Qt::ISODate) << ","
                << event.udid << ","
                << event.notification << ",";
            
            if (!event.details.isEmpty()) {
                QStringList details;
                for (auto it = event.details.begin(); it != event.details.end(); ++it) {
                    details.append(QString("%1=%2").arg(it.key(), it.value().toString()));
                }
                out << details.join(";");
            }
            
            out << "
";
        }
        
        file.close();
        return true;
    }

signals:
    void notificationEventAdded(const NotificationEvent& event);
    void eventHistoryCleared();
    
protected:
    // 重写通知处理，添加历史记录
    void handleNotification(const QString& notification) override {
        // 尝试识别事件对应的设备UDID
        QString udid = identifyDeviceForNotification(notification);
        
        // 创建事件详情
        QVariantMap details;
        
        if (notification == NP_APP_INSTALLED || notification == NP_APP_UNINSTALLED) {
            details["type"] = "app_change";
        } else if (notification.contains("sync")) {
            details["type"] = "sync";
        } else if (notification.contains("backup")) {
            details["type"] = "backup";
        }
        
        // 添加到历史记录
        addEventToHistory(udid, notification, details);
        
        // 调用基类处理
        DeviceNotificationManager::handleNotification(notification);
    }
    
private:
    // 尝试识别通知对应的设备UDID
    QString identifyDeviceForNotification(const QString& notification) {
        // 实际应用中可能需要更复杂的逻辑来识别设备
        // 这里简化处理，使用第一个已知设备或默认值
        
        if (!udidToClientMap_.isEmpty()) {
            return udidToClientMap_.begin().key();
        }
        
        return "unknown_device";
    }
};

// 通知事件可视化工具
class NotificationEventVisualizer : public QObject {
    Q_OBJECT
    
private:
    QMap<QString, int> notificationCounts_;
    QMap<QString, QDateTime> lastNotificationTime_;
    QDateTime startTime_;
    
public:
    NotificationEventVisualizer(QObject* parent = nullptr) 
        : QObject(parent), startTime_(QDateTime::currentDateTime()) {
    }
    
    // 添加通知事件
    void addNotificationEvent(const QString& notification, const QString& udid = QString()) {
        // 更新计数
        notificationCounts_[notification]++;
        
        // 更新最后通知时间
        lastNotificationTime_[notification] = QDateTime::currentDateTime();
        
        // 发送更新信号
        emit dataUpdated();
    }
    
    // 获取通知分布数据（用于饼图）
    QVariantMap getNotificationDistribution() const {
        QVariantMap result;
        
        for (auto it = notificationCounts_.begin(); it != notificationCounts_.end(); ++it) {
            result[it.key()] = it.value();
        }
        
        return result;
    }
    
    // 获取通知时间线数据（用于折线图）
    QVariantMap getNotificationTimeline() const {
        QVariantMap result;
        
        // 按小时聚合通知数据
        QMap<int, int> hourlyCounts;
        
        for (auto it = lastNotificationTime_.begin(); it != lastNotificationTime_.end(); ++it) {
            int hour = it.value().time().hour();
            hourlyCounts[hour] += notificationCounts_[it.key()];
        }
        
        // 转换为图表友好格式
        QList<QVariant> hours;
        QList<QVariant> counts;
        
        for (int i = 0; i < 24; i++) {
            hours.append(i);
            counts.append(hourlyCounts.value(i, 0));
        }
        
        result["hours"] = hours;
        result["counts"] = counts;
        
        return result;
    }
    
    // 获取最频繁的通知
    QString getMostFrequentNotification() const {
        if (notificationCounts_.isEmpty()) {
            return QString();
        }
        
        QString mostFrequent;
        int maxCount = 0;
        
        for (auto it = notificationCounts_.begin(); it != notificationCounts_.end(); ++it) {
            if (it.value() > maxCount) {
                maxCount = it.value();
                mostFrequent = it.key();
            }
        }
        
        return mostFrequent;
    }
    
    // 清除所有数据
    void clear() {
        notificationCounts_.clear();
        lastNotificationTime_.clear();
        startTime_ = QDateTime::currentDateTime();
        emit dataUpdated();
    }

signals:
    void dataUpdated();
};
```

## phone-linkc项目集成

### 项目结构中的使用模式

#### 条件编译支持
```cpp
// devicemanager.h 中的条件编译模式
#ifdef HAVE_LIBIMOBILEDEVICE
// Forward declarations to avoid including headers in header file
typedef struct idevice_private idevice_private;
typedef idevice_private* idevice_t;
typedef struct lockdownd_client_private lockdownd_client_private;
typedef lockdownd_client_private* lockdownd_client_t;
typedef struct idevice_subscription_context* idevice_subscription_context_t;
#endif

class DeviceManager : public QObject {
private:
#ifdef HAVE_LIBIMOBILEDEVICE
    idevice_t m_device;
    lockdownd_client_t m_lockdown;
    idevice_subscription_context_t m_eventContext;
#endif
};
```

#### 完整的设备信息获取
基于项目中的DeviceInfo结构：

```cpp
// phone-linkc项目中的设备信息结构
struct DeviceInfo {
    QString udid;
    QString name;
    QString model;
    QString productVersion;
    QString buildVersion;
    QString serialNumber;
    QString deviceClass;
    QString productType;
    qint64 totalCapacity;
    qint64 availableCapacity;
    QString wifiAddress;
    QString activationState;
    
    QVariantMap toMap() const;
    QString toString() const;
};

// 完整的设备信息获取实现
DeviceInfo getDeviceInfo(const QString &udid) {
    DeviceInfo info;
    info.udid = udid;
    
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    
    if (idevice_new(&device, udid.toUtf8().constData()) == IDEVICE_E_SUCCESS) {
        if (lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") == LOCKDOWN_E_SUCCESS) {
            
            // 获取基本信息
            info.name = getStringValue(lockdown, nullptr, "DeviceName");
            info.model = getStringValue(lockdown, nullptr, "ProductType");
            info.productVersion = getStringValue(lockdown, nullptr, "ProductVersion");
            info.buildVersion = getStringValue(lockdown, nullptr, "BuildVersion");
            info.serialNumber = getStringValue(lockdown, nullptr, "SerialNumber");
            info.deviceClass = getStringValue(lockdown, nullptr, "DeviceClass");
            info.activationState = getStringValue(lockdown, nullptr, "ActivationState");
            
            // 获取WiFi地址
            info.wifiAddress = getStringValue(lockdown, nullptr, "WiFiAddress");
            
            // 获取存储容量信息
            info.totalCapacity = getIntValue(lockdown, "com.apple.disk_usage", "TotalDataCapacity");
            info.availableCapacity = getIntValue(lockdown, "com.apple.disk_usage", "TotalDataAvailable");
            
            lockdownd_client_free(lockdown);
        }
        idevice_free(device);
    }
    
    return info;
}

// 辅助函数实现
QString getStringValue(lockdownd_client_t lockdown, const char* domain, const char* key) {
    plist_t value = nullptr;
    QString result;
    
    if (lockdownd_get_value(lockdown, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (plist_get_node_type(value) == PLIST_STRING) {
            char *str_value = nullptr;
            plist_get_string_val(value, &str_value);
            if (str_value) {
                result = QString::fromUtf8(str_value);
                free(str_value);
            }
        }
        plist_free(value);
    }
    
    return result;
}

qint64 getIntValue(lockdownd_client_t lockdown, const char* domain, const char* key) {
    plist_t value = nullptr;
    qint64 result = 0;
    
    if (lockdownd_get_value(lockdown, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (plist_get_node_type(value) == PLIST_UINT) {
            uint64_t int_value = 0;
            plist_get_uint_val(value, &int_value);
            result = static_cast<qint64>(int_value);
        }
        plist_free(value);
    }
    
    return result;
}
```

## 调试和诊断

### 启用调试输出
```cpp
#include "libimobiledevice/libimobiledevice.h"

// 设置调试级别
void enableDebugOutput() {
    idevice_set_debug_level(1);  // 0=无输出, 1=错误, 2=详细
}

// phone-linkc项目中的日志集成
void setupLibimobiledeviceLogging() {
#ifdef HAVE_LIBIMOBILEDEVICE
    // 在开发模式下启用详细日志
    #ifdef QT_DEBUG
        idevice_set_debug_level(2);
        qDebug() << "libimobiledevice调试输出已启用";
    #else
        idevice_set_debug_level(1);
    #endif
#endif
}
```

### 常见问题诊断

#### 1. 设备连接问题诊断
```cpp
// 基于phone-linkc项目的诊断实现
class DeviceDiagnostics {
public:
    struct DiagnosticResult {
        bool success;
        QString message;
        QStringList suggestions;
    };
    
    static DiagnosticResult diagnoseDeviceConnection(const QString& udid) {
        DiagnosticResult result;
        result.success = false;
        
        // 1. 检查libimobiledevice可用性
        #ifndef HAVE_LIBIMOBILEDEVICE
        result.message = "libimobiledevice未安装或不可用";
        result.suggestions << "请安装libimobiledevice库";
        result.suggestions << "确保iTunes或3uTools已安装（Windows）";
        return result;
        #endif
        
        // 2. 检查设备是否在列表中
        char **device_list = nullptr;
        int count = 0;
        
        if (idevice_get_device_list(&device_list, &count) != IDEVICE_E_SUCCESS) {
            result.message = "无法获取设备列表";
            result.suggestions << "检查USB连接";
            result.suggestions << "重启usbmuxd服务";
            return result;
        }
        
        bool deviceFound = false;
        for (int i = 0; i < count; i++) {
            if (QString::fromUtf8(device_list[i]) == udid) {
                deviceFound = true;
                break;
            }
        }
        idevice_device_list_free(device_list);
        
        if (!deviceFound) {
            result.message = QString("设备 %1 未在设备列表中找到").arg(udid);
            result.suggestions << "检查设备是否已连接";
            result.suggestions << "确保设备已解锁";
            result.suggestions << "点击设备上的'信任此电脑'";
            return result;
        }
        
        // 3. 尝试建立连接
        idevice_t device = nullptr;
        idevice_error_t error = idevice_new(&device, udid.toUtf8().constData());
        if (error != IDEVICE_E_SUCCESS) {
            result.message = QString("设备连接失败: %1").arg(getErrorMessage(error));
            result.suggestions << "重新拔插USB线";
            result.suggestions << "重启设备";
            result.suggestions << "更新iTunes或安装最新驱动";
            return result;
        }
        
        // 4. 测试lockdown服务
        lockdownd_client_t lockdown = nullptr;
        lockdownd_error_t lockdown_error = lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc");
        if (lockdown_error != LOCKDOWN_E_SUCCESS) {
            result.message = QString("Lockdown服务连接失败: %1").arg(lockdown_error);
            result.suggestions << "设备可能需要重新配对";
            result.suggestions << "尝试在设置中重置网络设置";
            idevice_free(device);
            return result;
        }
        
        // 5. 获取设备基本信息验证
        plist_t device_name = nullptr;
        if (lockdownd_get_value(lockdown, nullptr, "DeviceName", &device_name) == LOCKDOWN_E_SUCCESS && device_name) {
            char *name_str = nullptr;
            plist_get_string_val(device_name, &name_str);
            result.message = QString("设备连接正常: %1").arg(QString::fromUtf8(name_str));
            result.success = true;
            free(name_str);
            plist_free(device_name);
        }
        
        lockdownd_client_free(lockdown);
        idevice_free(device);
        
        return result;
    }
    
    static QString getErrorMessage(idevice_error_t error) {
        switch (error) {
            case IDEVICE_E_SUCCESS: return "成功";
            case IDEVICE_E_INVALID_ARG: return "参数无效";
            case IDEVICE_E_NO_DEVICE: return "设备未找到或无法访问";
            case IDEVICE_E_NOT_ENOUGH_DATA: return "数据不足";
            case IDEVICE_E_SSL_ERROR: return "SSL连接错误";
            case IDEVICE_E_TIMEOUT: return "连接超时";
            default: return QString("未知错误 (%1)").arg(error);
        }
    }
};
```

#### 2. 服务可用性检查
```cpp
bool checkServiceAvailability(idevice_t device, const QString& serviceName) {
    if (!device) return false;
    
    lockdownd_client_t lockdown = nullptr;
    lockdownd_error_t error = lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc");
    if (error != LOCKDOWN_E_SUCCESS) {
        return false;
    }
    
    lockdownd_service_descriptor_t service = nullptr;
    error = lockdownd_start_service(lockdown, serviceName.toUtf8().constData(), &service);
    
    bool available = (error == LOCKDOWN_E_SUCCESS && service != nullptr);
    
    if (service) {
        lockdownd_service_descriptor_free(service);
    }
    lockdownd_client_free(lockdown);
    
    return available;
}

// 检查主要服务的可用性
QMap<QString, bool> checkAllServicesAvailability(idevice_t device) {
    QMap<QString, bool> services;
    
    services["com.apple.mobile.screenshotr"] = checkServiceAvailability(device, "com.apple.mobile.screenshotr");
    services["com.apple.mobile.installation_proxy"] = checkServiceAvailability(device, "com.apple.mobile.installation_proxy");
    services["com.apple.afc"] = checkServiceAvailability(device, "com.apple.afc");
    services["com.apple.syslog_relay"] = checkServiceAvailability(device, "com.apple.syslog_relay");
    services["com.apple.springboardservices"] = checkServiceAvailability(device, "com.apple.springboardservices");
    services["com.apple.mobile.notification_proxy"] = checkServiceAvailability(device, "com.apple.mobile.notification_proxy");
    
    return services;
}
```

### 性能监控和分析

```cpp
// 性能监控类
class LibimobiledeviceProfiler {
private:
    static QMap<QString, QElapsedTimer> timers_;
    static QMap<QString, qint64> totalTimes_;
    static QMap<QString, int> callCounts_;
    
public:
    class ScopeTimer {
    private:
        QString operation_;
        QElapsedTimer timer_;
        
    public:
        ScopeTimer(const QString& operation) : operation_(operation) {
            timer_.start();
        }
        
        ~ScopeTimer() {
            qint64 elapsed = timer_.elapsed();
            totalTimes_[operation_] += elapsed;
            callCounts_[operation_]++;
            
            if (elapsed > 1000) { // 超过1秒的操作记录警告
                qWarning() << "慢操作:" << operation_ << "耗时" << elapsed << "ms";
            }
        }
    };
    
    static void printStats() {
        qDebug() << "=== libimobiledevice 性能统计 ===";
        for (auto it = totalTimes_.begin(); it != totalTimes_.end(); ++it) {
            qDebug() << QString("%1: 总耗时 %2ms, 调用次数 %3, 平均 %4ms")
                        .arg(it.key())
                        .arg(it.value())
                        .arg(callCounts_[it.key()])
                        .arg(it.value() / callCounts_[it.key()]);
        }
    }
};

// 使用宏简化性能监控
#define PROFILE_OPERATION(name) LibimobiledeviceProfiler::ScopeTimer timer(name)

// 使用示例
QImage takeScreenshotWithProfiling(idevice_t device) {
    PROFILE_OPERATION("screenshot");
    
    screenshotr_client_t screenshotr = nullptr;
    // ... 截图实现
    
    return screenshot;
}
```

## 实用示例集合

### 设备管理完整示例

```cpp
// 完整的设备管理类，包含所有基础功能
class DeviceManager {
private:
    QMap<QString, idevice_t> deviceConnections_;  // 设备连接池
    QSet<QString> pairedDevices_;                  // 已配对设备列表
    QMutex deviceMutex_;                           // 线程安全锁
    QTimer* heartbeatTimer_;                       // 心跳定时器
    
public:
    DeviceManager(QObject* parent = nullptr) : QObject(parent) {
        // 设置心跳定时器，每30秒检查一次设备状态
        heartbeatTimer_ = new QTimer(this);
        connect(heartbeatTimer_, &QTimer::timeout, this, &DeviceManager::checkAllDevices);
        heartbeatTimer_->start(30000);
        
        // 初始扫描
        scanForDevices();
    }
    
    ~DeviceManager() {
        // 清理所有设备连接
        QMutexLocker locker(&deviceMutex_);
        for (auto it = deviceConnections_.begin(); it != deviceConnections_.end(); ++it) {
            idevice_free(it.value());
        }
        deviceConnections_.clear();
    }
    
    // 获取所有可用设备
    QStringList getAvailableDevices() {
        QStringList devices;
        char **device_list = nullptr;
        int count = 0;
        
        if (idevice_get_device_list(&device_list, &count) == IDEVICE_E_SUCCESS) {
            for (int i = 0; i < count; i++) {
                devices << QString::fromUtf8(device_list[i]);
            }
            idevice_device_list_free(device_list);
        }
        
        return devices;
    }
    
    // 获取设备连接（使用连接池）
    idevice_t getDeviceConnection(const QString& udid) {
        QMutexLocker locker(&deviceMutex_);
        
        if (deviceConnections_.contains(udid)) {
            // 测试连接是否仍然有效
            if (isConnectionValid(deviceConnections_[udid])) {
                return deviceConnections_[udid];
            } else {
                // 连接无效，移除并重新创建
                idevice_free(deviceConnections_[udid]);
                deviceConnections_.remove(udid);
            }
        }
        
        // 创建新连接
        idevice_t device = nullptr;
        if (idevice_new(&device, udid.toUtf8().constData()) == IDEVICE_E_SUCCESS) {
            deviceConnections_[udid] = device;
            return device;
        }
        
        return nullptr;
    }
    
    // 测试连接是否有效
    bool isConnectionValid(idevice_t device) {
        if (!device) return false;
        
        // 尝试获取设备UDID作为连接测试
        char *udid = nullptr;
        bool isValid = (idevice_get_udid(device, &udid) == IDEVICE_E_SUCCESS);
        
        if (udid) {
            free(udid);
        }
        
        return isValid;
    }
    
signals:
    void deviceConnected(const QString& udid);
    void deviceDisconnected(const QString& udid);
    
private slots:
    void scanForDevices() {
        QStringList currentDevices = getAvailableDevices();
        QStringList previousDevices = deviceConnections_.keys();
        
        // 检查新连接的设备
        for (const QString& udid : currentDevices) {
            if (!previousDevices.contains(udid)) {
                emit deviceConnected(udid);
                qDebug() << "检测到新设备:" << udid;
            }
        }
        
        // 检查断开的设备
        for (const QString& udid : previousDevices) {
            if (!currentDevices.contains(udid)) {
                // 从连接池中移除
                QMutexLocker locker(&deviceMutex_);
                if (deviceConnections_.contains(udid)) {
                    idevice_free(deviceConnections_[udid]);
                    deviceConnections_.remove(udid);
                }
                emit deviceDisconnected(udid);
                qDebug() << "设备断开连接:" << udid;
            }
        }
    }
    
    void checkAllDevices() {
        // 心跳检查所有连接的设备
        QMutexLocker locker(&deviceMutex_);
        for (auto it = deviceConnections_.begin(); it != deviceConnections_.end(); ) {
            if (!isConnectionValid(it.value())) {
                qDebug() << "设备连接失效:" << it.key();
                emit deviceDisconnected(it.key());
                idevice_free(it.value());
                it = deviceConnections_.erase(it);
            } else {
                ++it;
            }
        }
    }
};
```

### 应用管理完整示例

```cpp
// 应用管理器实现
class AppManager {
private:
    DeviceManager* deviceManager_;
    QMap<QString, QPixmap> iconCache_;  // 应用图标缓存
    
public:
    AppManager(DeviceManager* deviceManager, QObject* parent = nullptr)
        : QObject(parent), deviceManager_(deviceManager) {
    }
    
    // 获取所有已安装应用
    QList<AppInfo> getAllInstalledApps(const QString& udid) {
        QList<AppInfo> apps;
        
        idevice_t device = deviceManager_->getDeviceConnection(udid);
        if (!device) {
            qWarning() << "无法连接到设备:" << udid;
            return apps;
        }
        
        instproxy_client_t instproxy = nullptr;
        if (instproxy_client_start_service(device, &instproxy, "phone-linkc") != INSTPROXY_E_SUCCESS) {
            qWarning() << "无法启动应用代理服务";
            deviceManager_->releaseDeviceConnection(udid);
            return apps;
        }
        
        // 设置浏览选项
        plist_t options = plist_new_dict();
        plist_t app_types = plist_new_array();
        plist_array_append_item(app_types, plist_new_string("User"));
        plist_array_append_item(app_types, plist_new_string("System"));
        plist_dict_set_item(options, "ApplicationType", app_types);
        
        // 获取应用列表
        plist_t result = nullptr;
        if (instproxy_browse(instproxy, options, &result) == INSTPROXY_E_SUCCESS && result) {
            uint32_t app_count = plist_array_get_size(result);
            
            for (uint32_t i = 0; i < app_count; i++) {
                plist_t app_dict = plist_array_get_item(result, i);
                if (app_dict) {
                    AppInfo appInfo = parseAppInfo(app_dict);
                    if (!appInfo.bundleId.isEmpty()) {
                        apps.append(appInfo);
                    }
                }
            }
            
            plist_free(result);
        }
        
        plist_free(options);
        instproxy_client_free(instproxy);
        deviceManager_->releaseDeviceConnection(udid);
        
        return apps;
    }
    
    // 获取应用图标
    QPixmap getAppIcon(const QString& udid, const QString& bundleId) {
        // 检查缓存
        QString cacheKey = QString("%1:%2").arg(udid, bundleId);
        if (iconCache_.contains(cacheKey)) {
            return iconCache_[cacheKey];
        }
        
        QPixmap icon;
        
        idevice_t device = deviceManager_->getDeviceConnection(udid);
        if (!device) {
            return icon;
        }
        
        sbservices_client_t sbservices = nullptr;
        if (sbservices_client_start_service(device, &sbservices, "phone-linkc") != SBSERVICES_E_SUCCESS) {
            deviceManager_->releaseDeviceConnection(udid);
            return icon;
        }
        
        char *pngdata = nullptr;
        uint64_t pngsize = 0;
        
        if (sbservices_get_icon_pngdata(sbservices, bundleId.toUtf8().constData(), 
                                        &pngdata, &pngsize) == SBSERVICES_E_SUCCESS && pngdata) {
            icon.loadFromData(reinterpret_cast<const uchar*>(pngdata), static_cast<int>(pngsize), "PNG");
            free(pngdata);
            
            // 缓存图标
            iconCache_[cacheKey] = icon;
        }
        
        sbservices_client_free(sbservices);
        deviceManager_->releaseDeviceConnection(udid);
        
        return icon;
    }
    
    // 安装应用
    bool installApp(const QString& udid, const QString& ipaPath, 
                    const QHash<QString, QVariant>& options = QHash<QString, QVariant>()) {
        idevice_t device = deviceManager_->getDeviceConnection(udid);
        if (!device) {
            return false;
        }
        
        instproxy_client_t instproxy = nullptr;
        if (instproxy_client_start_service(device, &instproxy, "phone-linkc") != INSTPROXY_E_SUCCESS) {
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 准备安装选项
        plist_t client_options = plist_new_dict();
        
        // 基础选项
        plist_dict_set_item(client_options, "ApplicationType", 
                           plist_new_string(options.value("ApplicationType", "User").toString().toUtf8().constData()));
        
        // 处理其他选项
        if (options.contains("SkipUninstall")) {
            plist_dict_set_item(client_options, "SkipUninstall", 
                               plist_new_bool(options.value("SkipUninstall").toBool()));
        }
        
        // 执行安装
        QByteArray ipaPathBytes = ipaPath.toUtf8();
        instproxy_error_t error = instproxy_install(instproxy, ipaPathBytes.constData(), 
                                                   client_options, installStatusCallback, this);
        
        bool success = (error == INSTPROXY_E_SUCCESS);
        
        plist_free(client_options);
        instproxy_client_free(instproxy);
        deviceManager_->releaseDeviceConnection(udid);
        
        return success;
    }
    
signals:
    void installProgress(int percentage);
    void installStatusChanged(const QString& status);
    void errorOccurred(const QString& message);
    
private:
    // 解析应用信息
    AppInfo parseAppInfo(plist_t appDict) {
        AppInfo info;
        
        // 使用辅助函数获取值
        info.bundleId = getPlistStringValue(appDict, "CFBundleIdentifier");
        info.displayName = getPlistStringValue(appDict, "CFBundleDisplayName");
        info.version = getPlistStringValue(appDict, "CFBundleShortVersionString");
        info.bundleVersion = getPlistStringValue(appDict, "CFBundleVersion");
        
        // 检查是否为系统应用
        QString appType = getPlistStringValue(appDict, "ApplicationType");
        info.isSystemApp = (appType == "System");
        
        // 获取安装日期
        plist_t installDateNode = plist_dict_get_item(appDict, "InstallDate");
        if (installDateNode && plist_get_node_type(installDateNode) == PLIST_DATE) {
            int32_t secs = 0, usecs = 0;
            plist_get_date_val(installDateNode, &secs, &usecs);
            info.installDate = QDateTime::fromSecsSinceEpoch(secs);
        }
        
        return info;
    }
    
    // 获取plist字符串值
    QString getPlistStringValue(plist_t dict, const char* key) {
        plist_t value = plist_dict_get_item(dict, key);
        if (value && plist_get_node_type(value) == PLIST_STRING) {
            char *str_value = nullptr;
            plist_get_string_val(value, &str_value);
            if (str_value) {
                QString result = QString::fromUtf8(str_value);
                free(str_value);
                return result;
            }
        }
        return QString();
    }
    
    // 安装状态回调
    static void installStatusCallback(const char *operation, plist_t status, void *user_data) {
        AppManager* manager = static_cast<AppManager*>(user_data);
        
        if (!status) return;
        
        // 获取状态
        plist_t statusNode = plist_dict_get_item(status, "Status");
        if (statusNode) {
            char *statusStr = nullptr;
            plist_get_string_val(statusNode, &statusStr);
            if (statusStr) {
                QString statusString = QString::fromUtf8(statusStr);
                emit manager->installStatusChanged(statusString);
                
                if (statusString == "Complete") {
                    emit manager->installProgress(100);
                }
                free(statusStr);
            }
        }
        
        // 获取进度
        plist_t progressNode = plist_dict_get_item(status, "PercentComplete");
        if (progressNode) {
            uint64_t progress = 0;
            plist_get_uint_val(progressNode, &progress);
            emit manager->installProgress(static_cast<int>(progress));
        }
        
        // 获取错误信息
        plist_t errorNode = plist_dict_get_item(status, "ErrorDescription");
        if (errorNode) {
            char *errorStr = nullptr;
            plist_get_string_val(errorNode, &errorStr);
            if (errorStr) {
                QString errorMessage = QString::fromUtf8(errorStr);
                emit manager->errorOccurred(QString("安装错误: %1").arg(errorMessage));
                free(errorStr);
            }
        }
    }
};
```

### 文件传输完整示例

```cpp
// 文件传输管理器实现
class FileManager {
private:
    DeviceManager* deviceManager_;
    
public:
    FileManager(DeviceManager* deviceManager, QObject* parent = nullptr)
        : QObject(parent), deviceManager_(deviceManager) {
    }
    
    // 上传文件到设备
    bool uploadFile(const QString& udid, const QString& localPath, 
                   const QString& remotePath, bool overwrite = true) {
        QFile localFile(localPath);
        if (!localFile.open(QIODevice::ReadOnly)) {
            emit errorOccurred(QString("无法打开本地文件: %1").arg(localPath));
            return false;
        }
        
        idevice_t device = deviceManager_->getDeviceConnection(udid);
        if (!device) {
            return false;
        }
        
        afc_client_t afc = nullptr;
        if (afc_client_start_service(device, &afc, "phone-linkc") != AFC_E_SUCCESS) {
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 检查文件是否已存在
        if (!overwrite && fileExists(afc, remotePath)) {
            afc_client_free(afc);
            deviceManager_->releaseDeviceConnection(udid);
            return false;
        }
        
        // 确保远程目录存在
        QString remoteDir = QFileInfo(remotePath).path();
        if (!ensureDirectoryExists(afc, remoteDir)) {
            afc_client_free(afc);
            deviceManager_->releaseDeviceConnection(udid);
            emit errorOccurred(QString("无法创建远程目录: %1").arg(remoteDir));
            return false;
        }
        
        // 打开远程文件
        uint64_t handle = 0;
        QByteArray remotePathBytes = remotePath.toUtf8();
        if (afc_file_open(afc, remotePathBytes.constData(), AFC_FOPEN_WRONLY, &handle) != AFC_E_SUCCESS) {
            afc_client_free(afc);
            deviceManager_->releaseDeviceConnection(udid);
            emit errorOccurred(QString("无法打开远程文件: %1").arg(remotePath));
            return false;
        }
        
        // 上传文件内容
        qint64 totalSize = localFile.size();
        qint64 transferred = 0;
        
        const int BUFFER_SIZE = 65536; // 64KB缓冲区
        char buffer[BUFFER_SIZE];
        
        while (!localFile.atEnd()) {
            qint64 bytesRead = localFile.read(buffer, BUFFER_SIZE);
            if (bytesRead <= 0) break;
            
            uint32_t bytesWritten = 0;
            if (afc_file_write(afc, handle, buffer, bytesRead, &bytesWritten) != AFC_E_SUCCESS ||
                bytesWritten != static_cast<uint32_t>(bytesRead)) {
                afc_file_close(afc, handle);
                afc_client_free(afc);
                deviceManager_->releaseDeviceConnection(udid);
                emit errorOccurred(QString("写入文件失败: %1").arg(remotePath));
                return false;
            }
            
            transferred += bytesWritten;
            
            // 发送进度更新
            emit uploadProgress(transferred, totalSize);
        }
        
        // 清理资源
        afc_file_close(afc, handle);
        afc_client_free(afc);
        deviceManager_->releaseDeviceConnection(udid);
        
        return true;
    }
    
signals:
    void uploadProgress(qint64 transferred, qint64 total);
    void downloadProgress(qint64 transferred, qint64 total);
    void errorOccurred(const QString& message);
    
private:
    // 检查文件是否存在
    bool fileExists(afc_client_t afc, const QString& path) {
        char **list = nullptr;
        QByteArray pathBytes = path.toUtf8();
        
        if (afc_read_directory(afc, pathBytes.constData(), &list) == AFC_E_SUCCESS) {
            afc_dictionary_free(list);
            return true;
        }
        
        return false;
    }
    
    // 确保目录存在
    bool ensureDirectoryExists(afc_client_t afc, const QString& path) {
        // 简化实现，实际应递归检查和创建目录
        return afc_make_directory(afc, path.toUtf8().constData()) == AFC_E_SUCCESS;
    }
    
    // 获取文件大小
    uint64_t getFileSize(afc_client_t afc, const QString& path) {
        char **info = nullptr;
        uint64_t size = 0;
        
        if (afc_get_file_info(afc, path.toUtf8().constData(), &info) == AFC_E_SUCCESS) {
            for (int i = 0; info[i]; i += 2) {
                if (QString(info[i]) == "st_size" && info[i+1]) {
                    size = QString(info[i+1]).toULongLong();
                    break;
                }
            }
            afc_dictionary_free(info);
        }
        
        return size;
    }
};
```

---

**注意事项**:
1. 本API参考基于libimobiledevice v1.3.x版本
2. 某些功能可能需要设备处于特定状态（如开发者模式）
3. 部分高级功能可能需要设备越狱
4. 使用前请确保已正确配置开发环境和依赖库
5. 建议在实际使用中根据具体需求进行错误处理和异常捕获

**相关文档**:
- [功能使用指南](FUNCTION-GUIDE.md)
- [基础说明文档](README.md)
- [官方文档](https://libimobiledevice.org/)
