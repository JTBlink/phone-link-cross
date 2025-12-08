#include "devicemanager.h"
#include "platform/libimobiledevice_dynamic.h"
#include <QDebug>
#include <QRandomGenerator>
#include <QMetaObject>
#include <QCoreApplication>

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_isConnected(false)
    , m_device(nullptr)
    , m_lockdown(nullptr)
    , m_eventSubscribed(false)
{
    qDebug() << "DeviceManager 已创建";
    
    // 初始化动态库加载器
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (loader.initialize()) {
        qDebug() << "libimobiledevice 动态库已加载 - 事件驱动模式";
    } else {
        qDebug() << "libimobiledevice 动态库加载失败，无法连接 iOS 设备";
    }
}

DeviceManager::~DeviceManager()
{
    cleanup();
    qDebug() << "DeviceManager 已销毁";
}

void DeviceManager::startDiscovery()
{
    qDebug() << "开始设备发现...";
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        qDebug() << "libimobiledevice 不可用，无法连接 iOS 设备";
        return;
    }
    
    // 先扫描一次现有设备
    scanCurrentDevices();
    
    // 启用事件订阅（不管有没有设备都订阅，以便监听新设备连接）
    startEventSubscription();
    if (!m_knownDevices.isEmpty()) {
        qDebug() << "发现" << m_knownDevices.size() << "台设备，已启用事件监听";
    } else {
        qDebug() << "未发现任何设备，已启用事件监听等待设备连接";
    }
}

void DeviceManager::stopDiscovery()
{
    qDebug() << "停止设备发现";
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (loader.isInitialized()) {
        stopEventSubscription();
    }
}

void DeviceManager::refreshDevices()
{
    qDebug() << "手动刷新设备列表";
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        qDebug() << "libimobiledevice 不可用，无法刷新 iOS 设备";
        return;
    }
    
    scanCurrentDevices();
    
    // 如果刷新后发现了设备，且还没有订阅事件，则启动事件订阅
    if (!m_eventSubscribed) {
        startEventSubscription();
    }
    if (!m_knownDevices.isEmpty()) {
        qDebug() << "发现" << m_knownDevices.size() << "台设备";
    } else {
        qDebug() << "未发现任何设备，事件监听已启用，等待设备连接";
    }
}

QStringList DeviceManager::getConnectedDevices() const
{
    return m_knownDevices;
}

void DeviceManager::scanCurrentDevices()
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized() || !loader.idevice_get_device_list || !loader.idevice_device_list_free) {
        qDebug() << "获取设备列表失败 - 动态库未正确加载";
        emit noDevicesFound();
        return;
    }
    
    char **device_list = nullptr;
    int device_count = 0;
    
    // 获取设备列表
    if (loader.idevice_get_device_list(&device_list, &device_count) != IDEVICE_E_SUCCESS) {
        qDebug() << "获取设备列表失败";
        emit noDevicesFound();
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
    loader.idevice_device_list_free(device_list);
    
    // 如果没有发现任何设备，发送信号通知 UI
    if (m_knownDevices.isEmpty()) {
        emit noDevicesFound();
    }
}

bool DeviceManager::connectToDevice(const QString &udid)
{
    if (m_isConnected) {
        disconnectFromDevice();
    }
    
    qDebug() << "尝试连接到设备:" << udid;
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        emit errorOccurred("libimobiledevice 未安装或不可用。无法连接到 iOS 设备。");
        return false;
    }
    
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

void DeviceManager::disconnectFromDevice()
{
    if (m_isConnected) {
        cleanup();
        m_isConnected = false;
        QString udid = m_currentUdid;
        m_currentUdid.clear();
        emit deviceDisconnected();
        qDebug() << "已断开设备连接:" << udid;
    }
}


bool DeviceManager::initializeConnection(const QString &udid)
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized() || !loader.idevice_new || !loader.lockdownd_client_new_with_handshake) {
        qWarning() << "动态库未正确加载，无法初始化连接";
        return false;
    }
    
    // 创建设备连接
    if (loader.idevice_new(&m_device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
        qWarning() << "创建设备连接失败:" << udid;
        return false;
    }
    
    // 创建 lockdown 客户端
    if (loader.lockdownd_client_new_with_handshake(m_device, &m_lockdown, "phone-linkc") != LOCKDOWN_E_SUCCESS) {
        qWarning() << "创建 lockdown 客户端失败:" << udid;
        loader.idevice_free(m_device);
        m_device = nullptr;
        return false;
    }
    
    return true;
}

QString DeviceManager::getDeviceName(const QString &udid)
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        return "Unknown Device";
    }
    
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    plist_t value = nullptr;
    QString name = "Unknown Device";
    
    // 创建临时连接获取设备名称
    if (loader.idevice_new && loader.idevice_new(&device, udid.toUtf8().constData()) == IDEVICE_E_SUCCESS) {
        if (loader.lockdownd_client_new_with_handshake && 
            loader.lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") == LOCKDOWN_E_SUCCESS) {
            if (loader.lockdownd_get_value && 
                loader.lockdownd_get_value(lockdown, nullptr, "DeviceName", &value) == LOCKDOWN_E_SUCCESS) {
                if (value && loader.plist_get_node_type && loader.plist_get_node_type(value) == PLIST_STRING) {
                    char *str_value = nullptr;
                    if (loader.plist_get_string_val) {
                        loader.plist_get_string_val(value, &str_value);
                        if (str_value) {
                            name = QString::fromUtf8(str_value);
                            // 不要调用 free()，因为 plist_get_string_val 分配的内存
                            // 会在 plist_free() 时一起释放
                        }
                    }
                }
                if (value && loader.plist_free) {
                    loader.plist_free(value);
                }
            }
            if (loader.lockdownd_client_free) {
                loader.lockdownd_client_free(lockdown);
            }
        }
        if (loader.idevice_free) {
            loader.idevice_free(device);
        }
    }
    
    return name;
}

void DeviceManager::cleanup()
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    
    // 停止事件订阅
    stopEventSubscription();
    
    if (m_lockdown && loader.lockdownd_client_free) {
        loader.lockdownd_client_free(m_lockdown);
        m_lockdown = nullptr;
    }
    
    if (m_device && loader.idevice_free) {
        loader.idevice_free(m_device);
        m_device = nullptr;
    }
}

void DeviceManager::startEventSubscription()
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized() || !loader.idevice_event_subscribe) {
        qDebug() << "动态库未正确加载，无法启动事件订阅";
        return;
    }
    
    if (m_eventSubscribed) {
        return; // 已经订阅了
    }
    
    // Use a lambda wrapper to match the expected callback signature
    auto callback_wrapper = [](const idevice_event_t* event, void* user_data) -> void {
        deviceEventCallback(event, user_data);
    };
    
    // 使用旧版 API (libimobiledevice 1.3.x)
    idevice_error_t ret = loader.idevice_event_subscribe(callback_wrapper, this);
    if (ret == IDEVICE_E_SUCCESS) {
        m_eventSubscribed = true;
        qDebug() << "成功订阅 USB 设备事件通知 - 纯事件驱动模式";
    } else {
        qDebug() << "订阅 USB 设备事件失败";
        m_eventSubscribed = false;
    }
}

void DeviceManager::stopEventSubscription()
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    
    if (m_eventSubscribed && loader.idevice_event_unsubscribe) {
        // 使用旧版 API (libimobiledevice 1.3.x)
        loader.idevice_event_unsubscribe();
        m_eventSubscribed = false;
        qDebug() << "已停止 USB 设备事件订阅";
    }
}

void DeviceManager::deviceEventCallback(const void* event, void* user_data)
{
    const idevice_event_t* device_event = static_cast<const idevice_event_t*>(event);
    DeviceManager* manager = static_cast<DeviceManager*>(user_data);
    
    if (!manager || !device_event) {
        return;
    }
    
    QString udid = QString::fromUtf8(device_event->udid);
    
    if (device_event->event == IDEVICE_DEVICE_ADD) {
        qDebug() << "USB 事件：设备连接" << udid;
        
        // 使用线程安全的方式在主线程中处理设备连接事件
        QMetaObject::invokeMethod(manager, [manager, udid]() {
            if (!manager->m_knownDevices.contains(udid)) {
                QString deviceName = manager->getDeviceName(udid);
                manager->m_knownDevices << udid;
                qDebug() << "主线程处理：发现新设备" << udid << "名称:" << deviceName;
                emit manager->deviceFound(udid, deviceName);
            }
        }, Qt::QueuedConnection);
        
    } else if (device_event->event == IDEVICE_DEVICE_REMOVE) {
        qDebug() << "USB 事件：设备断开" << udid;
        
        // 使用线程安全的方式在主线程中处理设备断开事件
        QMetaObject::invokeMethod(manager, [manager, udid]() {
            if (manager->m_knownDevices.contains(udid)) {
                manager->m_knownDevices.removeAll(udid);
                qDebug() << "主线程处理：设备断开连接" << udid;
                emit manager->deviceLost(udid);
                
                // 如果是当前连接的设备断开了
                if (udid == manager->m_currentUdid) {
                    manager->disconnectFromDevice();
                }
            }
        }, Qt::QueuedConnection);
        
    } else if (device_event->event == IDEVICE_DEVICE_PAIRED) {
        qDebug() << "USB 事件：设备配对" << udid;
        
        // 使用线程安全的方式在主线程中处理设备配对事件
        QMetaObject::invokeMethod(manager, [manager, udid]() {
            if (!manager->m_knownDevices.contains(udid)) {
                QString deviceName = manager->getDeviceName(udid);
                manager->m_knownDevices << udid;
                qDebug() << "主线程处理：设备配对" << udid << "名称:" << deviceName;
                emit manager->deviceFound(udid, deviceName);
            }
        }, Qt::QueuedConnection);
    }
}