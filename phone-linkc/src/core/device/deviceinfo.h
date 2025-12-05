#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QObject>
#include <QVariantMap>

// Forward declarations for libimobiledevice types (used with dynamic loading)
typedef struct lockdownd_client_private lockdownd_client_private;
typedef lockdownd_client_private* lockdownd_client_t;

struct DeviceInfo {
    QString udid;
    QString name;
    QString model;
    QString productVersion;
    QString buildVersion;
    QString serialNumber;
    QString deviceClass;
    QString productType;
    qint64 totalCapacity;
    qint64 availableCapacity;
    QString wifiAddress;
    QString activationState;
    
    QVariantMap toMap() const;
    QString toString() const;
};

class DeviceInfoManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceInfoManager(QObject *parent = nullptr);
    
    // 获取设备信息
    DeviceInfo getDeviceInfo(const QString &udid);
    QVariantMap getDetailedInfo(const QString &udid);

private:
    // libimobiledevice辅助方法（动态加载模式）
    QString getStringValue(lockdownd_client_t lockdown, const char* domain, const char* key);
    qint64 getIntValue(lockdownd_client_t lockdown, const char* domain, const char* key);
    bool getBoolValue(lockdownd_client_t lockdown, const char* domain, const char* key);

    // 模拟设备信息（用于没有 libimobiledevice 的情况）
    DeviceInfo getSimulatedDeviceInfo(const QString &udid);
};

#endif // DEVICEINFO_H