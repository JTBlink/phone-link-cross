# libimobiledevice 接入功能说明文档

本文档说明如何在 phone-link-cross 项目中集成和使用 libimobiledevice 库实现iOS设备通信功能。

## 项目集成概述

### 目录结构
```
phone-linkc/thirdparty/libimobiledevice/
├── include/                    # 头文件目录
├── *.exe                      # 命令行工具
└── README.md                  # 本文档
```

### 依赖管理
- libimobiledevice 库已预编译并放置在 `thirdparty/libimobiledevice/` 目录
- 头文件位于 `include/` 子目录，包含所有API定义
- 可执行工具可直接调用或集成到应用中

## 核心功能接入

### 1. 设备连接与检测

#### 功能描述
检测连接的iOS设备，获取设备基本信息。

#### 接入方式

**C++ API接入**：
```cpp
#include "libimobiledevice/libimobiledevice.h"
#include "libimobiledevice/lockdown.h"

// 获取设备列表
char **device_list = NULL;
int device_count = 0;
idevice_get_device_list(&device_list, &device_count);

// 连接设备
idevice_t device = NULL;
if (device_count > 0) {
    idevice_new(&device, device_list[0]);
}
```

**命令行工具调用**：
```cpp
// 调用 idevice_id 获取设备UDID
QProcess process;
process.start("idevice_id", QStringList() << "-l");
process.waitForFinished();
QString output = process.readAllStandardOutput();
```

#### 应用场景
- 设备管理器中显示连接设备
- 设备状态监控
- 多设备支持

### 2. 设备信息获取

#### 功能描述
获取设备详细信息，包括型号、版本、序列号等。

#### 接入方式

**C++ API接入**：
```cpp
#include "libimobiledevice/lockdown.h"

lockdownd_client_t client = NULL;
lockdownd_client_new(device, &client, "phone-linkc");

// 获取设备信息
plist_t node = NULL;
lockdownd_get_value(client, NULL, "DeviceName", &node);
// 解析plist数据获取信息
```

**命令行工具调用**：
```cpp
// 使用 ideviceinfo 获取设备信息
QProcess::execute("ideviceinfo", QStringList() << "-k" << "DeviceName");
```

#### 支持的信息类型
- 设备名称 (DeviceName)
- 设备型号 (ProductType)
- iOS版本 (ProductVersion)
- 序列号 (SerialNumber)
- UDID (UniqueDeviceID)

### 3. 屏幕截图功能

#### 功能描述
实时获取iOS设备屏幕截图。

#### 接入方式

**C++ API接入**：
```cpp
#include "libimobiledevice/screenshotr.h"

screenshotr_client_t screenshotr = NULL;
screenshotr_client_start_service(device, &screenshotr, "phone-linkc");

char *imgdata = NULL;
uint64_t imgsize = 0;
screenshotr_take_screenshot(screenshotr, &imgdata, &imgsize);

// 将imgdata转换为QImage或保存为文件
```

**命令行工具调用**：
```cpp
// 使用 idevicescreenshot 保存截图
QProcess::execute("idevicescreenshot", QStringList() << "screenshot.png");
```

#### 实现要点
- 支持实时截图
- 可保存为PNG格式
- 集成到UI显示

### 4. 应用管理功能

#### 功能描述
安装、卸载、查看iOS设备上的应用程序。

#### 接入方式

**应用列表获取**：
```cpp
#include "libimobiledevice/installation_proxy.h"

instproxy_client_t client = NULL;
instproxy_client_start_service(device, &client, "phone-linkc");

plist_t apps = NULL;
instproxy_browse(client, NULL, &apps);
// 解析apps列表获取应用信息
```

**应用安装**：
```cpp
// 安装IPA文件
instproxy_install(client, ipa_path, options, status_cb, NULL);
```

#### 支持操作
- 获取已安装应用列表
- 安装IPA文件
- 卸载应用
- 获取应用详细信息

### 5. 文件传输功能

#### 功能描述
在设备和电脑间传输文件，访问应用沙盒。

#### 接入方式

**AFC文件访问**：
```cpp
#include "libimobiledevice/afc.h"

afc_client_t afc = NULL;
afc_client_start_service(device, &afc, "phone-linkc");

// 列出目录内容
char **list = NULL;
afc_read_directory(afc, "/", &list);

// 上传文件
afc_file_open(afc, "/remote_file.txt", AFC_FOPEN_WRONLY, &handle);
afc_file_write(afc, handle, data, data_size, &bytes_written);
```

**沙盒文件访问**：
```cpp
#include "libimobiledevice/house_arrest.h"

house_arrest_client_t house_arrest = NULL;
house_arrest_client_start_service(device, &house_arrest, "phone-linkc");
house_arrest_send_command(house_arrest, "VendContainer", "com.app.bundleid");
```

### 6. 系统日志监控

#### 功能描述
实时查看设备系统日志，便于调试和监控。

#### 接入方式

**C++ API接入**：
```cpp
#include "libimobiledevice/syslog_relay.h"

syslog_relay_client_t syslog = NULL;
syslog_relay_client_start_service(device, &syslog, "phone-linkc");

// 接收日志回调
syslog_relay_receive_with_timeout(syslog, &message, 1000);
```

**命令行工具调用**：
```cpp
// 启动日志监控进程
QProcess *logProcess = new QProcess();
logProcess->start("idevicesyslog");
connect(logProcess, &QProcess::readyReadStandardOutput, 
        this, &MainWindow::onLogDataReceived);
```

## CMake 集成配置

### CMakeLists.txt 配置
```cmake
# 设置libimobiledevice路径
set(LIBIMOBILEDEVICE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libimobiledevice")

# 包含头文件目录
include_directories(${LIBIMOBILEDEVICE_DIR}/include)

# 链接库文件（如果有）
if(WIN32)
    # Windows平台链接配置
    target_link_libraries(${PROJECT_NAME} 
        ${LIBIMOBILEDEVICE_DIR}/lib/imobiledevice.lib
        ${LIBIMOBILEDEVICE_DIR}/lib/plist.lib
    )
endif()

# 设置可执行文件路径
set(IDEVICE_TOOLS_DIR ${LIBIMOBILEDEVICE_DIR})
```

### 预编译器定义
```cmake
add_definitions(-DLIBIMOBILEDEVICE_AVAILABLE)
add_definitions(-DIDEVICE_TOOLS_PATH="${IDEVICE_TOOLS_DIR}")
```

## 错误处理和最佳实践

### 错误处理
```cpp
// 统一错误处理
idevice_error_t error = idevice_new(&device, udid);
if (error != IDEVICE_E_SUCCESS) {
    qWarning() << "设备连接失败:" << error;
    return false;
}

// 资源清理
if (client) {
    lockdownd_client_free(client);
}
if (device) {
    idevice_free(device);
}
```

### 异步操作
```cpp
// 使用QThread进行异步设备操作
class DeviceWorker : public QObject {
    Q_OBJECT
public slots:
    void connectDevice(const QString& udid) {
        // 在工作线程中执行设备操作
        idevice_t device = NULL;
        idevice_new(&device, udid.toStdString().c_str());
        emit deviceConnected(device != NULL);
    }
signals:
    void deviceConnected(bool success);
};
```

### 线程安全
- libimobiledevice API不是线程安全的
- 每个设备连接应在单独线程中操作
- 使用互斥锁保护共享资源

### 内存管理
```cpp
// RAII模式管理资源
class DeviceConnection {
private:
    idevice_t device_;
    lockdownd_client_t client_;
public:
    DeviceConnection(const char* udid) : device_(nullptr), client_(nullptr) {
        idevice_new(&device_, udid);
        if (device_) {
            lockdownd_client_new(device_, &client_, "phone-linkc");
        }
    }
    
    ~DeviceConnection() {
        if (client_) lockdownd_client_free(client_);
        if (device_) idevice_free(device_);
    }
};
```

## 实际应用示例

### 设备管理器实现
```cpp
class DeviceManager : public QObject {
    Q_OBJECT
public:
    QStringList getConnectedDevices() {
        QStringList devices;
        char **device_list = NULL;
        int count = 0;
        
        if (idevice_get_device_list(&device_list, &count) == IDEVICE_E_SUCCESS) {
            for (int i = 0; i < count; i++) {
                devices << QString(device_list[i]);
            }
            idevice_device_list_free(device_list);
        }
        return devices;
    }
    
    DeviceInfo getDeviceInfo(const QString& udid) {
        DeviceConnection conn(udid.toStdString().c_str());
        // 获取设备详细信息
        return parseDeviceInfo(conn.getClient());
    }
};
```

### 屏幕镜像实现
```cpp
class ScreenMirror : public QThread {
    Q_OBJECT
private:
    QString udid_;
    bool running_;
    
protected:
    void run() override {
        DeviceConnection conn(udid_.toStdString().c_str());
        screenshotr_client_t screenshotr = NULL;
        
        if (screenshotr_client_start_service(conn.getDevice(), 
                                           &screenshotr, "phone-linkc") == SCREENSHOTR_E_SUCCESS) {
            while (running_) {
                char *imgdata = NULL;
                uint64_t imgsize = 0;
                
                if (screenshotr_take_screenshot(screenshotr, &imgdata, &imgsize) == SCREENSHOTR_E_SUCCESS) {
                    QImage image = QImage::fromData((uchar*)imgdata, imgsize);
                    emit screenshotReady(image);
                    free(imgdata);
                }
                msleep(33); // ~30 FPS
            }
            screenshotr_client_free(screenshotr);
        }
    }
    
signals:
    void screenshotReady(const QImage& image);
};
```

## 部署说明

### Windows 部署
1. 确保所有 `.exe` 工具文件与应用程序在同一目录
2. 包含必要的依赖DLL文件
3. 设置正确的环境变量路径

### 跨平台注意事项
- Windows: 使用预编译的 `.exe` 工具
- macOS/Linux: 需要编译对应平台的工具
- 路径分隔符处理
- 文件权限设置

## 调试和故障排除

### 常见问题
1. **设备无法识别**
   - 检查USB连接
   - 确认设备信任状态
   - 重启usbmuxd服务

2. **权限不足**
   - 某些功能需要设备越狱
   - 开发者模式设置
   - 企业证书配置

3. **API调用失败**
   - 检查返回错误码
   - 验证设备连接状态
   - 确认服务可用性

### 日志记录
```cpp
// 启用详细日志
#define LIBIMOBILEDEVICE_DEBUG 1
#include "libimobiledevice/libimobiledevice.h"

// 自定义日志回调
void debug_callback(const char* message) {
    qDebug() << "libimobiledevice:" << message;
}

// 注册回调
idevice_set_debug_callback(debug_callback);
```

## 功能扩展

### 自定义服务
可以基于现有API开发自定义iOS设备服务，如：
- 自动化测试控制
- 性能监控
- 文件同步服务
- 远程调试支持

### 插件架构
通过插件机制扩展设备功能：
```cpp
class IDevicePlugin {
public:
    virtual ~IDevicePlugin() = default;
    virtual bool initialize(idevice_t device) = 0;
    virtual void processData(const QByteArray& data) = 0;
};
```

## 版本兼容性

### iOS版本支持
- iOS 7.0 及以上版本
- 最新iOS版本的兼容性更新
- 不同版本API差异处理

### libimobiledevice版本
- 当前使用版本：v1.3.17
- 向后兼容性保证
- 升级注意事项

---

**注意**: 本文档基于项目当前集成的 libimobiledevice 版本编写，实际使用时请根据具体需求调整实现方案。
