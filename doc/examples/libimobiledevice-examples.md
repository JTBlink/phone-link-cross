# libimobiledevice 实践示例

> 🔧 libimobiledevice + Qt 集成的完整代码示例与最佳实践

## 目录

1. [环境准备](#环境准备)
2. [基础连接示例](#基础连接示例)
3. [设备信息获取](#设备信息获取)
4. [应用管理示例](#应用管理示例)
5. [文件传输示例](#文件传输示例)
6. [Qt 集成示例](#qt-集成示例)
7. [完整项目示例](#完整项目示例)

## 环境准备

### 1. 依赖安装

#### macOS (使用 Homebrew)
```bash
# 安装 libimobiledevice 及其依赖
brew install libimobiledevice
brew install libplist
brew install libusbmuxd

# 验证安装
idevice_id -l  # 列出连接的设备
```

#### Ubuntu/Debian
```bash
# 安装依赖包
sudo apt-get update
sudo apt-get install \
    libimobiledevice6 \
    libimobiledevice-dev \
    libplist-dev \
    libusbmuxd-dev \
    usbmuxd

# 启动 usbmuxd 服务
sudo systemctl start usbmuxd
sudo systemctl enable usbmuxd
```

#### Windows
```powershell
# 使用 vcpkg 安装
vcpkg install libimobiledevice:x64-windows
vcpkg install libplist:x64-windows

# 或下载预编译包
# https://github.com/libimobiledevice-win32/imobiledevice-net/releases
```

### 2. CMake 项目配置

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.19)
project(iOSDeviceManager LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# 查找 libimobiledevice
find_package(PkgConfig REQUIRED)
pkg_check_modules(IMOBILEDEVICE REQUIRED libimobiledevice-1.0)
pkg_check_modules(PLIST REQUIRED libplist-2.0)
pkg_check_modules(USBMUXD REQUIRED libusbmuxd-2.0)

# Qt 标准项目设置
qt_standard_project_setup()

# 添加可执行文件
qt_add_executable(iOSDeviceManager
    main.cpp
    devicemanager.cpp
    devicemanager.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# 链接库
target_link_libraries(iOSDeviceManager
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ${IMOBILEDEVICE_LIBRARIES}
    ${PLIST_LIBRARIES}
    ${USBMUXD_LIBRARIES}
)

# 包含目录
target_include_directories(iOSDeviceManager
    PRIVATE
    ${IMOBILEDEVICE_INCLUDE_DIRS}
    ${PLIST_INCLUDE_DIRS}
    ${USBMUXD_LIBRARIES}
)
```

## 基础连接示例

### 1. 设备发现和连接

```cpp
// deviceconnector.h
#ifndef DEVICECONNECTOR_H
#define DEVICECONNECTOR_H

#include <QObject>
#include <QTimer>
#include <QStringList>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

class DeviceConnector : public QObject
{
    Q_OBJECT

public:
    explicit DeviceConnector(QObject *parent = nullptr);
    ~DeviceConnector();

    void startDiscovery();
    void stopDiscovery();
    bool connectToDevice(const QString &udid);
    void disconnectFromDevice();

signals:
    void deviceFound(const QString &udid, const QString &name);
    void deviceLost(const QString &udid);
    void deviceConnected(const QString &udid);
    void deviceDisconnected();
    void errorOccurred(const QString &error);

private slots:
    void checkDevices();

private:
    QTimer *m_discoveryTimer;
    QStringList m_knownDevices;
    idevice_t m_device;
    lockdownd_client_t m_lockdown;
    QString m_currentUdid;
    bool m_isConnected;

    bool initializeConnection(const QString &udid);
    QString getDeviceName(const QString &udid);
    void cleanup();
};

#endif // DEVICECONNECTOR_H
```

```cpp
// deviceconnector.cpp
#include "deviceconnector.h"
#include <QDebug>
#include <libimobiledevice/libimobiledevice.h>

DeviceConnector::DeviceConnector(QObject *parent)
    : QObject(parent)
    , m_discoveryTimer(new QTimer(this))
    , m_device(nullptr)
    , m_lockdown(nullptr)
    , m_isConnected(false)
{
    // 设置发现定时器
    m_discoveryTimer->setInterval(2000); // 2秒检查一次
    connect(m_discoveryTimer, &QTimer::timeout, this, &DeviceConnector::checkDevices);
}

DeviceConnector::~DeviceConnector()
{
    cleanup();
}

void DeviceConnector::startDiscovery()
{
    qDebug() << "开始设备发现...";
    m_discoveryTimer->start();
    checkDevices(); // 立即检查一次
}

void DeviceConnector::stopDiscovery()
{
    qDebug() << "停止设备发现";
    m_discoveryTimer->stop();
}

void DeviceConnector::checkDevices()
{
    char **device_list = nullptr;
    int device_count = 0;
    
    // 获取设备列表
    if (idevice_get_device_list(&device_list, &device_count) != IDEVICE_E_SUCCESS) {
        qWarning() << "无法获取设备列表";
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
            
            // 如果当前连接的设备断开了
            if (knownUdid == m_currentUdid) {
                disconnectFromDevice();
            }
        }
    }
    
    m_knownDevices = currentDevices;
    
    // 清理设备列表
    idevice_device_list_free(device_list);
}

bool DeviceConnector::connectToDevice(const QString &udid)
{
    if (m_isConnected) {
        disconnectFromDevice();
    }
    
    qDebug() << "尝试连接到设备:" << udid;
    
    if (initializeConnection(udid)) {
        m_currentUdid = udid;
        m_isConnected = true;
        emit deviceConnected(udid);
        qDebug() << "成功连接到设备:" << udid;
        return true;
    } else {
        emit errorOccurred(QString("无法连接到设备: %1").arg(udid));
        return false;
    }
}

void DeviceConnector::disconnectFromDevice()
{
    if (m_isConnected) {
        cleanup();
        m_isConnected = false;
        emit deviceDisconnected();
        qDebug() << "已断开设备连接";
    }
}

bool DeviceConnector::initializeConnection(const QString &udid)
{
    // 创建设备连接
    if (idevice_new(&m_device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
        qWarning() << "创建设备连接失败:" << udid;
        return false;
    }
    
    // 创建 lockdown 客户端
    if (lockdownd_client_new_with_handshake(m_device, &m_lockdown, "QtDeviceManager") != LOCKDOWN_E_SUCCESS) {
        qWarning() << "创建 lockdown 客户端失败:" << udid;
        idevice_free(m_device);
        m_device = nullptr;
        return false;
    }
    
    return true;
}

QString DeviceConnector::getDeviceName(const QString &udid)
{
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    plist_t value = nullptr;
    QString name = "Unknown Device";
    
    // 创建临时连接获取设备名称
    if (idevice_new(&device, udid.toUtf8().constData()) == IDEVICE_E_SUCCESS) {
        if (lockdownd_client_new_with_handshake(device, &lockdown, "QtDeviceManager") == LOCKDOWN_E_SUCCESS) {
            if (lockdownd_get_value(lockdown, nullptr, "DeviceName", &value) == LOCKDOWN_E_SUCCESS) {
                if (value && plist_get_node_type(value) == PLIST_STRING) {
                    char *str_value = nullptr;
                    plist_get_string_val(value, &str_value);
                    if (str_value) {
                        name = QString::fromUtf8(str_value);
                        free(str_value);
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

void DeviceConnector::cleanup()
{
    if (m_lockdown) {
        lockdownd_client_free(m_lockdown);
        m_lockdown = nullptr;
    }
    
    if (m_device) {
        idevice_free(m_device);
        m_device = nullptr;
    }
}
```

## 设备信息获取

### 1. 设备信息管理器

```cpp
// deviceinfo.h
#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QObject>
#include <QVariantMap>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

struct DeviceInfo {
    QString udid;
    QString name;
    QString model;
    QString productVersion;
    QString buildVersion;
    QString serialNumber;
    QString deviceClass;
    QString productType;
    QString marketingName;
    qint64 totalCapacity;
    qint64 availableCapacity;
    QString wifiAddress;
    QString bluetoothAddress;
    bool passcodeSet;
    QString activationState;
    
    QVariantMap toVariantMap() const;
};

class DeviceInfoManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceInfoManager(QObject *parent = nullptr);
    
    DeviceInfo getDeviceInfo(idevice_t device, lockdownd_client_t lockdown);
    QVariantMap getDetailedInfo(idevice_t device, lockdownd_client_t lockdown);
    
private:
    QString getStringValue(lockdownd_client_t lockdown, const char* domain, const char* key);
    qint64 getIntValue(lockdownd_client_t lockdown, const char* domain, const char* key);
    bool getBoolValue(lockdownd_client_t lockdown, const char* domain, const char* key);
};

#endif // DEVICEINFO_H
```

```cpp
// deviceinfo.cpp
#include "deviceinfo.h"
#include <QDebug>
#include <libplist/plist.h>

QVariantMap DeviceInfo::toVariantMap() const
{
    QVariantMap map;
    map["udid"] = udid;
    map["name"] = name;
    map["model"] = model;
    map["productVersion"] = productVersion;
    map["buildVersion"] = buildVersion;
    map["serialNumber"] = serialNumber;
    map["deviceClass"] = deviceClass;
    map["productType"] = productType;
    map["marketingName"] = marketingName;
    map["totalCapacity"] = totalCapacity;
    map["availableCapacity"] = availableCapacity;
    map["wifiAddress"] = wifiAddress;
    map["bluetoothAddress"] = bluetoothAddress;
    map["passcodeSet"] = passcodeSet;
    map["activationState"] = activationState;
    return map;
}

DeviceInfoManager::DeviceInfoManager(QObject *parent)
    : QObject(parent)
{
}

DeviceInfo DeviceInfoManager::getDeviceInfo(idevice_t device, lockdownd_client_t lockdown)
{
    DeviceInfo info;
    
    // 获取 UDID
    char* udid = nullptr;
    if (idevice_get_udid(device, &udid) == IDEVICE_E_SUCCESS && udid) {
        info.udid = QString::fromUtf8(udid);
        free(udid);
    }
    
    // 基本信息
    info.name = getStringValue(lockdown, nullptr, "DeviceName");
    info.model = getStringValue(lockdown, nullptr, "ModelNumber");
    info.productVersion = getStringValue(lockdown, nullptr, "ProductVersion");
    info.buildVersion = getStringValue(lockdown, nullptr, "BuildVersion");
    info.serialNumber = getStringValue(lockdown, nullptr, "SerialNumber");
    info.deviceClass = getStringValue(lockdown, nullptr, "DeviceClass");
    info.productType = getStringValue(lockdown, nullptr, "ProductType");
    info.marketingName = getStringValue(lockdown, nullptr, "MarketingName");
    
    // 存储信息
    info.totalCapacity = getIntValue(lockdown, "com.apple.disk_usage", "TotalDiskCapacity");
    info.availableCapacity = getIntValue(lockdown, "com.apple.disk_usage", "TotalDataAvailable");
    
    // 网络信息
    info.wifiAddress = getStringValue(lockdown, nullptr, "WiFiAddress");
    info.bluetoothAddress = getStringValue(lockdown, nullptr, "BluetoothAddress");
    
    // 安全信息
    info.passcodeSet = getBoolValue(lockdown, nullptr, "PasswordProtected");
    info.activationState = getStringValue(lockdown, nullptr, "ActivationState");
    
    return info;
}

QString DeviceInfoManager::getStringValue(lockdownd_client_t lockdown, const char* domain, const char* key)
{
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

qint64 DeviceInfoManager::getIntValue(lockdownd_client_t lockdown, const char* domain, const char* key)
{
    plist_t value = nullptr;
    qint64 result = 0;
    
    if (lockdownd_get_value(lockdown, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (plist_get_node_type(value) == PLIST_UINT) {
            uint64_t uint_value = 0;
            plist_get_uint_val(value, &uint_value);
            result = static_cast<qint64>(uint_value);
        }
        plist_free(value);
    }
    
    return result;
}

bool DeviceInfoManager::getBoolValue(lockdownd_client_t lockdown, const char* domain, const char* key)
{
    plist_t value = nullptr;
    bool result = false;
    
    if (lockdownd_get_value(lockdown, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (plist_get_node_type(value) == PLIST_BOOLEAN) {
            uint8_t bool_value = 0;
            plist_get_bool_val(value, &bool_value);
            result = bool_value != 0;
        }
        plist_free(value);
    }
    
    return result;
}

QVariantMap DeviceInfoManager::getDetailedInfo(idevice_t device, lockdownd_client_t lockdown)
{
    QVariantMap detailedInfo;
    
    // 获取所有可用的域信息
    const char* domains[] = {
        nullptr,                           // 基本信息
        "com.apple.disk_usage",           // 存储信息
        "com.apple.mobile.battery",       // 电池信息
        "com.apple.mobile.wireless_lockdown", // 无线信息
        "com.apple.international",        // 国际化信息
        "com.apple.mobile.iTunes"         // iTunes 信息
    };
    
    for (const char* domain : domains) {
        QVariantMap domainInfo;
        plist_t domain_plist = nullptr;
        
        if (lockdownd_get_value(lockdown, domain, nullptr, &domain_plist) == LOCKDOWN_E_SUCCESS && domain_plist) {
            // 递归解析 plist 数据
            parsePlistToVariantMap(domain_plist, domainInfo);
            plist_free(domain_plist);
        }
        
        QString domainName = domain ? QString::fromUtf8(domain) : "basic";
        detailedInfo[domainName] = domainInfo;
    }
    
    return detailedInfo;
}

void DeviceInfoManager::parsePlistToVariantMap(plist_t node, QVariantMap &map)
{
    if (!node) return;
    
    plist_type type = plist_get_node_type(node);
    
    switch (type) {
    case PLIST_DICT: {
        plist_dict_iter iter = nullptr;
        plist_dict_new_iter(node, &iter);
        
        char *key = nullptr;
        plist_t subnode = nullptr;
        
        do {
            plist_dict_next_item(node, iter, &key, &subnode);
            if (key && subnode) {
                QVariant value = plistToVariant(subnode);
                map[QString::fromUtf8(key)] = value;
                free(key);
                key = nullptr;
            }
        } while (subnode);
        
        if (iter) free(iter);
        break;
    }
    default:
        // 非字典类型不处理
        break;
    }
}

QVariant DeviceInfoManager::plistToVariant(plist_t node)
{
    if (!node) return QVariant();
    
    plist_type type = plist_get_node_type(node);
    
    switch (type) {
    case PLIST_STRING: {
        char *str_val = nullptr;
        plist_get_string_val(node, &str_val);
        if (str_val) {
            QVariant result = QString::fromUtf8(str_val);
            free(str_val);
            return result;
        }
        break;
    }
    case PLIST_UINT: {
        uint64_t uint_val = 0;
        plist_get_uint_val(node, &uint_val);
        return QVariant::fromValue(static_cast<qint64>(uint_val));
    }
    case PLIST_BOOLEAN: {
        uint8_t bool_val = 0;
        plist_get_bool_val(node, &bool_val);
        return QVariant(bool_val != 0);
    }
    case PLIST_REAL: {
        double real_val = 0.0;
        plist_get_real_val(node, &real_val);
        return QVariant(real_val);
    }
    case PLIST_DICT: {
        QVariantMap dict_map;
        parsePlistToVariantMap(node, dict_map);
        return dict_map;
    }
    case PLIST_ARRAY: {
        QVariantList array_list;
        uint32_t array_size = plist_array_get_size(node);
        for (uint32_t i = 0; i < array_size; i++) {
            plist_t array_item = plist_array_get_item(node, i);
            array_list << plistToVariant(array_item);
        }
        return array_list;
    }
    case PLIST_DATA: {
        char *data_ptr = nullptr;
        uint64_t data_len = 0;
        plist_get_data_val(node, &data_ptr, &data_len);
        if (data_ptr && data_len > 0) {
            QByteArray data(data_ptr, static_cast<int>(data_len));
            free(data_ptr);
            return data;
        }
        break;
    }
    default:
        break;
    }
    
    return QVariant();
}
```

## 应用管理示例

### 1. 应用安装管理器

```cpp
// appmanager.h
#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/installation_proxy.h>

struct AppInfo {
    QString bundleId;
    QString name;
    QString version;
    QString shortVersion;
    QString executableName;
    qint64 staticDiskUsage;
    qint64 dynamicDiskUsage;
    QByteArray iconData;
    QString installDate;
    QString applicationType;
    
    QVariantMap toVariantMap() const;
};

class AppInstallWorker : public QObject
{
    Q_OBJECT

public slots:
    void installApp(const QString &ipaPath, const QString &udid);
    void uninstallApp(const QString &bundleId, const QString &udid);

signals:
    void installProgress(int percentage);
    void installFinished(bool success, const QString &message);
    void uninstallFinished(bool success, const QString &message);

private:
    static void install_callback(plist_t command, plist_t status, void* user_data);
};

class AppManager : public QObject
{
    Q_OBJECT

public:
    explicit AppManager(QObject *parent = nullptr);
    ~AppManager();
    
    QList<AppInfo> getInstalledApps(idevice_t device, lockdownd_client_t lockdown);
    void installApp(const QString &ipaPath, const QString &udid);
    void uninstallApp(const QString &bundleId, const QString &udid);
    
signals:
    void installProgress(int percentage);
    void installFinished(bool success, const QString &message);
    void uninstallFinished(bool success, const QString &message);

private:
    QThread *m_workerThread;
    AppInstallWorker *m_worker;
    
    AppInfo parseAppInfo(plist_t app_plist);
    QByteArray extractIconData(plist_t app_plist);
};

#endif // APPMANAGER_H
```

```cpp
// appmanager.cpp
#include "appmanager.h"
#include <QDebug>
#include <QFileInfo>
#include <libplist/plist.h>

QVariantMap AppInfo::toVariantMap() const
{
    QVariantMap map;
    map["bundleId"] = bundleId;
    map["name"] = name;
    map["version"] = version;
    map["shortVersion"] = shortVersion;
    map["executableName"] = executableName;
    map["staticDiskUsage"] = staticDiskUsage;
    map["dynamicDiskUsage"] = dynamicDiskUsage;
    map["installDate"] = installDate;
    map["applicationType"] = applicationType;
    return map;
}

AppManager::AppManager(QObject *parent)
    : QObject(parent)
    , m_workerThread(new QThread(this))
    , m_worker(new AppInstallWorker)
{
    m_worker->moveToThread(m_workerThread);
    
    // 连接信号
    connect(m_worker, &AppInstallWorker::installProgress, 
            this, &AppManager::installProgress);
    connect(m_worker, &AppInstallWorker::installFinished, 
            this, &AppManager::installFinished);
    connect(m_worker, &AppInstallWorker::uninstallFinished, 
            this, &AppManager::uninstallFinished);
    
    m_workerThread->start();
}

AppManager::~AppManager()
{
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_worker;
}

QList<AppInfo> AppManager::getInstalledApps(idevice_t device, lockdownd_client_t lockdown)
{
    QList<AppInfo> appList;
    instproxy_client_t ipc = nullptr;
    
    // 创建安装代理客户端
    if (instproxy_client_new(device, lockdown, &ipc) != INSTPROXY_E_SUCCESS) {
        qWarning() << "无法创建安装代理客户端";
        return appList;
    }
    
    plist_t apps_plist = nullptr;
    plist_t client_opts = instproxy_client_options_new();
    
    // 设置选项，获取用户应用
    instproxy_client_options_add(client_opts, "ApplicationType", "User", nullptr);
    
    // 浏览已安装的应用
    if (instproxy_browse(ipc, client_opts, &apps_plist) == INSTPROXY_E_SUCCESS && apps_plist) {
        uint32_t app_count = plist_array_get_size(apps_plist);
        
        for (uint32_t i = 0; i < app_count; i++) {
            plist_t app_plist = plist_array_get_item(apps_plist, i);
            if (app_plist) {
                AppInfo appInfo = parseAppInfo(app_plist);
                if (!appInfo.bundleId.isEmpty()) {
                    appList << appInfo;
                }
            }
        }
        
        plist_free(apps_plist);
    }
    
    if (client_opts) plist_free(client_opts);
    instproxy_client_free(ipc);
    
    return appList;
}

void AppManager::installApp(const QString &ipaPath, const QString &udid)
{
    QMetaObject::invokeMethod(m_worker, "installApp", 
                             Q_ARG(QString, ipaPath), Q_ARG(QString, udid));
}

void AppManager::uninstallApp(const QString &bundleId, const QString &udid)
{
    QMetaObject::invokeMethod(m_worker, "uninstallApp", 
                             Q_ARG(QString, bundleId), Q_ARG(QString, udid));
}

AppInfo AppManager::parseAppInfo(plist_t app_plist)
{
    AppInfo info;
    
    auto getString = [app_plist](const char* key) -> QString {
        plist_t value = plist_dict_get_item(app_plist, key);
        if (value && plist_get_node_type(value) == PLIST_STRING) {
            char *str_val = nullptr;
            plist_get_string_val(value, &str_val);
            if (str_val) {
                QString result = QString::fromUtf8(str_val);
                free(str_val);
                return result;
            }
        }
        return QString();
    };
    
    auto getInt = [app_plist](const char* key) -> qint64 {
        plist_t value = plist_dict_get_item(app_plist, key);
        if (value && plist_get_node_type(value) == PLIST_UINT) {
            uint64_t uint_val = 0;
            plist_get_uint_val(value, &uint_val);
            return static_cast<qint64>(uint_val);
        }
        return 0;
    };
    
    info.bundleId = getString("CFBundleIdentifier");
    info.name = getString("CFBundleDisplayName");
    if (info.name.isEmpty()) {
        info.name = getString("CFBundleName");
    }
    info.version = getString("CFBundleVersion");
    info.shortVersion = getString("CFBundleShortVersionString");
    info.executableName = getString("CFBundleExecutable");
    info.staticDiskUsage = getInt("StaticDiskUsage");
    info.dynamicDiskUsage = getInt("DynamicDiskUsage");
    info.applicationType = getString("ApplicationType");
    
    // 提取图标数据
    info.iconData = extractIconData(app_plist);
    
    return info;
}

QByteArray AppManager::extractIconData(plist_t app_plist)
{
    plist_t icons_dict = plist_dict_get_item(app_plist, "CFBundleIcons");
    if (!icons_dict) return QByteArray();
    
    plist_t primary_icon = plist_dict_get_item(icons_dict, "CFBundlePrimaryIcon");
    if (!primary_icon) return QByteArray();
    
    plist_t icon_files = plist_dict_get_item(primary_icon, "CFBundleIconFiles");
    if (!icon_files || plist_get_node_type(icon_files) != PLIST_ARRAY) return QByteArray();
    
    // 获取第一个图标文件名
    plist_t first_icon = plist_array_get_item(icon_files, 0);
    if (!first_icon) return QByteArray();
    
    char *icon_name = nullptr;
    plist_get_string_val(first_icon, &icon_name);
    if (!icon_name) return QByteArray();
    
    // 这里需要通过 AFC 服务获取图标数据
    // 实际实现需要连接到应用沙箱并读取图标文件
    
    free(icon_name);
    return QByteArray();
}

void AppInstallWorker::installApp(const QString &ipaPath, const QString &udid)
{
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    instproxy_client_t ipc = nullptr;
    
    do {
        // 连接设备
        if (idevice_new(&device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
            emit installFinished(false, "无法连接到设备");
            break;
        }
        
        if (lockdownd_client_new_with_handshake(device, &lockdown, "AppInstaller") != LOCKDOWN_E_SUCCESS) {
            emit installFinished(false, "无法建立 lockdown 连接");
            break;
        }
        
        if (instproxy_client_new(device, lockdown, &ipc) != INSTPROXY_E_SUCCESS) {
            emit installFinished(false, "无法创建安装代理客户端");
            break;
        }
        
        // 检查 IPA 文件
        QFileInfo ipaFile(ipaPath);
        if (!ipaFile.exists()) {
            emit installFinished(false, "IPA 文件不存在");
            break;
        }
        
        // 开始安装
        plist_t client_opts = instproxy_client_options_new();
        instproxy_client_options_add(client_opts, "PackageType", "Developer", nullptr);
        
        instproxy_error_t err = instproxy_install(ipc, ipaPath.toUtf8().constData(), 
                                                 client_opts, install_callback, this);
        
        plist_free(client_opts);
        
        if (err == INSTPROXY_E_SUCCESS) {
            emit installFinished(true, "应用安装成功");
        } else {
            emit installFinished(false, QString("安装失败，错误代码: %1").arg(err));
        }
        
    } while (false);
    
    // 清理资源
    if (ipc) instproxy_client_free(ipc);
    if (lockdown) lockdownd_client_free(lockdown);
    if (device) idevice_free(device);
}

void AppInstallWorker::uninstallApp(const QString &bundleId, const QString &udid)
{
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    instproxy_client_t ipc = nullptr;
    
    do {
        // 连接设备
        if (idevice_new(&device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
            emit uninstallFinished(false, "无法连接到设备");
            break;
        }
        
        if (lockdownd_client_new_with_handshake(device, &lockdown, "AppUninstaller") != LOCKDOWN_E_SUCCESS) {
            emit uninstallFinished(false, "无法建立 lockdown 连接");
            break;
        }
        
        if (instproxy_client_new(device, lockdown, &ipc) != INSTPROXY_E_SUCCESS) {
            emit uninstallFinished(false, "无法创建安装代理客户端");
            break;
        }
        
        // 开始卸载
        instproxy_error_t err = instproxy_uninstall(ipc, bundleId.toUtf8().constData(), 
                                                   nullptr, install_callback, this);
        
        if (err == INSTPROXY_E_SUCCESS) {
            emit uninstallFinished(true, "应用卸载成功");
        } else {
            emit uninstallFinished(false, QString("卸载失败，错误代码: %1").arg(err));
        }
        
    } while (false);
    
    // 清理资源
    if (ipc) instproxy_client_free(ipc);
    if (lockdown) lockdownd_client_free(lockdown);
    if (device) idevice_free(device);
}

void AppInstallWorker::install_callback(plist_t command, plist_t status, void* user_data)
{
    AppInstallWorker *worker = static_cast<AppInstallWorker*>(user_data);
    
    if (!status) return;
    
    plist_t percent_complete = plist_dict_get_item(status, "PercentComplete");
    if (percent_complete && plist_get_node_type(percent_complete) == PLIST_UINT) {
        uint64_t percent = 0;
        plist_get_uint_val(percent_complete, &percent);
        worker->installProgress(static_cast<int>(percent));
    }
}
```

## 文件传输示例

### 1. AFC 文件传输管理器

```cpp
// filemanager.h
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/afc.h>

struct FileInfo {
    QString name;
    QString path;
    qint64 size;
    bool isDirectory;
    QString mtime;
    QString birthtime;
    
    QVariantMap toVariantMap() const;
};

class FileTransferWorker : public QObject
{
    Q_OBJECT

public slots:
    void uploadFile(const QString &localPath, const QString &remotePath, const QString &udid);
    void downloadFile(const QString &remotePath, const QString &localPath, const QString &udid);
    void listDirectory(const QString &remotePath, const QString &udid);

signals:
    void transferProgress(qint64 transferred, qint64 total);
    void transferFinished(bool success, const QString &message);
    void directoryListed(const QList<FileInfo> &files);

private:
    afc_client_t connectToAFC(const QString &udid);
    void cleanup(idevice_t device, lockdownd_client_t lockdown, afc_client_t afc);
};

class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QObject *parent = nullptr);
    ~FileManager();
    
    void uploadFile(const QString &localPath, const QString &remotePath, const QString &udid);
    void downloadFile(const QString &remotePath, const QString &localPath, const QString &udid);
    void listDirectory(const QString &remotePath, const QString &udid);
    
signals:
    void transferProgress(qint64 transferred, qint64 total);
    void transferFinished(bool success, const QString &message);
    void directoryListed(const QList<FileInfo> &files);

private:
    QThread *m_workerThread;
    FileTransferWorker *m_worker;
};

#endif // FILEMANAGER_H
```

## Qt 集成示例

### 1. 主窗口集成

```cpp
// mainwindow.h (更新版本)
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTreeWidget>
#include <QProgressBar>
#include <QLabel>
#include "deviceconnector.h"
#include "deviceinfo.h"
#include "appmanager.h"
#include "filemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onDeviceFound(const QString &udid, const QString &name);
    void onDeviceLost(const QString &udid);
    void onDeviceConnected(const QString &udid);
    void onDeviceDisconnected();
    void onDeviceListItemClicked(QListWidgetItem *item);
    
    void onInstallProgress(int percentage);
    void onInstallFinished(bool success, const QString &message);
    void onTransferProgress(qint64 transferred, qint64 total);
    void onDirectoryListed(const QList<FileInfo> &files);
    
    void installAppClicked();
    void uninstallAppClicked();
    void refreshAppsClicked();
    void uploadFileClicked();
    void downloadFileClicked();

private:
    Ui::MainWindow *ui;
    
    DeviceConnector *m_deviceConnector;
    DeviceInfoManager *m_deviceInfoManager;
    AppManager *m_appManager;
    FileManager *m_fileManager;
    
    QString m_currentDeviceUdid;
    
    void setupUI();
    void updateDeviceInfo(const QString &udid);
    void updateAppList(const QString &udid);
    void updateFileList(const QString &udid);
};

#endif // MAINWINDOW_H
```

### 2. 使用示例

```cpp
// main.cpp 使用示例
#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"
#include "deviceconnector.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("iOS Device Manager");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Your Company");
    
    // 检查 libimobiledevice 是否可用
    char **device_list = nullptr;
    int device_count = 0;
    
    if (idevice_get_device_list(&device_list, &device_count) != IDEVICE_E_SUCCESS) {
        QMessageBox::critical(nullptr, "错误", 
                             "libimobiledevice 初始化失败！\n"
                             "请确保已正确安装 libimobiledevice。");
        return -1;
    }
    
    if (device_list) {
        idevice_device_list_free(device_list);
    }
    
    // 创建主窗口
    MainWindow window;
    window.show();
    
    return app.exec();
}
```

## 完整项目示例

### 1. CMakeLists.txt 完整配置

```cmake
cmake_minimum_required(VERSION 3.19)
project(phone-linkc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt6
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network LinguistTools)

# 查找 libimobiledevice
find_package(PkgConfig REQUIRED)
pkg_check_modules(IMOBILEDEVICE REQUIRED libimobiledevice-1.0)
pkg_check_modules(PLIST REQUIRED libplist-2.0)
pkg_check_modules(USBMUXD REQUIRED libusbmuxd-2.0)

qt_standard_project_setup()

qt_add_executable(phone-linkc
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    deviceconnector.cpp
    deviceconnector.h
    deviceinfo.cpp
    deviceinfo.h
    appmanager.cpp
    appmanager.h
    filemanager.cpp
    filemanager.h
)

qt_add_translations(
    TARGETS phone-linkc
    TS_FILES phone-linkc_zh_CN.ts
)

target_link_libraries(phone-linkc
    PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Network
    ${IMOBILEDEVICE_LIBRARIES}
    ${PLIST_LIBRARIES}
    ${USBMUXD_LIBRARIES}
)

target_include_directories(phone-linkc
    PRIVATE
    ${IMOBILEDEVICE_INCLUDE_DIRS}
    ${PLIST_INCLUDE_DIRS}
    ${USBMUXD_INCLUDE_DIRS}
)

include(GNUInstallDirs)

install(TARGETS phone-linkc
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET phone-linkc
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})
```

### 2. 构建和运行

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 构建项目
cmake --build . --config Release

# 运行应用
./phone-linkc  # Linux/macOS
# 或 phone-linkc.exe (Windows)
```

### 3. 部署说明

#### macOS 部署
```bash
# 使用 macdeployqt 打包
macdeployqt phone-linkc.app

# 创建 DMG 安装包
hdiutil create -volname "phone-linkc" -srcfolder phone-linkc.app -ov -format UDZO phone-linkc.dmg
```

#### Windows 部署
```cmd
# 使用 windeployqt 打包
windeployqt.exe phone-linkc.exe

# 复制 libimobiledevice DLL 文件
copy "%VCPKG_ROOT%\installed\x64-windows\bin\*.dll" .
```

#### Linux 部署
```bash
# 使用 linuxdeployqt 打包
linuxdeployqt phone-linkc -appimage

# 或创建 deb 包
cpack -G DEB
```

## 最佳实践与注意事项

###
