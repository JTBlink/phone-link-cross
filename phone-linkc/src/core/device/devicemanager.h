#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // 设备管理
    void startDiscovery();
    void stopDiscovery();
    void refreshDevices();
    bool connectToDevice(const QString &udid);
    void disconnectFromDevice();
    
    // 获取信息
    QStringList getConnectedDevices() const;
    QString getCurrentDevice() const { return m_currentUdid; }
    bool isConnected() const { return m_isConnected; }
    QString getDeviceName(const QString &udid);

signals:
    void deviceFound(const QString &udid, const QString &name);
    void deviceLost(const QString &udid);
    void deviceConnected(const QString &udid);
    void deviceDisconnected();
    void errorOccurred(const QString &error);
    void noDevicesFound();  // 扫描完成但没有发现任何设备

private:
    QStringList m_knownDevices;
    QString m_currentUdid;
    bool m_isConnected;
    
    // libimobiledevice相关成员（动态加载模式）
    idevice_t m_device;
    lockdownd_client_t m_lockdown;
    bool m_eventSubscribed;
    
    // 私有方法
    bool initializeConnection(const QString &udid);
    void cleanup();
    void scanCurrentDevices();
    
    // 事件订阅相关
    void startEventSubscription();
    void stopEventSubscription();
    static void deviceEventCallback(const void* event, void* user_data);
};

#endif // DEVICEMANAGER_H