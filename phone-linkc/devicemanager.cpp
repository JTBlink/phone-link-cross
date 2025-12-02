#include "devicemanager.h"
#include <QDebug>
#include <QRandomGenerator>

#ifdef HAVE_LIBIMOBILEDEVICE
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <plist/plist.h>
#endif

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_isConnected(false)
#ifdef HAVE_LIBIMOBILEDEVICE
    , m_device(nullptr)
    , m_lockdown(nullptr)
    , m_eventContext(nullptr)
#endif
{
    qDebug() << "DeviceManager 已创建";
#ifdef HAVE_LIBIMOBILEDEVICE
    qDebug() << "libimobiledevice 支持已启用 - 事件驱动模式";
#else
    qDebug() << "libimobiledevice 不可用，无法连接 iOS 设备";
#endif
}

DeviceManager::~DeviceManager()
{
#ifdef HAVE_LIBIMOBILEDEVICE
    cleanup();
#endif
    qDebug() << "DeviceManager 已销毁";
}

void DeviceManager::startDiscovery()
{
    qDebug() << "开始设备发现...";
#ifdef HAVE_LIBIMOBILEDEVICE
    // 先扫描一次现有设备
    scanCurrentDevices();
    
    // 如果有设备连接，才启用事件订阅
    if (!m_knownDevices.isEmpty()) {
        startEventSubscription();
        qDebug() << "发现" << m_knownDevices.size() << "台设备，启用事件监听";
    } else {
        qDebug() << "未发现任何设备，暂停设备发现。连接设备或点击刷新重新开始";
    }
#else
    qDebug() << "libimobiledevice 不可用，无法连接 iOS 设备";
#endif
}

void DeviceManager::stopDiscovery()
{
    qDebug() << "停止设备发现";
#ifdef HAVE_LIBIMOBILEDEVICE
    stopEventSubscription();
#endif
}

void DeviceManager::refreshDevices()
{
    qDebug() << "手动刷新设备列表";
#ifdef HAVE_LIBIMOBILEDEVICE
    scanCurrentDevices();
    
    // 如果刷新后发现了设备，且还没有订阅事件，则启动事件订阅
    if (!m_knownDevices.isEmpty() && !m_eventContext) {
        startEventSubscription();
        qDebug() << "发现" << m_knownDevices.size() << "台设备，启用事件监听";
    } else if (m_knownDevices.isEmpty() && m_eventContext) {
        // 如果没有设备了，停止事件订阅
        stopEventSubscription();
        qDebug() << "未发现任何设备，暂停设备发现。连接设备后再次点击刷新";
    } else if (m_knownDevices.isEmpty() && !m_eventContext) {
        // 如果依然没有设备且未监听
        qDebug() << "未发现任何设备，暂停设备发现。连接设备后再次点击刷新";
    }
#else
    qDebug() << "libimobiledevice 不可用，无法刷新 iOS 设备";
#endif
}

QStringList DeviceManager::getConnectedDevices() const
{
    return m_knownDevices;
}

#ifdef HAVE_LIBIMOBILEDEVICE
void DeviceManager::scanCurrentDevices()
{
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
#endif

bool DeviceManager::connectToDevice(const QString &udid)
{
    if (m_isConnected) {
        disconnectFromDevice();
    }
    
    qDebug() << "尝试连接到设备:" << udid;
    
#ifdef HAVE_LIBIMOBILEDEVICE
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
#else
    // 没有 libimobiledevice 支持，无法连接设备
    emit errorOccurred("libimobiledevice 未安装或不可用。无法连接到 iOS 设备。");
    return false;
#endif
}

void DeviceManager::disconnectFromDevice()
{
    if (m_isConnected) {
#ifdef HAVE_LIBIMOBILEDEVICE
        cleanup();
#endif
        m_isConnected = false;
        QString udid = m_currentUdid;
        m_currentUdid.clear();
        emit deviceDisconnected();
        qDebug() << "已断开设备连接:" << udid;
    }
}


#ifdef HAVE_LIBIMOBILEDEVICE
bool DeviceManager::initializeConnection(const QString &udid)
{
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

QString DeviceManager::getDeviceName(const QString &udid)
{
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

void DeviceManager::cleanup()
{
    // 停止事件订阅
    stopEventSubscription();
    
    if (m_lockdown) {
        lockdownd_client_free(m_lockdown);
        m_lockdown = nullptr;
    }
    
    if (m_device) {
        idevice_free(m_device);
        m_device = nullptr;
    }
}

void DeviceManager::startEventSubscription()
{
    if (m_eventContext) {
        return; // 已经订阅了
    }
    
    idevice_subscription_context_t context = nullptr;
    
    // Use a lambda wrapper to match the expected callback signature
    auto callback_wrapper = [](const idevice_event_t* event, void* user_data) -> void {
        deviceEventCallback(event, user_data);
    };
    
    idevice_error_t ret = idevice_events_subscribe(&context, callback_wrapper, this);
    if (ret == IDEVICE_E_SUCCESS) {
        m_eventContext = context;
        qDebug() << "成功订阅 USB 设备事件通知 - 纯事件驱动模式";
    } else {
        qDebug() << "订阅 USB 设备事件失败";
        m_eventContext = nullptr;
    }
}

void DeviceManager::stopEventSubscription()
{
    if (m_eventContext) {
        idevice_subscription_context_t context = static_cast<idevice_subscription_context_t>(m_eventContext);
        idevice_events_unsubscribe(context);
        m_eventContext = nullptr;
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
        
        // 直接处理设备连接事件
        if (!manager->m_knownDevices.contains(udid)) {
            QString deviceName = manager->getDeviceName(udid);
            manager->m_knownDevices << udid;
            emit manager->deviceFound(udid, deviceName);
        }
        
    } else if (device_event->event == IDEVICE_DEVICE_REMOVE) {
        qDebug() << "USB 事件：设备断开" << udid;
        
        // 从已知设备列表中移除
        if (manager->m_knownDevices.contains(udid)) {
            manager->m_knownDevices.removeAll(udid);
            emit manager->deviceLost(udid);
            
            // 如果是当前连接的设备断开了
            if (udid == manager->m_currentUdid) {
                manager->disconnectFromDevice();
            }
        }
        
    } else if (device_event->event == IDEVICE_DEVICE_PAIRED) {
        qDebug() << "USB 事件：设备配对" << udid;
        // 设备配对后直接处理
        if (!manager->m_knownDevices.contains(udid)) {
            QString deviceName = manager->getDeviceName(udid);
            manager->m_knownDevices << udid;
            emit manager->deviceFound(udid, deviceName);
        }
    }
}
#endif
