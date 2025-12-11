# libimobiledevice 功能使用指南

> **版本**: libimobiledevice v1.4.0+ | 最后更新: 2025-12-11

## 概述

本文档提供libimobiledevice v1.4.0+在phone-link-cross项目中的具体功能实现指南，包括完整的代码示例、最佳实践和常见问题解决方案。

### v1.4.0 主要更新

- ✅ **新增事件订阅API**: `idevice_events_subscribe()` / `idevice_events_unsubscribe()` 替代旧版API
- ✅ **网络设备支持**: 支持通过WiFi连接的iOS设备，新增 `idevice_get_device_list_extended()` 和 `idevice_new_with_options()`
- ✅ **统一内存管理**: plist相关内存使用 `plist_mem_free()` 统一释放，替代 `plist_to_xml_free()` 等专用函数
- ✅ **新增依赖库**: libimobiledevice-glue、libtatsu、Brotli压缩库、OpenSSL 3.x等
- ✅ **性能优化**: 文件传输提升15-20%，优化连接管理和内存使用
- ✅ **向后兼容**: 保持与v1.3.17的API兼容性，支持优雅降级

### 重要提示

- 📌 本文档所有代码示例均基于 **libimobiledevice v1.4.0+**
- 📌 如使用旧版本(v1.3.17)，部分新API可能不可用，需回退到旧版API
- 📌 phone-linkc项目采用动态加载机制，可自动适配不同版本

## 快速开始

### 1. 环境配置

#### 1.1 依赖检查
```cpp
// DeviceManager.h
#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QMutex>
#include <QImage>

// libimobiledevice v1.4.0+ 头文件
extern "C" {
#include "libimobiledevice/libimobiledevice.h"
#include "libimobiledevice/lockdown.h"
#include "libimobiledevice/screenshotr.h"
#include "libimobiledevice/installation_proxy.h"
#include "libimobiledevice/afc.h"
#include "libimobiledevice/syslog_relay.h"
#include "libimobiledevice/mobilebackup2.h"
#include "libimobiledevice/notification_proxy.h"
}

class DeviceManager : public QObject {
    Q_OBJECT

public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();
    
    // 基础功能
    bool isLibraryAvailable();
    QStringList getConnectedDevices();
    bool connectToDevice(const QString& udid);
    void disconnectFromDevice();
    
    // 设备信息
    QString getDeviceName();
    QString getDeviceModel();
    QString getIOSVersion();
    QString getSerialNumber();
    
    // 屏幕功能
    QImage takeScreenshot();
    void startScreenMirroring();
    void stopScreenMirroring();
    
    // v1.4.0+ 事件订阅
    void startEventSubscription();
    void stopEventSubscription();
    
signals:
    void deviceConnected(const QString& udid);
    void deviceDisconnected(const QString& udid);
    void screenshotReady(const QImage& image);
    void errorOccurred(const QString& message);
    
private:
    idevice_t device_;
    lockdownd_client_t lockdown_;
    idevice_subscription_context_t subscriptionContext_;  // v1.4.0+ 事件订阅上下文
    QMutex deviceMutex_;
    QString currentUdid_;
    
    bool initializeDevice(const QString& udid);
    void cleanupDevice();
    QString getErrorMessage(int errorCode);
    
    // v1.4.0+ 事件回调
    static void deviceEventCallback(const idevice_event_t *event, void *user_data);
};

#endif // DEVICEMANAGER_H
```

#### 1.2 CMakeLists.txt 配置
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(phone-linkc)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# 设置libimobiledevice v1.4.0+ 路径
set(LIBIMOBILEDEVICE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libimobiledevice")

# 包含目录
include_directories(${LIBIMOBILEDEVICE_DIR}/include)

# 源文件
set(SOURCES
    main.cpp
    mainwindow.cpp
    devicemanager.cpp
    deviceinfo.cpp
)

set(HEADERS
    mainwindow.h
    devicemanager.h
    deviceinfo.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 链接Qt库
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets)

# Windows特定配置
if(WIN32)
    # 设置运行时库路径
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=${LIBIMOBILEDEVICE_DIR};$ENV{PATH}"
    )
    
    # 复制所有DLL文件到输出目录（包含v1.4.0新增依赖）
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${LIBIMOBILEDEVICE_DIR}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "复制 libimobiledevice v1.4.0+ DLL文件"
    )
endif()

# 预定义宏
target_compile_definitions(${PROJECT_NAME} PRIVATE
    LIBIMOBILEDEVICE_AVAILABLE
    LIBIMOBILEDEVICE_VERSION_1_4_0  # 标识使用v1.4.0+
    IDEVICE_TOOLS_PATH="${LIBIMOBILEDEVICE_DIR}"
)
```

### 2. 核心功能实现

#### 2.1 设备管理器实现

```cpp
// DeviceManager.cpp
#include "devicemanager.h"
#include <QDebug>
#include <QProcess>
#include <QCoreApplication>

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , device_(nullptr)
    , lockdown_(nullptr)
    , subscriptionContext_(nullptr)
{
    // v1.4.0+ 使用事件订阅而非定时轮询
    startEventSubscription();
}

DeviceManager::~DeviceManager() {
    stopEventSubscription();
    cleanupDevice();
}

bool DeviceManager::isLibraryAvailable() {
    // 检查是否能获取设备列表来验证库可用性
    char **device_list = nullptr;
    int count = 0;
    idevice_error_t error = idevice_get_device_list(&device_list, &count);
    
    if (device_list) {
        idevice_device_list_free(device_list);
    }
    
    return (error == IDEVICE_E_SUCCESS);
}

QStringList DeviceManager::getConnectedDevices() {
    QStringList devices;
    
    // v1.4.0+ 支持扩展设备列表（包含网络设备）
    idevice_info_t *device_list = nullptr;
    int count = 0;
    
    idevice_error_t error = idevice_get_device_list_extended(&device_list, &count);
    if (error == IDEVICE_E_SUCCESS && device_list) {
        for (int i = 0; i < count; i++) {
            QString udid = QString::fromUtf8(device_list[i].udid);
            devices << udid;
            qDebug() << "设备:" << udid
                     << "连接类型:" << (device_list[i].conn_type == CONNECTION_USBMUXD ? "USB" : "网络");
        }
        idevice_device_list_extended_free(device_list);
    } else {
        // 回退到标准API
        char **simple_list = nullptr;
        error = idevice_get_device_list(&simple_list, &count);
        if (error == IDEVICE_E_SUCCESS && simple_list) {
            for (int i = 0; i < count; i++) {
                devices << QString::fromUtf8(simple_list[i]);
            }
            idevice_device_list_free(simple_list);
        } else {
            qWarning() << "获取设备列表失败:" << getErrorMessage(error);
        }
    }
    
    return devices;
}

bool DeviceManager::connectToDevice(const QString& udid) {
    QMutexLocker locker(&deviceMutex_);
    
    // 清理现有连接
    cleanupDevice();
    
    return initializeDevice(udid);
}

void DeviceManager::disconnectFromDevice() {
    QMutexLocker locker(&deviceMutex_);
    cleanupDevice();
    emit deviceDisconnected(currentUdid_);
    currentUdid_.clear();
}

bool DeviceManager::initializeDevice(const QString& udid) {
    // v1.4.0+ 支持网络设备连接选项
    QByteArray udidBytes = udid.toUtf8();
    idevice_error_t error = idevice_new_with_options(&device_,
                                                      udidBytes.constData(),
                                                      IDEVICE_LOOKUP_USBMUX | IDEVICE_LOOKUP_NETWORK);
    if (error != IDEVICE_E_SUCCESS) {
        // 回退到标准连接方式
        error = idevice_new(&device_, udidBytes.constData());
        if (error != IDEVICE_E_SUCCESS) {
            emit errorOccurred(QString("设备连接失败: %1").arg(getErrorMessage(error)));
            return false;
        }
    }
    
    // 创建lockdown客户端，使用推荐的握手方式
    lockdownd_error_t lockdown_error = lockdownd_client_new_with_handshake(device_, &lockdown_, "phone-linkc");
    if (lockdown_error != LOCKDOWN_E_SUCCESS) {
        emit errorOccurred(QString("Lockdown客户端创建失败: %1").arg(lockdown_error));
        cleanupDevice();
        return false;
    }
    
    currentUdid_ = udid;
    emit deviceConnected(udid);
    qDebug() << "设备连接成功:" << udid;
    
    return true;
}

void DeviceManager::cleanupDevice() {
    if (lockdown_) {
        lockdownd_client_free(lockdown_);
        lockdown_ = nullptr;
    }
    
    if (device_) {
        idevice_free(device_);
        device_ = nullptr;
    }
}

QString DeviceManager::getErrorMessage(int errorCode) {
    switch (errorCode) {
        case IDEVICE_E_SUCCESS: return "成功";
        case IDEVICE_E_INVALID_ARG: return "参数无效";
        case IDEVICE_E_NO_DEVICE: return "设备未找到";
        case IDEVICE_E_NOT_ENOUGH_DATA: return "数据不足";
        case IDEVICE_E_SSL_ERROR: return "SSL错误";
        default: return QString("未知错误 (%1)").arg(errorCode);
    }
}

// v1.4.0+ 使用事件订阅机制
void DeviceManager::startEventSubscription() {
    if (subscriptionContext_) {
        return; // 已经订阅
    }
    
    idevice_error_t ret = idevice_events_subscribe(&subscriptionContext_,
                                                    deviceEventCallback,
                                                    this);
    if (ret == IDEVICE_E_SUCCESS) {
        qDebug() << "成功订阅设备事件（v1.4.0+ API）";
    } else {
        qWarning() << "订阅设备事件失败，错误码:" << ret;
        subscriptionContext_ = nullptr;
    }
}

void DeviceManager::stopEventSubscription() {
    if (subscriptionContext_) {
        idevice_events_unsubscribe(subscriptionContext_);
        subscriptionContext_ = nullptr;
        qDebug() << "已停止设备事件订阅";
    }
}

void DeviceManager::deviceEventCallback(const idevice_event_t *event, void *user_data) {
    DeviceManager *manager = static_cast<DeviceManager*>(user_data);
    if (!manager || !event) {
        return;
    }
    
    QString udid = QString::fromUtf8(event->udid);
    QString connType = (event->conn_type == CONNECTION_USBMUXD) ? "USB" : "网络";
    
    switch (event->event) {
        case IDEVICE_DEVICE_ADD:
            qDebug() << "设备连接事件:" << udid << "类型:" << connType;
            emit manager->deviceConnected(udid);
            break;
            
        case IDEVICE_DEVICE_REMOVE:
            qDebug() << "设备断开事件:" << udid;
            if (udid == manager->currentUdid_) {
                manager->disconnectFromDevice();
            }
            emit manager->deviceDisconnected(udid);
            break;
            
        case IDEVICE_DEVICE_PAIRED:
            qDebug() << "设备配对事件:" << udid;
            break;
    }
}
```

#### 2.2 设备信息获取

```cpp
QString DeviceManager::getDeviceName() {
    if (!lockdown_) return QString();
    
    plist_t value = nullptr;
    lockdownd_error_t error = lockdownd_get_value(lockdown_, nullptr, "DeviceName", &value);
    
    if (error == LOCKDOWN_E_SUCCESS && value) {
        char *name_str = nullptr;
        plist_get_string_val(value, &name_str);
        QString result = QString::fromUtf8(name_str);
        
        plist_mem_free(name_str);  // v1.4.0+ 使用统一内存释放函数
        plist_free(value);
        
        return result;
    }
    
    return QString();
}

QString DeviceManager::getDeviceModel() {
    if (!lockdown_) return QString();
    
    plist_t value = nullptr;
    lockdownd_error_t error = lockdownd_get_value(lockdown_, nullptr, "ProductType", &value);
    
    if (error == LOCKDOWN_E_SUCCESS && value) {
        char *model_str = nullptr;
        plist_get_string_val(value, &model_str);
        QString result = QString::fromUtf8(model_str);
        
        plist_mem_free(model_str);  // v1.4.0+ 使用统一内存释放函数
        plist_free(value);
        
        return result;
    }
    
    return QString();
}

QString DeviceManager::getIOSVersion() {
    if (!lockdown_) return QString();
    
    plist_t value = nullptr;
    lockdownd_error_t error = lockdownd_get_value(lockdown_, nullptr, "ProductVersion", &value);
    
    if (error == LOCKDOWN_E_SUCCESS && value) {
        char *version_str = nullptr;
        plist_get_string_val(value, &version_str);
        QString result = QString::fromUtf8(version_str);
        
        plist_mem_free(version_str);  // v1.4.0+ 使用统一内存释放函数
        plist_free(value);
        
        return result;
    }
    
    return QString();
}

// 获取完整设备信息的便捷方法
struct DeviceInfo {
    QString udid;
    QString name;
    QString model;
    QString iosVersion;
    QString serialNumber;
    QString buildVersion;
    QString hardwareModel;
    QString connectionType;  // v1.4.0+ 新增：连接类型（USB/网络）
};

DeviceInfo DeviceManager::getFullDeviceInfo() {
    DeviceInfo info;
    
    if (!lockdown_) return info;
    
    info.udid = currentUdid_;
    
    // v1.4.0+ 获取连接类型信息
    idevice_info_t *device_list = nullptr;
    int count = 0;
    if (idevice_get_device_list_extended(&device_list, &count) == IDEVICE_E_SUCCESS) {
        for (int i = 0; i < count; i++) {
            if (QString::fromUtf8(device_list[i].udid) == currentUdid_) {
                info.connectionType = (device_list[i].conn_type == CONNECTION_USBMUXD) ? "USB" : "网络";
                break;
            }
        }
        idevice_device_list_extended_free(device_list);
    }
    
    // 批量获取设备信息
    QStringList keys = {
        "DeviceName", "ProductType", "ProductVersion",
        "SerialNumber", "BuildVersion", "HardwareModel"
    };
    
    for (const QString& key : keys) {
        plist_t value = nullptr;
        lockdownd_error_t error = lockdownd_get_value(lockdown_, nullptr,
                                                     key.toUtf8().constData(), &value);
        
        if (error == LOCKDOWN_E_SUCCESS && value) {
            char *str_value = nullptr;
            plist_get_string_val(value, &str_value);
            QString result = QString::fromUtf8(str_value);
            
            if (key == "DeviceName") info.name = result;
            else if (key == "ProductType") info.model = result;
            else if (key == "ProductVersion") info.iosVersion = result;
            else if (key == "SerialNumber") info.serialNumber = result;
            else if (key == "BuildVersion") info.buildVersion = result;
            else if (key == "HardwareModel") info.hardwareModel = result;
            
            plist_mem_free(str_value);  // v1.4.0+ 使用统一内存释放函数
            plist_free(value);
        }
    }
    
    return info;
}
```

#### 2.3 屏幕截图功能

```cpp
QImage DeviceManager::takeScreenshot() {
    if (!device_) {
        emit errorOccurred("设备未连接");
        return QImage();
    }
    
    screenshotr_client_t screenshotr = nullptr;
    screenshotr_error_t error = screenshotr_client_start_service(device_, &screenshotr, "phone-linkc");
    
    if (error != SCREENSHOTR_E_SUCCESS) {
        emit errorOccurred(QString("启动截图服务失败: %1").arg(error));
        return QImage();
    }
    
    char *imgdata = nullptr;
    uint64_t imgsize = 0;
    
    error = screenshotr_take_screenshot(screenshotr, &imgdata, &imgsize);
    
    QImage screenshot;
    if (error == SCREENSHOTR_E_SUCCESS && imgdata) {
        screenshot = QImage::fromData(reinterpret_cast<const uchar*>(imgdata),
                                    static_cast<int>(imgsize), "PNG");
        // 注意：screenshotr返回的数据使用标准free()释放
        // 这不是plist数据，所以不使用plist_mem_free()
        free(imgdata);
        
        if (!screenshot.isNull()) {
            emit screenshotReady(screenshot);
            qDebug() << "截图成功，尺寸:" << screenshot.size();
        }
    } else {
        emit errorOccurred(QString("截图失败: %1").arg(error));
    }
    
    screenshotr_client_free(screenshotr);
    return screenshot;
}

// 屏幕镜像功能（异步）
class ScreenMirrorWorker : public QThread {
    Q_OBJECT

public:
    ScreenMirrorWorker(idevice_t device, QObject *parent = nullptr)
        : QThread(parent), device_(device), running_(false) {}
    
    void stopMirroring() { running_ = false; }

protected:
    void run() override {
        running_ = true;
        
        screenshotr_client_t screenshotr = nullptr;
        screenshotr_error_t error = screenshotr_client_start_service(device_, &screenshotr, "phone-linkc");
        
        if (error != SCREENSHOTR_E_SUCCESS) {
            emit errorOccurred(QString("启动截图服务失败: %1").arg(error));
            return;
        }
        
        while (running_) {
            char *imgdata = nullptr;
            uint64_t imgsize = 0;
            
            error = screenshotr_take_screenshot(screenshotr, &imgdata, &imgsize);
            
            if (error == SCREENSHOTR_E_SUCCESS && imgdata) {
                QImage screenshot = QImage::fromData(reinterpret_cast<const uchar*>(imgdata), 
                                                   static_cast<int>(imgsize), "PNG");
                free(imgdata);
                
                if (!screenshot.isNull()) {
                    emit frameReady(screenshot);
                }
            }
            
            // 控制帧率，大约30fps
            msleep(33);
        }
        
        screenshotr_client_free(screenshotr);
    }

signals:
    void frameReady(const QImage& frame);
    void errorOccurred(const QString& message);

private:
    idevice_t device_;
    bool running_;
};

void DeviceManager::startScreenMirroring() {
    if (!device_) {
        emit errorOccurred("设备未连接");
        return;
    }
    
    ScreenMirrorWorker *worker = new ScreenMirrorWorker(device_, this);
    connect(worker, &ScreenMirrorWorker::frameReady, this, &DeviceManager::screenshotReady);
    connect(worker, &ScreenMirrorWorker::errorOccurred, this, &DeviceManager::errorOccurred);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    
    worker->start();
    
    // 保存worker引用以便停止
    setProperty("mirrorWorker", QVariant::fromValue(worker));
}

void DeviceManager::stopScreenMirroring() {
    ScreenMirrorWorker *worker = property("mirrorWorker").value<ScreenMirrorWorker*>();
    if (worker) {
        worker->stopMirroring();
        worker->wait(3000); // 等待3秒
        setProperty("mirrorWorker", QVariant());
    }
}
```

### 3. 应用管理功能

#### 3.1 应用列表获取

```cpp
// AppManager.h
class AppManager : public QObject {
    Q_OBJECT
    
public:
    struct AppInfo {
        QString bundleId;
        QString displayName;
        QString version;
        QString bundleVersion;
        qint64 installDate;
        bool isSystemApp;
        QIcon icon;
    };
    
    explicit AppManager(DeviceManager *deviceManager, QObject *parent = nullptr);
    
    QList<AppInfo> getInstalledApps();
    bool installApp(const QString& ipaPath);
    bool uninstallApp(const QString& bundleId);
    QIcon getAppIcon(const QString& bundleId);

signals:
    void appInstalled(const QString& bundleId);
    void appUninstalled(const QString& bundleId);
    void installProgress(int percentage);
    void errorOccurred(const QString& message);

private:
    DeviceManager *deviceManager_;
    
    AppInfo parseAppInfo(plist_t appDict);
    static void installStatusCallback(const char *operation, plist_t status, void *user_data);
};

// AppManager.cpp
#include "appmanager.h"
#include <QDebug>
#include <QFileInfo>

AppManager::AppManager(DeviceManager *deviceManager, QObject *parent)
    : QObject(parent), deviceManager_(deviceManager) {
}

QList<AppManager::AppInfo> AppManager::getInstalledApps() {
    QList<AppInfo> apps;
    
    idevice_t device = deviceManager_->getCurrentDevice();
    if (!device) {
        emit errorOccurred("设备未连接");
        return apps;
    }
    
    instproxy_client_t instproxy = nullptr;
    instproxy_error_t error = instproxy_client_start_service(device, &instproxy, "phone-linkc");
    
    if (error != INSTPROXY_E_SUCCESS) {
        emit errorOccurred(QString("启动应用代理服务失败: %1").arg(error));
        return apps;
    }
    
    // 设置浏览选项
    plist_t options = plist_new_dict();
    plist_t app_types = plist_new_array();
    
    // 添加用户应用和系统应用
    plist_array_append_item(app_types, plist_new_string("User"));
    plist_array_append_item(app_types, plist_new_string("System"));
    
    plist_dict_set_item(options, "ApplicationType", app_types);
    
    // 获取应用列表
    plist_t result = nullptr;
    error = instproxy_browse(instproxy, options, &result);
    
    if (error == INSTPROXY_E_SUCCESS && result) {
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
    } else {
        emit errorOccurred(QString("获取应用列表失败: %1").arg(error));
    }
    
    plist_free(options);
    instproxy_client_free(instproxy);
    
    qDebug() << "获取到" << apps.size() << "个应用";
    return apps;
}

AppManager::AppInfo AppManager::parseAppInfo(plist_t appDict) {
    AppInfo info;
    
    // 获取Bundle ID
    plist_t bundle_id_node = plist_dict_get_item(appDict, "CFBundleIdentifier");
    if (bundle_id_node) {
        char *bundle_id = nullptr;
        plist_get_string_val(bundle_id_node, &bundle_id);
        info.bundleId = QString::fromUtf8(bundle_id);
        free(bundle_id);
    }
    
    // 获取显示名称
    plist_t display_name_node = plist_dict_get_item(appDict, "CFBundleDisplayName");
    if (display_name_node) {
        char *display_name = nullptr;
        plist_get_string_val(display_name_node, &display_name);
        info.displayName = QString::fromUtf8(display_name);
        free(display_name);
    }
    
    // 如果没有显示名称，使用Bundle名称
    if (info.displayName.isEmpty()) {
        plist_t name_node = plist_dict_get_item(appDict, "CFBundleName");
        if (name_node) {
            char *name = nullptr;
            plist_get_string_val(name_node, &name);
            info.displayName = QString::fromUtf8(name);
            free(name);
        }
    }
    
    // 获取版本信息
    plist_t version_node = plist_dict_get_item(appDict, "CFBundleShortVersionString");
    if (version_node) {
        char *version = nullptr;
        plist_get_string_val(version_node, &version);
        info.version = QString::fromUtf8(version);
        free(version);
    }
    
    plist_t bundle_version_node = plist_dict_get_item(appDict, "CFBundleVersion");
    if (bundle_version_node) {
        char *bundle_version = nullptr;
        plist_get_string_val(bundle_version_node, &bundle_version);
        info.bundleVersion = QString::fromUtf8(bundle_version);
        free(bundle_version);
    }
    
    // 检查是否为系统应用
    plist_t app_type_node = plist_dict_get_item(appDict, "ApplicationType");
    if (app_type_node) {
        char *app_type = nullptr;
        plist_get_string_val(app_type_node, &app_type);
        info.isSystemApp = (QString::fromUtf8(app_type) == "System");
        free(app_type);
    }
    
    return info;
}

bool AppManager::installApp(const QString& ipaPath) {
    idevice_t device = deviceManager_->getCurrentDevice();
    if (!device) {
        emit errorOccurred("设备未连接");
        return false;
    }
    
    QFileInfo fileInfo(ipaPath);
    if (!fileInfo.exists() || fileInfo.suffix().toLower() != "ipa") {
        emit errorOccurred("IPA文件不存在或格式不正确");
        return false;
    }
    
    instproxy_client_t instproxy = nullptr;
    instproxy_error_t error = instproxy_client_start_service(device, &instproxy, "phone-linkc");
    
    if (error != INSTPROXY_E_SUCCESS) {
        emit errorOccurred(QString("启动应用代理服务失败: %1").arg(error));
        return false;
    }
    
    // 设置安装选项
    plist_t options = plist_new_dict();
    plist_dict_set_item(options, "PackageType", plist_new_string("Developer"));
    
    // 开始安装
    QByteArray ipaPathBytes = ipaPath.toUtf8();
    error = instproxy_install(instproxy, ipaPathBytes.constData(), options, 
                             installStatusCallback, this);
    
    bool success = (error == INSTPROXY_E_SUCCESS);
    if (!success) {
        emit errorOccurred(QString("应用安装失败: %1").arg(error));
    }
    
    plist_free(options);
    instproxy_client_free(instproxy);
    
    return success;
}

void AppManager::installStatusCallback(const char *operation, plist_t status, void *user_data) {
    AppManager *manager = static_cast<AppManager*>(user_data);
    
    if (!status) return;
    
    // 获取进度信息
    plist_t progress_node = plist_dict_get_item(status, "PercentComplete");
    if (progress_node) {
        uint64_t progress = 0;
        plist_get_uint_val(progress_node, &progress);
        emit manager->installProgress(static_cast<int>(progress));
    }
    
    // 检查状态
    plist_t status_node = plist_dict_get_item(status, "Status");
    if (status_node) {
        char *status_str = nullptr;
        plist_get_string_val(status_node, &status_str);
        QString status_string = QString::fromUtf8(status_str);
        
        if (status_string == "Complete") {
            qDebug() << "应用安装完成";
        } else if (status_string.contains("Error")) {
            plist_t error_node = plist_dict_get_item(status, "ErrorDescription");
            if (error_node) {
                char *error_str = nullptr;
                plist_get_string_val(error_node, &error_str);
                emit manager->errorOccurred(QString("安装错误: %1").arg(QString::fromUtf8(error_str)));
                free(error_str);
            }
        }
        
        free(status_str);
    }
}
```

### 4. 文件传输功能

#### 4.1 基础文件操作

```cpp
// FileManager.h
class FileManager : public QObject {
    Q_OBJECT
    
public:
    explicit FileManager(DeviceManager *deviceManager, QObject *parent = nullptr);
    
    QStringList listDirectory(const QString& path);
    bool uploadFile(const QString& localPath, const QString& remotePath);
    bool downloadFile(const QString& remotePath, const QString& localPath);
    bool createDirectory(const QString& path);
    bool deleteFile(const QString& path);
    qint64 getFileSize(const QString& path);
    
    // 沙盒访问
    bool connectToAppSandbox(const QString& bundleId);
    QStringList listAppDocuments(const QString& bundleId);

signals:
    void transferProgress(qint64 transferred, qint64 total);
    void errorOccurred(const QString& message);

private:
    DeviceManager *deviceManager_;
    afc_client_t afc_;
    
    bool initializeAFC();
    void cleanupAFC();
};

// FileManager.cpp
#include "filemanager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>

FileManager::FileManager(DeviceManager *deviceManager, QObject *parent)
    : QObject(parent), deviceManager_(deviceManager), afc_(nullptr) {
}

bool FileManager::initializeAFC() {
    idevice_t device = deviceManager_->getCurrentDevice();
    if (!device) {
        emit errorOccurred("设备未连接");
        return false;
    }
    
    afc_error_t error = afc_client_start_service(device, &afc_, "phone-linkc");
    if (error != AFC_E_SUCCESS) {
        emit errorOccurred(QString("启动AFC服务失败: %1").arg(error));
        return false;
    }
    
    return true;
}

void FileManager::cleanupAFC() {
    if (afc_) {
        afc_client_free(afc_);
        afc_ = nullptr;
    }
}

QStringList FileManager::listDirectory(const QString& path) {
    QStringList files;
    
    if (!initializeAFC()) {
        return files;
    }
    
    char **list = nullptr;
    QByteArray pathBytes = path.toUtf8();
    afc_error_t error = afc_read_directory(afc_, pathBytes.constData(), &list);
    
    if (error == AFC_E_SUCCESS && list) {
        int i = 0;
        while (list[i]) {
            QString filename = QString::fromUtf8(list[i]);
            if (filename != "." && filename != "..") {
                files << filename;
            }
            i++;
        }
        afc_dictionary_free(list);
    } else {
        emit errorOccurred(QString("读取目录失败: %1").arg(error));
    }
    
    cleanupAFC();
    return files;
}

bool FileManager::uploadFile(const QString& localPath, const QString& remotePath) {
    QFile localFile(localPath);
    if (!localFile.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("无法打开本地文件: %1").arg(localPath));
        return false;
    }
    
    if (!initializeAFC()) {
        return false;
    }
    
    uint64_t handle = 0;
    QByteArray remotePathBytes = remotePath.toUtf8();
    afc_error_t error = afc_file_open(afc_, remotePathBytes.constData(), 
                                     AFC_FOPEN_WRONLY, &handle);
    
    if (error != AFC_E_SUCCESS) {
        emit errorOccurred(QString("无法打开远程文件: %1").arg(error));
        cleanupAFC();
        return false;
    }
    
    // 分块传输文件
    const int BUFFER_SIZE = 8192;
    QByteArray buffer;
    qint64 totalSize = localFile.size();
    qint64 transferred = 0;
    
    while (!localFile.atEnd()) {
        buffer = localFile.read(BUFFER_SIZE);
        uint32_t bytes_written = 0;
        
        error = afc_file_write(afc_, handle, buffer.constData(), 
                              buffer.size(), &bytes_written);
        
        if (error != AFC_E_SUCCESS) {
            emit errorOccurred(QString("写入文件失败: %1").arg(error));
            afc_file_close(afc_, handle);
            cleanupAFC();
            return false;
        }
        
        transferred += bytes_written;
        emit transferProgress(transferred, totalSize);
    }
    
    afc_file_close(afc_, handle);
    cleanupAFC();
    
    qDebug() << "文件上传完成:" << remotePath;
    return true;
}

bool FileManager::downloadFile(const QString& remotePath, const QString& localPath) {
    if (!initializeAFC()) {
        return false;
    }
    
    uint64_t handle = 0;
    QByteArray remotePathBytes = remotePath.toUtf8();
    afc_error_t error = afc_file_open(afc_, remotePathBytes.constData(), 
                                     AFC_FOPEN_RDONLY, &handle);
    
    if (error != AFC_E_SUCCESS) {
        emit errorOccurred(QString("无法打开远程文件: %1").arg(error));
        cleanupAFC();
        return false;
    }
    
    QFile localFile(localPath);
    if (!localFile.open(QIODevice::WriteOnly)) {
        emit errorOccurred(QString("无法创建本地文件: %1").arg(localPath));
        afc_file_close(afc_, handle);
        cleanupAFC();
        return false;
    }
    
    // 获取文件大小
    qint64 fileSize = getFileSize(remotePath);
    
    // 分块下载文件
    const int BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE];
    qint64 transferred = 0;
    
    while (true) {
        uint32_t bytes_read = 0;
        error = afc_file_read(afc_, handle, buffer, BUFFER_SIZE, &bytes_read);
        
        if (error != AFC_E_SUCCESS || bytes_read == 0) {
            break;
        }
        
        localFile.write(buffer, bytes_read);
        transferred += bytes_read;
        
        if (fileSize > 0) {
            emit transferProgress(transferred, fileSize);
        }
    }
    
    localFile.close();
    afc_file_close(afc_, handle);
    cleanupAFC();
    
    qDebug() << "文件下载完成:" << localPath;
    return true;
}
```

### 5. 系统日志监控

#### 5.1 日志监控实现

```cpp
// LogMonitor.h
class LogMonitor : public QObject {
    Q_OBJECT
    
public:
    explicit LogMonitor(DeviceManager *deviceManager, QObject *parent = nullptr);
    ~LogMonitor();
    
    bool startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const { return monitoring_; }
    
    // 日志过滤
    void setLogLevel(const QString& level);
    void setProcessFilter(const QString& process);
    void clearFilters();

signals:
    void logReceived(const QString& timestamp, const QString& process, 
                    const QString& level, const QString& message);
    void errorOccurred(const QString& message);

private slots:
    void processLogData();

private:
    DeviceManager *deviceManager_;
    QThread *logThread_;
    bool monitoring_;
    QString logLevel_;
    QString processFilter_;
    
    class LogWorker;
    LogWorker *logWorker_;
};

// 日志监控工作线程
class LogMonitor::LogWorker : public QThread {
    Q_OBJECT
    
public:
    LogWorker(idevice_t device, QObject *parent = nullptr)
        : QThread(parent), device_(device), running_(false) {}
    
    void stopMonitoring() { running_ = false; }

protected:
    void run() override {
        running_ = true;
        
        syslog_relay_client_t syslog = nullptr;
        syslog_relay_error_t error = syslog_relay_client_start_service(device_, &syslog, "phone-linkc");
        
        if (error != SYSLOG_RELAY_E_SUCCESS) {
            emit errorOccurred(QString("启动系统日志服务失败: %1").arg(error));
            return;
        }
        
        while (running_) {
            char *data = nullptr;
            uint32_t size = 0;
            
            error = syslog_relay_receive_with_timeout(syslog, &data, &size, 1000);
            
            if (error == SYSLOG_RELAY_E_SUCCESS && data && size > 0) {
                QString logData = QString::fromUtf8(data, size);
                emit logDataReceived(logData);
                free(data);
            }
        }
        
        syslog_relay_client_free(syslog);
    }

signals:
    void logDataReceived(const QString& data);
    void errorOccurred(const QString& message);

private:
    idevice_t device_;
    bool running_;
};

// LogMonitor.cpp
#include "logmonitor.h"
#include <QRegularExpression>
#include <QDateTime>

LogMonitor::LogMonitor(DeviceManager *deviceManager, QObject *parent)
    : QObject(parent)
    , deviceManager_(deviceManager)
    , logThread_(nullptr)
    , monitoring_(false)
    , logWorker_(nullptr) {
}

LogMonitor::~LogMonitor() {
    stopMonitoring();
}

bool LogMonitor::startMonitoring() {
    if (monitoring_) return true;
    
    idevice_t device = deviceManager_->getCurrentDevice();
    if (!device) {
        emit errorOccurred("设备未连接");
        return false;
    }
    
    logWorker_ = new LogWorker(device, this);
    connect(logWorker_, &LogWorker::logDataReceived, this, &LogMonitor::processLogData);
    connect(logWorker_, &LogWorker::errorOccurred, this, &LogMonitor::errorOccurred);
    connect(logWorker_, &QThread::finished, logWorker_, &QObject::deleteLater);
    
    logWorker_->start();
    monitoring_ = true;
    
    qDebug() << "开始监控系统日志";
    return true;
}

void LogMonitor::stopMonitoring() {
    if (!monitoring_) return;
    
    if (logWorker_) {
        logWorker_->stopMonitoring();
        logWorker_->wait(3000);
        logWorker_ = nullptr;
    }
    
    monitoring_ = false;
    qDebug() << "停止监控系统日志";
}

void LogMonitor::processLogData() {
    // 解析日志数据的简化实现
    // 实际实现需要根据具体的日志格式进行解析
    QRegularExpression logPattern(R"((\w+\s+\d+\s+\d+:\d+:\d+)\s+\w+\s+(\w+)\[(\d+)\]\s+<(\w+)>:\s+(.+))");
    
    // 这里应该处理从logWorker_接收到的数据
    // 由于信号槽的限制，这里只是示例代码
}

void LogMonitor::setLogLevel(const QString& level) {
    logLevel_ = level.toLower();
}

void LogMonitor::setProcessFilter(const QString& process) {
    processFilter_ = process;
}

void LogMonitor::clearFilters() {
    logLevel_.clear();
    processFilter_.clear();
}
```

### 6. 完整使用示例

#### 6.1 主窗口集成

```cpp
// MainWindow.h
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onDeviceConnected(const QString& udid);
    void onDeviceDisconnected(const QString& udid);
    void onScreenshotReady(const QImage& image);
    void onTakeScreenshot();
    void onStartMirroring();
    void onStopMirroring();
    void onRefreshApps();
    void onInstallApp();
    void onUninstallApp();
    void onBrowseFiles();
    void onStartLogging();
    void onStopLogging();

private:
    void setupUI();
    void updateDeviceInfo();
    void updateAppsList();
    
    DeviceManager *deviceManager_;
    AppManager *appManager_;
    FileManager *fileManager_;
    LogMonitor *logMonitor_;
    
    // UI控件
    QLabel *deviceInfoLabel_;
    QLabel *screenshotLabel_;
    QPushButton *screenshotButton_;
    QPushButton *mirrorButton_;
    QListWidget *appsListWidget_;
    QTextEdit *logTextEdit_;
    QProgressBar *progressBar_;
};

// MainWindow.cpp
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , deviceManager_(new DeviceManager(this))
    , appManager_(new AppManager(deviceManager_, this))
    , fileManager_(new FileManager(deviceManager_, this))
    , logMonitor_(new LogMonitor(deviceManager_, this))
{
    setupUI();
    
    // 连接信号槽
    connect(deviceManager_, &DeviceManager::deviceConnected, 
            this, &MainWindow::onDeviceConnected);
    connect(deviceManager_, &DeviceManager::deviceDisconnected, 
            this, &MainWindow::onDeviceDisconnected);
    connect(deviceManager_, &DeviceManager::screenshotReady, 
            this, &MainWindow::onScreenshotReady);
    
    // 检查库可用性
    if (!deviceManager_->isLibraryAvailable()) {
        QMessageBox::warning(this, "警告", "libimobiledevice库不可用，请检查安装");
    }
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // 左侧面板
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    // 设备信息组
    QGroupBox *deviceGroup = new QGroupBox("设备信息");
    QVBoxLayout *deviceLayout = new QVBoxLayout(deviceGroup);
    deviceInfoLabel_ = new QLabel("未连接设备");
    deviceLayout->addWidget(deviceInfoLabel_);
    leftLayout->addWidget(deviceGroup);
    
    // 屏幕控制组
    QGroupBox *screenGroup = new QGroupBox("屏幕控制");
    QVBoxLayout *screenLayout = new QVBoxLayout(screenGroup);
    
    screenshotButton_ = new QPushButton("截图");
    mirrorButton_ = new QPushButton("开始镜像");
    screenshotLabel_ = new QLabel();
    screenshotLabel_->setMinimumSize(200, 300);
    screenshotLabel_->setScaledContents(true);
    screenshotLabel_->setStyleSheet("border: 1px solid gray;");
    
    connect(screenshotButton_, &QPushButton::clicked, this, &MainWindow::onTakeScreenshot);
    connect(mirrorButton_, &QPushButton::clicked, this, &MainWindow::onStartMirroring);
    
    screenLayout->addWidget(screenshotButton_);
    screenLayout->addWidget(mirrorButton_);
    screenLayout->addWidget(screenshotLabel_);
    leftLayout->addWidget(screenGroup);
    
    // 应用管理组
    QGroupBox *appGroup = new QGroupBox("应用管理");
    QVBoxLayout *appLayout = new QVBoxLayout(appGroup);
    
    QPushButton *refreshAppsButton = new QPushButton("刷新应用列表");
    QPushButton *installAppButton = new QPushButton("安装应用");
    QPushButton *uninstallAppButton = new QPushButton("卸载应用");
    appsListWidget_ = new QListWidget();
    
    connect(refreshAppsButton, &QPushButton::clicked, this, &MainWindow::onRefreshApps);
    connect(installAppButton, &QPushButton::clicked, this, &MainWindow::onInstallApp);
    connect(uninstallAppButton, &QPushButton::clicked, this, &MainWindow::onUninstallApp);
    
    appLayout->addWidget(refreshAppsButton);
    appLayout->addWidget(installAppButton);
    appLayout->addWidget(uninstallAppButton);
    appLayout->addWidget(appsListWidget_);
    leftLayout->addWidget(appGroup);
    
    // 右侧日志面板
    QVBoxLayout *rightLayout = new QVBoxLayout();
    
    QGroupBox *logGroup = new QGroupBox("系统日志");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    
    QPushButton *startLogButton = new QPushButton("开始日志监控");
    QPushButton *stopLogButton = new QPushButton("停止日志监控");
    QPushButton *clearLogButton = new QPushButton("清除日志");
    logTextEdit_ = new QTextEdit();
    logTextEdit_->setFont(QFont("Consolas", 9));
    
    connect(startLogButton, &QPushButton::clicked, this, &MainWindow::onStartLogging);
    connect(stopLogButton, &QPushButton::clicked, this, &MainWindow::onStopLogging);
    connect(clearLogButton, &QPushButton::clicked, logTextEdit_, &QTextEdit::clear);
    
    QHBoxLayout *logButtonLayout = new QHBoxLayout();
    logButtonLayout->addWidget(startLogButton);
    logButtonLayout->addWidget(stopLogButton);
    logButtonLayout->addWidget(clearLogButton);
    
    logLayout->addLayout(logButtonLayout);
    logLayout->addWidget(logTextEdit_);
    rightLayout->addWidget(logGroup);
    
    // 添加到主布局
    QWidget *leftWidget = new QWidget();
    leftWidget->setLayout(leftLayout);
    leftWidget->setMinimumWidth(350);
    
    QWidget *rightWidget = new QWidget();
    rightWidget->setLayout(rightLayout);
    
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    
    // 状态栏
    progressBar_ = new QProgressBar();
    progressBar_->setVisible(false);
    statusBar()->addPermanentWidget(progressBar_);
    
    resize(1000, 700);
    setWindowTitle("Phone Link Cross - iOS设备管理工具");
}

void MainWindow::onDeviceConnected(const QString& udid) {
    qDebug() << "设备已连接:" << udid;
    updateDeviceInfo();
    statusBar()->showMessage("设备连接成功", 3000);
    
    // 启用相关控件
    screenshotButton_->setEnabled(true);
    mirrorButton_->setEnabled(true);
}

void MainWindow::onDeviceDisconnected(const QString& udid) {
    qDebug() << "设备已断开:" << udid;
    deviceInfoLabel_->setText("未连接设备");
    screenshotLabel_->clear();
    appsListWidget_->clear();
    statusBar()->showMessage("设备已断开", 3000);
    
    // 禁用相关控件
    screenshotButton_->setEnabled(false);
    mirrorButton_->setEnabled(false);
}

void MainWindow::onScreenshotReady(const QImage& image) {
    if (!image.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(image);
        screenshotLabel_->setPixmap(pixmap.scaled(screenshotLabel_->size(), 
                                                 Qt::KeepAspectRatio, 
                                                 Qt::SmoothTransformation));
    }
}

void MainWindow::updateDeviceInfo() {
    QString deviceName = deviceManager_->getDeviceName();
    QString deviceModel = deviceManager_->getDeviceModel();
    QString iosVersion = deviceManager_->getIOSVersion();
    
    QString info = QString("名称: %1\n型号: %2\niOS版本: %3")
                   .arg(deviceName, deviceModel, iosVersion);
    
    deviceInfoLabel_->setText(info);
}

void MainWindow::onTakeScreenshot() {
    deviceManager_->takeScreenshot();
}

void MainWindow::onStartMirroring() {
    if (mirrorButton_->text() == "开始镜像") {
        deviceManager_->startScreenMirroring();
        mirrorButton_->setText("停止镜像");
    } else {
        deviceManager_->stopScreenMirroring();
        mirrorButton_->setText("开始镜像");
    }
}

void MainWindow::onRefreshApps() {
    appsListWidget_->clear();
    
    QList<AppManager::AppInfo> apps = appManager_->getInstalledApps();
    for (const auto& app : apps) {
        QString itemText = QString("%1 (%2) - %3")
                          .arg(app.displayName, app.bundleId, app.version);
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, app.bundleId);
        appsListWidget_->addItem(item);
    }
    
    statusBar()->showMessage(QString("已加载 %1 个应用").arg(apps.size()), 3000);
}

void MainWindow::onInstallApp() {
    QString ipaPath = QFileDialog::getOpenFileName(this, 
                                                  "选择IPA文件", 
                                                  QString(), 
                                                  "IPA文件 (*.ipa)");
    
    if (!ipaPath.isEmpty()) {
        progressBar_->setVisible(true);
        progressBar_->setValue(0);
        
        connect(appManager_, &AppManager::installProgress, 
                progressBar_, &QProgressBar::setValue);
        
        bool success = appManager_->installApp(ipaPath);
        
        progressBar_->setVisible(false);
        
        if (success) {
            QMessageBox::information(this, "成功", "应用安装成功");
            onRefreshApps(); // 刷新应用列表
        }
    }
}

void MainWindow::onStartLogging() {
    if (logMonitor_->startMonitoring()) {
        statusBar()->showMessage("已开始日志监控", 3000);
        
        connect(logMonitor_, &LogMonitor::logReceived, 
                [this](const QString& timestamp, const QString& process, 
                       const QString& level, const QString& message) {
            QString logEntry = QString("[%1] %2[%3]: %4")
                              .arg(timestamp, process, level, message);
            logTextEdit_->append(logEntry);
        });
    }
}

void MainWindow::onStopLogging() {
    logMonitor_->stopMonitoring();
    statusBar()->showMessage("已停止日志监控", 3000);
}
```

### 7. 部署和发布

#### 7.1 Windows部署配置

```cmake
# 部署脚本片段
if(WIN32)
    # 安装程序
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
    
    # 安装libimobiledevice库文件
    install(DIRECTORY "${LIBIMOBILEDEVICE_DIR}/"
            DESTINATION bin
            FILES_MATCHING 
            PATTERN "*.dll"
            PATTERN "*.exe")
    
    # 安装Qt运行时
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # 使用windeployqt部署Qt依赖
        find_program(QT_DEPLOY_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
        if(QT_DEPLOY_EXECUTABLE)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${QT_DEPLOY_EXECUTABLE} $<TARGET_FILE:${PROJECT_NAME}>
                COMMENT "Qt Deploy Tool")
        endif()
    endif()
endif()
```

#### 7.2 错误处理和日志记录

```cpp
// Logger.h - 统一日志记录
class Logger : public QObject {
    Q_OBJECT
    
public:
    enum LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3
    };
    
    static Logger* instance();
    
    void setLogLevel(LogLevel level);
    void setLogFile(const QString& filepath);
    
    static void debug(const QString& message);
    static void info(const QString& message);
    static void warning(const QString& message);
    static void error(const QString& message);

signals:
    void logMessage(LogLevel level, const QString& message);

private:
    explicit Logger(QObject *parent = nullptr);
    void writeLog(LogLevel level, const QString& message);
    
    static Logger* instance_;
    LogLevel currentLevel_;
    QFile* logFile_;
    QTextStream* logStream_;
};

// 使用宏简化日志调用
#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_INFO(msg) Logger::info(msg)
#define LOG_WARNING(msg) Logger::warning(msg)
#define LOG_ERROR(msg) Logger::error(msg)
```

### 8. 性能优化建议

#### 8.1 连接池管理
```cpp
// DeviceConnectionPool.h
class DeviceConnectionPool : public QObject {
    Q_OBJECT
    
public:
    static DeviceConnectionPool* instance();
    
    idevice_t getConnection(const QString& udid);
    void releaseConnection(const QString& udid);
    void cleanupExpiredConnections();

private:
    struct ConnectionInfo {
        idevice_t device;
        QDateTime lastUsed;
        int refCount;
    };
    
    QMap<QString, ConnectionInfo> connections_;
    QTimer* cleanupTimer_;
    QMutex mutex_;
};
```

#### 8.2 异步操作封装
```cpp
// AsyncOperations.h
template<typename T>
class DeviceOperation : public QObject {
    Q_OBJECT
    
public:
    using OperationFunction = std::function<T(idevice_t)>;
    
    DeviceOperation(const QString& udid, OperationFunction func, QObject* parent = nullptr)
        : QObject(parent), udid_(udid), operation_(func) {}
    
    QFuture<T> execute() {
        return QtConcurrent::run([this]() -> T {
            idevice_t device = DeviceConnectionPool::instance()->getConnection(udid_);
            if (!device) {
                throw std::runtime_error("无法获取设备连接");
            }
            
            try {
                T result = operation_(device);
                DeviceConnectionPool::instance()->releaseConnection(udid_);
                return result;
            } catch (...) {
                DeviceConnectionPool::instance()->releaseConnection(udid_);
                throw;
            }
        });
    }

private:
    QString udid_;
    OperationFunction operation_;
};
```

### 9. 故障排除指南

#### 9.1 常见问题及解决方案

**问题1: 设备无法识别**
```cpp
bool DeviceManager::diagnoseConnection() {
    // 1. 检查设备是否在设备列表中
    QStringList devices = getConnectedDevices();
    if (devices.isEmpty()) {
        LOG_ERROR("未找到任何iOS设备，请检查：\n"
                 "1. USB连接是否正常\n"
                 "2. 设备是否已信任此电脑\n"
                 "3. iTunes是否已安装");
        return false;
    }
    
    // 2. 尝试连接第一个设备
    QString udid = devices.first();
    if (!connectToDevice(udid)) {
        LOG_ERROR("设备连接失败，可能的原因：\n"
                 "1. 设备锁定状态\n"
                 "2. 证书配对问题\n"
                 "3. libimobiledevice版本兼容性");
        return false;
    }
    
    // 3. 测试基本功能
    QString deviceName = getDeviceName();
    if (deviceName.isEmpty()) {
        LOG_WARNING("无法获取设备名称，连接可能不稳定");
    }
    
    LOG_INFO(QString("设备诊断完成 - 设备: %1, 名称: %2").arg(udid, deviceName));
    return true;
}
```

**问题2: 服务启动失败**
```cpp
bool DeviceManager::checkServiceAvailability() {
    const QStringList services = {
        "com.apple.mobile.screenshotr",
        "com.apple.mobile.installation_proxy",
        "com.apple.afc",
        "com.apple.syslog_relay"
    };
    
    for (const QString& service : services) {
        if (!isServiceAvailable(service)) {
            LOG_WARNING(QString("服务不可用: %1").arg(service));
            
            // 提供解决建议
            if (service.contains("screenshotr")) {
                LOG_INFO("截图服务需要iOS 4.0+，且设备需要处于解锁状态");
            } else if (service.contains("installation_proxy")) {
                LOG_INFO("应用安装服务可能需要开发者证书或企业证书");
            }
        }
    }
    
    return true;
}
```

---

## 总结

本功能使用指南提供了完整的libimobiledevice集成方案，包括：

### 核心功能模块
- **设备管理**: 自动发现、连接管理、状态监控
- **设备信息**: 完整的设备属性获取
- **屏幕功能**: 截图、实时镜像
- **应用管理**: 安装、卸载、列表管理
- **文件传输**: 上传、下载、目录浏览
- **日志监控**: 实时系统日志查看

### 最佳实践
- RAII资源管理模式
- 异步操作避免界面卡顿
- 连接池优化性能
- 统一错误处理和日志记录
- 线程安全的API封装

### 部署支持
- CMake构建系统集成
- Windows自动部署配置
- 依赖库管理
- 故障诊断工具

通过本指南，开发者可以快速在phone-link-cross项目中集成libimobiledevice功能，实现完整的iOS设备管理和交互能力。

**相关文档**:
- [API接口明细说明](API-REFERENCE.md)
- [基础说明文档](README.md)
- [项目构建指南](../../../README.md)
