# libimobiledevice API接口明细说明

## 概述

本文档详细说明了libimobiledevice库的核心API接口，包括函数原型、参数说明、返回值和使用示例。该文档专门针对phone-linkc项目进行优化，提供实际项目中的最佳实践。

## 目录结构

```
libimobiledevice/
├── imobiledevice.dll           # 核心库文件
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

## 版本兼容性

- **libimobiledevice版本**: v1.3.17+
- **支持的iOS版本**: iOS 7.0 - iOS 17.x
- **平台支持**: Windows 10/11, macOS 10.12+, Linux
- **编译器要求**: 支持C11标准的编译器
- **Qt版本**: Qt 5.15+ 或 Qt 6.2+（用于phone-linkc项目）

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
**功能**: 释放plist内存

### 使用示例

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
