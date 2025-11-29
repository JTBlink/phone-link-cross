#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QVariantMap>

#ifdef HAVE_LIBIMOBILEDEVICE
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#endif

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // 设备管理
    void startDiscovery();
    void stopDiscovery();
    bool connectToDevice(const QString &udid);
    void disconnectFromDevice();
    
    // 获取信息
    QStringList getConnectedDevices() const;
    QString getCurrentDevice() const { return m_currentUdid; }
    bool isConnected() const { return m_isConnected; }

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
    QString m_currentUdid;
    bool m_isConnected;

#ifdef HAVE_LIBIMOBILEDEVICE
    idevice_t m_device;
    lockdownd_client_t m_lockdown;
    
    bool initializeConnection(const QString &udid);
    QString getDeviceName(const QString &udid);
    void cleanup();
#endif

    // 模拟设备（用于没有 libimobiledevice 的情况）
    void simulateDeviceDiscovery();
};

#endif // DEVICEMANAGER_H
