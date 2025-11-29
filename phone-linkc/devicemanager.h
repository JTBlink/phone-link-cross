#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QVariantMap>

#ifdef HAVE_LIBIMOBILEDEVICE
// Forward declarations to avoid including headers in header file
typedef struct idevice_private idevice_private;
typedef idevice_private* idevice_t;
typedef struct lockdownd_client_private lockdownd_client_private;
typedef lockdownd_client_private* lockdownd_client_t;
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

};

#endif // DEVICEMANAGER_H
