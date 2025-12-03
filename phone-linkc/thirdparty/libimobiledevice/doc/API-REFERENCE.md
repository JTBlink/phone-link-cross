# libimobiledevice API接口明细说明

## 概述

本文档详细说明了libimobiledevice库的核心API接口，包括函数原型、参数说明、返回值和使用示例。

## 目录结构

```
libimobiledevice/
├── imobiledevice.dll           # 核心库文件
├── plist.dll                   # 属性列表处理库
├── usbmuxd.dll                # USB复用守护进程库
├── idevice_id.exe             # 设备ID查询工具
├── usbmuxd.exe                # USB复用守护进程
├── README.md                   # 基础说明文档
├── API-REFERENCE.md           # API接口明细（本文档）
├── FUNCTION-GUIDE.md          # 功能使用指南
└── [其他依赖库文件...]
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
- `IDEVICE_E_SUCCESS`: 成功
- `IDEVICE_E_INVALID_ARG`: 参数无效
- `IDEVICE_E_NO_DEVICE`: 没有找到设备

**使用示例**:
```cpp
char **device_list = NULL;
int device_count = 0;
idevice_error_t error = idevice_get_device_list(&device_list, &device_count);
if (error == IDEVICE_E_SUCCESS) {
    for (int i = 0; i < device_count; i++) {
        qDebug() << "发现设备:" << device_list[i];
    }
    idevice_device_list_free(device_list);
}
```

##### idevice_new()
```c
idevice_error_t idevice_new(idevice_t *device, const char *udid);
```

**功能描述**: 创建设备连接实例

**参数说明**:
- `device`: 输出参数，设备句柄
- `udid`: 设备UDID，NULL表示连接第一个可用设备

**返回值**:
- `IDEVICE_E_SUCCESS`: 连接成功
- `IDEVICE_E_NO_DEVICE`: 设备不存在
- `IDEVICE_E_TIMEOUT`: 连接超时

**使用示例**:
```cpp
idevice_t device = NULL;
idevice_error_t error = idevice_new(&device, NULL);
if (error == IDEVICE_E_SUCCESS) {
    // 设备连接成功，可以进行操作
    qDebug() << "设备连接成功";
    idevice_free(device);
}
```

##### idevice_free()
```c
idevice_error_t idevice_free(idevice_t device);
```

**功能描述**: 释放设备连接资源

**参数说明**:
- `device`: 要释放的设备句柄

**返回值**:
- `IDEVICE_E_SUCCESS`: 释放成功

#### 1.2 设备信息获取

##### idevice_get_udid()
```c
idevice_error_t idevice_get_udid(idevice_t device, char **udid);
```

**功能描述**: 获取设备UDID

**参数说明**:
- `device`: 设备句柄
- `udid`: 输出参数，设备UDID字符串

**返回值**:
- `IDEVICE_E_SUCCESS`: 获取成功

**使用示例**:
```cpp
char *udid = NULL;
idevice_error_t error = idevice_get_udid(device, &udid);
if (error == IDEVICE_E_SUCCESS) {
    qDebug() << "设备UDID:" << udid;
    free(udid);
}
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

## 调试和诊断

### 启用调试输出
```cpp
#include "libimobiledevice/libimobiledevice.h"

// 设置调试回调
void debug_callback(const char* message) {
    qDebug() << "libimobiledevice debug:" << message;
}

// 在应用初始化时启用调试
void enableDebugOutput() {
    idevice_set_debug_level(1);  // 设置调试级别
    // 注意：某些版本可能不支持回调设置
}
```

### 常见问题诊断

#### 1. 设备连接问题
```cpp
bool diagnoseDeviceConnection(const QString& udid) {
    // 检查设备列表
    char **device_list = NULL;
    int count = 0;
    idevice_get_device_list(&device_list, &count);
    
    bool found = false;
    for (int i = 0; i < count; i++) {
        if (QString(device_list[i]) == udid) {
            found = true;
            break;
        }
    }
    idevice_device_list_free(device_list);
    
    if (!found) {
        qWarning() << "设备未在设备列表中找到";
        return false;
    }
    
    // 尝试连接
    idevice_t device = NULL;
    idevice_error_t error = idevice_new(&device, udid.toStdString().c_str());
    if (error != IDEVICE_E_SUCCESS) {
        qWarning() << "设备连接失败:" << getErrorMessage(error);
        return false;
    }
    
    idevice_free(device);
    qDebug() << "设备连接诊断通过";
    return true;
}
```

#### 2. 服务可用性检查
```cpp
bool checkServiceAvailability(idevice_t device, const QString& serviceName) {
    lockdownd_client_t lockdown = NULL;
    lockdownd_error_t error = lockdownd_client_new(device, &lockdown, "phone-linkc");
    if (error != LOCKDOWN_E_SUCCESS) {
        return false;
    }
    
    lockdownd_service_descriptor_t service = NULL;
    error = lockdownd_start_service(lockdown, serviceName.toStdString().c_str(), &service);
    
    lockdownd_client_free(lockdown);
    
    if (error == LOCKDOWN_E_SUCCESS && service) {
        lockdownd_service_descriptor_free(service);
        return true;
    }
    
    return false;
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
