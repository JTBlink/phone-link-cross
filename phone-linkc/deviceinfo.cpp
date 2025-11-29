#include "deviceinfo.h"
#include <QDebug>

#ifdef HAVE_LIBIMOBILEDEVICE
#include <plist/plist.h>
#endif

QVariantMap DeviceInfo::toMap() const
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
    map["totalCapacity"] = totalCapacity;
    map["availableCapacity"] = availableCapacity;
    map["wifiAddress"] = wifiAddress;
    map["activationState"] = activationState;
    return map;
}

QString DeviceInfo::toString() const
{
    return QString("设备: %1 (%2)\n"
                   "型号: %3\n"
                   "系统版本: %4 (%5)\n"
                   "序列号: %6\n"
                   "设备类型: %7\n"
                   "产品类型: %8\n"
                   "总容量: %9 GB\n"
                   "可用容量: %10 GB\n"
                   "WiFi地址: %11\n"
                   "激活状态: %12")
            .arg(name, udid)
            .arg(model)
            .arg(productVersion, buildVersion)
            .arg(serialNumber)
            .arg(deviceClass)
            .arg(productType)
            .arg(totalCapacity / (1024*1024*1024))
            .arg(availableCapacity / (1024*1024*1024))
            .arg(wifiAddress)
            .arg(activationState);
}

DeviceInfoManager::DeviceInfoManager(QObject *parent)
    : QObject(parent)
{
}

DeviceInfo DeviceInfoManager::getDeviceInfo(const QString &udid)
{
#ifdef HAVE_LIBIMOBILEDEVICE
    DeviceInfo info;
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    
    do {
        // 连接到设备
        if (idevice_new(&device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
            qWarning() << "无法连接到设备:" << udid;
            break;
        }
        
        if (lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") != LOCKDOWN_E_SUCCESS) {
            qWarning() << "无法建立 lockdown 连接:" << udid;
            break;
        }
        
        // 获取设备信息
        info.udid = udid;
        info.name = getStringValue(lockdown, nullptr, "DeviceName");
        info.model = getStringValue(lockdown, nullptr, "ModelNumber");
        info.productVersion = getStringValue(lockdown, nullptr, "ProductVersion");
        info.buildVersion = getStringValue(lockdown, nullptr, "BuildVersion");
        info.serialNumber = getStringValue(lockdown, nullptr, "SerialNumber");
        info.deviceClass = getStringValue(lockdown, nullptr, "DeviceClass");
        info.productType = getStringValue(lockdown, nullptr, "ProductType");
        info.wifiAddress = getStringValue(lockdown, nullptr, "WiFiAddress");
        info.activationState = getStringValue(lockdown, nullptr, "ActivationState");
        
        // 获取存储信息
        info.totalCapacity = getIntValue(lockdown, "com.apple.disk_usage", "TotalDiskCapacity");
        info.availableCapacity = getIntValue(lockdown, "com.apple.disk_usage", "TotalDataAvailable");
        
    } while (false);
    
    // 清理资源
    if (lockdown) lockdownd_client_free(lockdown);
    if (device) idevice_free(device);
    
    return info;
#else
    // 返回模拟设备信息
    return getSimulatedDeviceInfo(udid);
#endif
}

QVariantMap DeviceInfoManager::getDetailedInfo(const QString &udid)
{
    DeviceInfo basicInfo = getDeviceInfo(udid);
    return basicInfo.toMap();
}

#ifdef HAVE_LIBIMOBILEDEVICE
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
#endif

DeviceInfo DeviceInfoManager::getSimulatedDeviceInfo(const QString &udid)
{
    DeviceInfo info;
    info.udid = udid;
    info.name = "模拟 iPhone";
    info.model = "A2482";
    info.productVersion = "17.1.1";
    info.buildVersion = "21B91";
    info.serialNumber = "F2LMQDXQ0D6Y";
    info.deviceClass = "iPhone";
    info.productType = "iPhone15,2";
    info.totalCapacity = 128LL * 1024 * 1024 * 1024; // 128GB
    info.availableCapacity = 64LL * 1024 * 1024 * 1024; // 64GB 可用
    info.wifiAddress = "aa:bb:cc:dd:ee:ff";
    info.activationState = "Activated";
    
    return info;
}
