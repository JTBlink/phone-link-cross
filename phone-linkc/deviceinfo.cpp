#include "deviceinfo.h"
#include "libimobiledevice_dynamic.h"
#include <QDebug>

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
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        // 返回模拟设备信息
        return getSimulatedDeviceInfo(udid);
    }
    
    DeviceInfo info;
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    
    do {
        // 连接到设备
        if (!loader.idevice_new || loader.idevice_new(&device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
            qWarning() << "无法连接到设备:" << udid;
            break;
        }
        
        if (!loader.lockdownd_client_new_with_handshake || 
            loader.lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") != LOCKDOWN_E_SUCCESS) {
            qWarning() << "无法建立 lockdown 连接:" << udid;
            break;
        }
        
        // 获取设备信息
        info.udid = udid;
        info.name = this->getStringValue(lockdown, nullptr, "DeviceName");
        info.model = this->getStringValue(lockdown, nullptr, "ModelNumber");
        info.productVersion = this->getStringValue(lockdown, nullptr, "ProductVersion");
        info.buildVersion = this->getStringValue(lockdown, nullptr, "BuildVersion");
        info.serialNumber = this->getStringValue(lockdown, nullptr, "SerialNumber");
        info.deviceClass = this->getStringValue(lockdown, nullptr, "DeviceClass");
        info.productType = this->getStringValue(lockdown, nullptr, "ProductType");
        info.wifiAddress = this->getStringValue(lockdown, nullptr, "WiFiAddress");
        info.activationState = this->getStringValue(lockdown, nullptr, "ActivationState");
        
        // 获取存储信息
        info.totalCapacity = this->getIntValue(lockdown, "com.apple.disk_usage", "TotalDiskCapacity");
        info.availableCapacity = this->getIntValue(lockdown, "com.apple.disk_usage", "TotalDataAvailable");
        
    } while (false);
    
    // 清理资源
    if (lockdown && loader.lockdownd_client_free) {
        loader.lockdownd_client_free(lockdown);
    }
    if (device && loader.idevice_free) {
        loader.idevice_free(device);
    }
    
    return info;
}

QVariantMap DeviceInfoManager::getDetailedInfo(const QString &udid)
{
    DeviceInfo basicInfo = getDeviceInfo(udid);
    return basicInfo.toMap();
}

QString DeviceInfoManager::getStringValue(lockdownd_client_t lockdown, const char* domain, const char* key)
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized() || !loader.lockdownd_get_value) {
        return QString();
    }
    
    plist_t value = nullptr;
    QString result;
    
    if (loader.lockdownd_get_value(lockdown, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (loader.plist_get_node_type && loader.plist_get_node_type(value) == PLIST_STRING) {
            char *str_value = nullptr;
            if (loader.plist_get_string_val) {
                loader.plist_get_string_val(value, &str_value);
                if (str_value) {
                    result = QString::fromUtf8(str_value);
                    free(str_value);
                }
            }
        }
        if (loader.plist_free) {
            loader.plist_free(value);
        }
    }
    
    return result;
}

qint64 DeviceInfoManager::getIntValue(lockdownd_client_t lockdown, const char* domain, const char* key)
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized() || !loader.lockdownd_get_value) {
        return 0;
    }
    
    plist_t value = nullptr;
    qint64 result = 0;
    
    if (loader.lockdownd_get_value(lockdown, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (loader.plist_get_node_type && loader.plist_get_node_type(value) == PLIST_UINT) {
            uint64_t uint_value = 0;
            if (loader.plist_get_uint_val) {
                loader.plist_get_uint_val(value, &uint_value);
                result = static_cast<qint64>(uint_value);
            }
        }
        if (loader.plist_free) {
            loader.plist_free(value);
        }
    }
    
    return result;
}

bool DeviceInfoManager::getBoolValue(lockdownd_client_t lockdown, const char* domain, const char* key)
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized() || !loader.lockdownd_get_value) {
        return false;
    }
    
    plist_t value = nullptr;
    bool result = false;
    
    if (loader.lockdownd_get_value(lockdown, domain, key, &value) == LOCKDOWN_E_SUCCESS && value) {
        if (loader.plist_get_node_type && loader.plist_get_node_type(value) == PLIST_BOOLEAN) {
            uint8_t bool_value = 0;
            if (loader.plist_get_bool_val) {
                loader.plist_get_bool_val(value, &bool_value);
                result = bool_value != 0;
            }
        }
        if (loader.plist_free) {
            loader.plist_free(value);
        }
    }
    
    return result;
}

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
