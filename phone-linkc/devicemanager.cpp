#include "devicemanager.h"
#include <QDebug>
#include <QRandomGenerator>

#ifdef HAVE_LIBIMOBILEDEVICE
#include <libplist/plist.h>
#endif

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_discoveryTimer(new QTimer(this))
    , m_isConnected(false)
#ifdef HAVE_LIBIMOBILEDEVICE
    , m_device(nullptr)
    , m_lockdown(nullptr)
#endif
{
    // 设置发现定时器
    m_discoveryTimer->setInterval(2000); // 2秒检查一次
    connect(m_discoveryTimer, &QTimer::timeout, this, &DeviceManager::checkDevices);
    
    qDebug() << "DeviceManager 已创建";
#ifdef HAVE_LIBIMOBILEDEVICE
    qDebug() << "libimobiledevice 支持已启用";
#else
    qDebug() << "libimobiledevice 不可用，使用模拟模式";
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
    m_discoveryTimer->start();
    checkDevices(); // 立即检查一次
}

void DeviceManager::stopDiscovery()
{
    qDebug() << "停止设备发现";
    m_discoveryTimer->stop();
}

QStringList DeviceManager::getConnectedDevices() const
{
    return m_knownDevices;
}

void DeviceManager::checkDevices()
{
#ifdef HAVE_LIBIMOBILEDEVICE
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
#else
    // 模拟设备发现
    simulateDeviceDiscovery();
#endif
}

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
    // 模拟连接
    if (m_knownDevices.contains(udid)) {
        m_currentUdid = udid;
        m_isConnected = true;
        emit deviceConnected(udid);
        qDebug() << "模拟连接到设备:" << udid;
        return true;
    } else {
        emit errorOccurred(QString("模拟模式：设备 %1 不存在").arg(udid));
        return false;
    }
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
    if (m_lockdown) {
        lockdownd_client_free(m_lockdown);
        m_lockdown = nullptr;
    }
    
    if (m_device) {
        idevice_free(m_device);
        m_device = nullptr;
    }
}
#endif

void DeviceManager::simulateDeviceDiscovery()
{
    // 模拟设备发现 - 用于演示目的
    static bool hasSimulatedDevice = false;
    static int discoveryCount = 0;
    
    discoveryCount++;
    
    // 第3次发现时添加模拟设备
    if (discoveryCount == 3 && !hasSimulatedDevice) {
        QString udid = "00000000-0000000000000000";
        QString name = "模拟 iPhone";
        
        m_knownDevices << udid;
        hasSimulatedDevice = true;
        
        qDebug() << "模拟发现设备:" << udid << "名称:" << name;
        emit deviceFound(udid, name);
    }
    
    // 第10次发现时移除模拟设备
    if (discoveryCount == 10 && hasSimulatedDevice) {
        QString udid = "00000000-0000000000000000";
        m_knownDevices.removeAll(udid);
        hasSimulatedDevice = false;
        discoveryCount = 0; // 重置计数器
        
        qDebug() << "模拟设备断开:" << udid;
        emit deviceLost(udid);
        
        if (m_currentUdid == udid) {
            disconnectFromDevice();
        }
    }
}
