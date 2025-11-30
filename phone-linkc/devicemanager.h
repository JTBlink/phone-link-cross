#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>

#ifdef HAVE_LIBIMOBILEDEVICE
// Forward declarations to avoid including headers in header file
typedef struct idevice_private idevice_private;
typedef idevice_private* idevice_t;
typedef struct lockdownd_client_private lockdownd_client_private;
typedef lockdownd_client_private* lockdownd_client_t;
// Forward declare the subscription context
typedef struct idevice_subscription_context* idevice_subscription_context_t;
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
    void refreshDevices();
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


private:
    QStringList m_knownDevices;
    QString m_currentUdid;
    bool m_isConnected;

#ifdef HAVE_LIBIMOBILEDEVICE
    idevice_t m_device;
    lockdownd_client_t m_lockdown;
    idevice_subscription_context_t m_eventContext;
    
    bool initializeConnection(const QString &udid);
    QString getDeviceName(const QString &udid);
    void cleanup();
    void scanCurrentDevices();
    
    // 事件订阅相关
    void startEventSubscription();
    void stopEventSubscription();
    static void deviceEventCallback(const void* event, void* user_data);
#endif

};

#endif // DEVICEMANAGER_H
