
#include "deviceinfo.h"
#include "platform/libimobiledevice_dynamic.h"
#include <QDebug>
#include <QMap>

// 产品类型到友好名称的映射表
static const QMap<QString, QString> productTypeToFriendlyName = {
    // iPhone 系列
    {"iPhone1,1", "iPhone"},
    {"iPhone1,2", "iPhone 3G"},
    {"iPhone2,1", "iPhone 3GS"},
    {"iPhone3,1", "iPhone 4"},
    {"iPhone3,2", "iPhone 4"},
    {"iPhone3,3", "iPhone 4"},
    {"iPhone4,1", "iPhone 4S"},
    {"iPhone5,1", "iPhone 5"},
    {"iPhone5,2", "iPhone 5"},
    {"iPhone5,3", "iPhone 5c"},
    {"iPhone5,4", "iPhone 5c"},
    {"iPhone6,1", "iPhone 5s"},
    {"iPhone6,2", "iPhone 5s"},
    {"iPhone7,1", "iPhone 6 Plus"},
    {"iPhone7,2", "iPhone 6"},
    {"iPhone8,1", "iPhone 6s"},
    {"iPhone8,2", "iPhone 6s Plus"},
    {"iPhone8,4", "iPhone SE (1st gen)"},
    {"iPhone9,1", "iPhone 7"},
    {"iPhone9,2", "iPhone 7 Plus"},
    {"iPhone9,3", "iPhone 7"},
    {"iPhone9,4", "iPhone 7 Plus"},
    {"iPhone10,1", "iPhone 8"},
    {"iPhone10,2", "iPhone 8 Plus"},
    {"iPhone10,3", "iPhone X"},
    {"iPhone10,4", "iPhone 8"},
    {"iPhone10,5", "iPhone 8 Plus"},
    {"iPhone10,6", "iPhone X"},
    {"iPhone11,2", "iPhone XS"},
    {"iPhone11,4", "iPhone XS Max"},
    {"iPhone11,6", "iPhone XS Max"},
    {"iPhone11,8", "iPhone XR"},
    {"iPhone12,1", "iPhone 11"},
    {"iPhone12,3", "iPhone 11 Pro"},
    {"iPhone12,5", "iPhone 11 Pro Max"},
    {"iPhone12,8", "iPhone SE (2nd gen)"},
    {"iPhone13,1", "iPhone 12 mini"},
    {"iPhone13,2", "iPhone 12"},
    {"iPhone13,3", "iPhone 12 Pro"},
    {"iPhone13,4", "iPhone 12 Pro Max"},
    {"iPhone14,2", "iPhone 13 Pro"},
    {"iPhone14,3", "iPhone 13 Pro Max"},
    {"iPhone14,4", "iPhone 13 mini"},
    {"iPhone14,5", "iPhone 13"},
    {"iPhone14,6", "iPhone SE (3rd gen)"},
    {"iPhone14,7", "iPhone 14"},
    {"iPhone14,8", "iPhone 14 Plus"},
    {"iPhone15,2", "iPhone 14 Pro"},
    {"iPhone15,3", "iPhone 14 Pro Max"},
    {"iPhone15,4", "iPhone 15"},
    {"iPhone15,5", "iPhone 15 Plus"},
    {"iPhone16,1", "iPhone 15 Pro"},
    {"iPhone16,2", "iPhone 15 Pro Max"},
    {"iPhone17,1", "iPhone 16 Pro"},
    {"iPhone17,2", "iPhone 16 Pro Max"},
    {"iPhone17,3", "iPhone 16"},
    {"iPhone17,4", "iPhone 16 Plus"},
    
    // iPad 系列
    {"iPad1,1", "iPad"},
    {"iPad2,1", "iPad 2"},
    {"iPad2,2", "iPad 2"},
    {"iPad2,3", "iPad 2"},
    {"iPad2,4", "iPad 2"},
    {"iPad2,5", "iPad mini"},
    {"iPad2,6", "iPad mini"},
    {"iPad2,7", "iPad mini"},
    {"iPad3,1", "iPad (3rd gen)"},
    {"iPad3,2", "iPad (3rd gen)"},
    {"iPad3,3", "iPad (3rd gen)"},
    {"iPad3,4", "iPad (4th gen)"},
    {"iPad3,5", "iPad (4th gen)"},
    {"iPad3,6", "iPad (4th gen)"},
    {"iPad4,1", "iPad Air"},
    {"iPad4,2", "iPad Air"},
    {"iPad4,3", "iPad Air"},
    {"iPad4,4", "iPad mini 2"},
    {"iPad4,5", "iPad mini 2"},
    {"iPad4,6", "iPad mini 2"},
    {"iPad4,7", "iPad mini 3"},
    {"iPad4,8", "iPad mini 3"},
    {"iPad4,9", "iPad mini 3"},
    {"iPad5,1", "iPad mini 4"},
    {"iPad5,2", "iPad mini 4"},
    {"iPad5,3", "iPad Air 2"},
    {"iPad5,4", "iPad Air 2"},
    {"iPad6,3", "iPad Pro (9.7-inch)"},
    {"iPad6,4", "iPad Pro (9.7-inch)"},
    {"iPad6,7", "iPad Pro (12.9-inch)"},
    {"iPad6,8", "iPad Pro (12.9-inch)"},
    {"iPad6,11", "iPad (5th gen)"},
    {"iPad6,12", "iPad (5th gen)"},
    {"iPad7,1", "iPad Pro (12.9-inch, 2nd gen)"},
    {"iPad7,2", "iPad Pro (12.9-inch, 2nd gen)"},
    {"iPad7,3", "iPad Pro (10.5-inch)"},
    {"iPad7,4", "iPad Pro (10.5-inch)"},
    {"iPad7,5", "iPad (6th gen)"},
    {"iPad7,6", "iPad (6th gen)"},
    {"iPad7,11", "iPad (7th gen)"},
    {"iPad7,12", "iPad (7th gen)"},
    {"iPad8,1", "iPad Pro (11-inch)"},
    {"iPad8,2", "iPad Pro (11-inch)"},
    {"iPad8,3", "iPad Pro (11-inch)"},
    {"iPad8,4", "iPad Pro (11-inch)"},
    {"iPad8,5", "iPad Pro (12.9-inch, 3rd gen)"},
    {"iPad8,6", "iPad Pro (12.9-inch, 3rd gen)"},
    {"iPad8,7", "iPad Pro (12.9-inch, 3rd gen)"},
    {"iPad8,8", "iPad Pro (12.9-inch, 3rd gen)"},
    {"iPad8,9", "iPad Pro (11-inch, 2nd gen)"},
    {"iPad8,10", "iPad Pro (11-inch, 2nd gen)"},
    {"iPad8,11", "iPad Pro (12.9-inch, 4th gen)"},
    {"iPad8,12", "iPad Pro (12.9-inch, 4th gen)"},
    {"iPad11,1", "iPad mini (5th gen)"},
    {"iPad11,2", "iPad mini (5th gen)"},
    {"iPad11,3", "iPad Air (3rd gen)"},
    {"iPad11,4", "iPad Air (3rd gen)"},
    {"iPad11,6", "iPad (8th gen)"},
    {"iPad11,7", "iPad (8th gen)"},
    {"iPad12,1", "iPad (9th gen)"},
    {"iPad12,2", "iPad (9th gen)"},
    {"iPad13,1", "iPad Air (4th gen)"},
    {"iPad13,2", "iPad Air (4th gen)"},
    {"iPad13,4", "iPad Pro (11-inch, 3rd gen)"},
    {"iPad13,5", "iPad Pro (11-inch, 3rd gen)"},
    {"iPad13,6", "iPad Pro (11-inch, 3rd gen)"},
    {"iPad13,7", "iPad Pro (11-inch, 3rd gen)"},
    {"iPad13,8", "iPad Pro (12.9-inch, 5th gen)"},
    {"iPad13,9", "iPad Pro (12.9-inch, 5th gen)"},
    {"iPad13,10", "iPad Pro (12.9-inch, 5th gen)"},
    {"iPad13,11", "iPad Pro (12.9-inch, 5th gen)"},
    {"iPad13,16", "iPad Air (5th gen)"},
    {"iPad13,17", "iPad Air (5th gen)"},
    {"iPad13,18", "iPad (10th gen)"},
    {"iPad13,19", "iPad (10th gen)"},
    {"iPad14,1", "iPad mini (6th gen)"},
    {"iPad14,2", "iPad mini (6th gen)"},
    {"iPad14,3", "iPad Pro (11-inch, 4th gen)"},
    {"iPad14,4", "iPad Pro (11-inch, 4th gen)"},
    {"iPad14,5", "iPad Pro (12.9-inch, 6th gen)"},
    {"iPad14,6", "iPad Pro (12.9-inch, 6th gen)"},
    {"iPad14,8", "iPad Air (11-inch, M2)"},
    {"iPad14,9", "iPad Air (11-inch, M2)"},
    {"iPad14,10", "iPad Air (13-inch, M2)"},
    {"iPad14,11", "iPad Air (13-inch, M2)"},
    {"iPad16,3", "iPad Pro (11-inch, M4)"},
    {"iPad16,4", "iPad Pro (11-inch, M4)"},
    {"iPad16,5", "iPad Pro (13-inch, M4)"},
    {"iPad16,6", "iPad Pro (13-inch, M4)"},
    
    // iPod touch 系列
    {"iPod1,1", "iPod touch"},
    {"iPod2,1", "iPod touch (2nd gen)"},
    {"iPod3,1", "iPod touch (3rd gen)"},
    {"iPod4,1", "iPod touch (4th gen)"},
    {"iPod5,1", "iPod touch (5th gen)"},
    {"iPod7,1", "iPod touch (6th gen)"},
    {"iPod9,1", "iPod touch (7th gen)"},
};

QString DeviceInfo::formatCapacity(qint64 bytes)
{
    if (bytes <= 0) return "未知";
    
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    const qint64 TB = GB * 1024;
    
    if (bytes >= TB) {
        return QString("%1 TB").arg(static_cast<double>(bytes) / TB, 0, 'f', 2);
    } else if (bytes >= GB) {
        return QString("%1 GB").arg(static_cast<double>(bytes) / GB, 0, 'f', 2);
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(static_cast<double>(bytes) / MB, 0, 'f', 2);
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(static_cast<double>(bytes) / KB, 0, 'f', 2);
    } else {
        return QString("%1 B").arg(bytes);
    }
}

QString DeviceInfo::getFriendlyModelName() const
{
    if (productTypeToFriendlyName.contains(productType)) {
        return productTypeToFriendlyName.value(productType);
    }
    // 如果找不到映射，返回原始产品类型
    return productType.isEmpty() ? "未知设备" : productType;
}

QVariantMap DeviceInfo::toMap() const
{
    QVariantMap map;
    
    // 基础标识信息
    map["udid"] = udid;
    map["name"] = name;
    map["serialNumber"] = serialNumber;
    
    // 产品信息
    map["deviceClass"] = deviceClass;
    map["productType"] = productType;
    map["productName"] = productName;
    map["modelNumber"] = modelNumber;
    map["regionInfo"] = regionInfo;
    map["friendlyModelName"] = getFriendlyModelName();
    
    // 硬件信息
    map["hardwareModel"] = hardwareModel;
    map["hardwarePlatform"] = hardwarePlatform;
    map["cpuArchitecture"] = cpuArchitecture;
    map["chipID"] = chipID;
    map["boardId"] = boardId;
    map["uniqueChipID"] = uniqueChipID;
    map["dieID"] = dieID;
    map["firmwareVersion"] = firmwareVersion;
    
    // 系统版本信息
    map["productVersion"] = productVersion;
    map["buildVersion"] = buildVersion;
    map["basebandVersion"] = basebandVersion;
    map["firmwareRevision"] = firmwareRevision;
    
    // 外观信息
    map["deviceColor"] = deviceColor;
    map["enclosureColor"] = enclosureColor;
    
    // 存储信息
    map["totalCapacity"] = totalCapacity;
    map["availableCapacity"] = availableCapacity;
    map["totalCapacityFormatted"] = formatCapacity(totalCapacity);
    map["availableCapacityFormatted"] = formatCapacity(availableCapacity);
    map["totalSystemCapacity"] = totalSystemCapacity;
    map["totalSystemAvailable"] = totalSystemAvailable;
    map["totalDataCapacity"] = totalDataCapacity;
    map["totalDataAvailable"] = totalDataAvailable;
    map["amountDataReserved"] = amountDataReserved;
    map["amountDataAvailable"] = amountDataAvailable;
    
    // 网络信息
    map["wifiAddress"] = wifiAddress;
    map["bluetoothAddress"] = bluetoothAddress;
    map["ethernetAddress"] = ethernetAddress;
    
    // 电话/SIM信息
    map["phoneNumber"] = phoneNumber;
    map["imei"] = imei;
    map["imei2"] = imei2;
    map["meid"] = meid;
    map["iccid"] = iccid;
    map["carrierBundleInfoVersion"] = carrierBundleInfoVersion;
    
    // 电池信息
    map["batteryCurrentCapacity"] = batteryCurrentCapacity;
    map["batteryIsCharging"] = batteryIsCharging;
    map["externalChargeCapable"] = externalChargeCapable;
    map["fullyCharged"] = fullyCharged;
    
    // 设备状态
    map["activationState"] = activationState;
    map["passwordProtected"] = passwordProtected;
    map["deviceSupportsLockdown"] = deviceSupportsLockdown;
    map["hostAttached"] = hostAttached;
    map["hasSimLockedStatus"] = hasSimLockedStatus;
    map["iTunesHasConnected"] = iTunesHasConnected;
    
    // 时间信息
    map["timeZone"] = timeZone;
    map["timeIntervalSince1970"] = timeIntervalSince1970;
    map["deviceTime"] = deviceTime;
    
    // 安全信息
    map["trustedHostAttached"] = trustedHostAttached;
    map["pairingState"] = pairingState;
    
    // 其他信息
    map["mlbSerialNumber"] = mlbSerialNumber;
    map["protocolVersion"] = protocolVersion;
    map["supportsWirelessSync"] = supportsWirelessSync;
    map["wirelessBuddyID"] = wirelessBuddyID;
    
    return map;
}

QString DeviceInfo::toString() const
{
    // 使用简洁的纯文本格式，无分割线，用空行分隔各部分
    QStringList lines;
    
    lines << "设备详细信息";
    
    // 基本信息
    lines << ""
          << "【基本信息】"
          << QString("  设备名称: %1").arg(name)
          << QString("  设备型号: %1 (%2)").arg(getFriendlyModelName(), productType)
          << QString("  序列号: %1").arg(serialNumber)
          << QString("  UDID: %1").arg(udid)
          << QString("  设备类别: %1").arg(deviceClass)
          << QString("  区域版本: %1").arg(regionInfo.isEmpty() ? "未知" : regionInfo);
    
    // 系统信息
    lines << ""
          << "【系统信息】"
          << QString("  iOS版本: %1 (%2)").arg(productVersion, buildVersion)
          << QString("  产品名称: %1").arg(productName.isEmpty() ? "iPhone OS" : productName)
          << QString("  基带版本: %1").arg(basebandVersion.isEmpty() ? "未知" : basebandVersion)
          << QString("  激活状态: %1").arg(activationState)
          << QString("  时区: %1").arg(timeZone.isEmpty() ? "未知" : timeZone);
    
    // 硬件信息
    lines << ""
          << "【硬件信息】"
          << QString("  硬件型号: %1").arg(hardwareModel.isEmpty() ? "未知" : hardwareModel)
          << QString("  型号编号: %1").arg(modelNumber.isEmpty() ? "未知" : modelNumber)
          << QString("  CPU架构: %1").arg(cpuArchitecture.isEmpty() ? "未知" : cpuArchitecture)
          << QString("  芯片ID: %1").arg(chipID.isEmpty() ? "未知" : chipID)
          << QString("  主板ID: %1").arg(boardId.isEmpty() ? "未知" : boardId)
          << QString("  设备颜色: %1").arg(deviceColor.isEmpty() ? "未知" : deviceColor);
    
    // 存储信息
    qint64 usedCapacity = totalCapacity - availableCapacity;
    double usagePercent = totalCapacity > 0 ? (static_cast<double>(usedCapacity) / totalCapacity * 100) : 0;
    
    lines << ""
          << "【存储信息】"
          << QString("  物理磁盘: %1").arg(formatCapacity(totalCapacity))
          << QString("  已使用: %1 (%2%)")
                  .arg(formatCapacity(usedCapacity))
                  .arg(usagePercent, 0, 'f', 1)
          << QString("  可用空间: %1").arg(formatCapacity(availableCapacity));
    
    // 分区详情
    if (totalSystemCapacity > 0 || totalDataCapacity > 0) {
        lines << "";
        lines << "  ┌─ 分区详情 ─";
        
        if (totalSystemCapacity > 0) {
            qint64 systemUsed = totalSystemCapacity - totalSystemAvailable;
            double systemUsagePercent = totalSystemCapacity > 0 ?
                (static_cast<double>(systemUsed) / totalSystemCapacity * 100) : 0;
            lines << QString("  │ 系统分区: %1 / %2 (使用 %3%)")
                      .arg(formatCapacity(systemUsed),
                           formatCapacity(totalSystemCapacity))
                      .arg(systemUsagePercent, 0, 'f', 1);
        }
        
        if (totalDataCapacity > 0) {
            qint64 dataUsed = totalDataCapacity - totalDataAvailable;
            double dataUsagePercent = totalDataCapacity > 0 ?
                (static_cast<double>(dataUsed) / totalDataCapacity * 100) : 0;
            lines << QString("  │ 数据分区: %1 / %2 (使用 %3%)")
                      .arg(formatCapacity(dataUsed),
                           formatCapacity(totalDataCapacity))
                      .arg(dataUsagePercent, 0, 'f', 1);
        }
        
        if (amountDataReserved > 0) {
            lines << QString("  │ 系统预留: %1").arg(formatCapacity(amountDataReserved));
        }
        
        if (amountDataAvailable > 0 && amountDataAvailable != totalDataAvailable) {
            lines << QString("  │ 立即可用: %1 (扣除预留)").arg(formatCapacity(amountDataAvailable));
        }
        
        lines << "  └────────────";
    }
    
    // 网络信息
    lines << ""
          << "【网络信息】"
          << QString("  WiFi地址: %1").arg(wifiAddress.isEmpty() ? "未知" : wifiAddress)
          << QString("  蓝牙地址: %1").arg(bluetoothAddress.isEmpty() ? "未知" : bluetoothAddress);
    
    if (!ethernetAddress.isEmpty()) {
        lines << QString("  以太网: %1").arg(ethernetAddress);
    }
    
    // 电话信息 (如果有)
    if (!phoneNumber.isEmpty() || !imei.isEmpty()) {
        lines << ""
              << "【电话信息】";
        
        if (!phoneNumber.isEmpty()) {
            lines << QString("  电话号码: %1").arg(phoneNumber);
        }
        if (!imei.isEmpty()) {
            lines << QString("  IMEI: %1").arg(imei);
        }
        if (!imei2.isEmpty()) {
            lines << QString("  IMEI2: %1").arg(imei2);
        }
        if (!meid.isEmpty()) {
            lines << QString("  MEID: %1").arg(meid);
        }
        if (!iccid.isEmpty()) {
            lines << QString("  ICCID: %1").arg(iccid);
        }
    }
    
    // 电池信息
    lines << ""
          << "【电池信息】"
          << QString("  电量: %1%").arg(batteryCurrentCapacity)
          << QString("  充电状态: %1")
                  .arg(batteryIsCharging ? "正在充电" : (fullyCharged ? "已充满" : "未充电"));
    
    // 安全状态
    lines << ""
          << "【安全状态】"
          << QString("  密码保护: %1").arg(passwordProtected ? "是" : "否")
          << QString("  配对状态: %1").arg(pairingState.isEmpty() ? "未知" : pairingState)
          << QString("  信任主机: %1").arg(trustedHostAttached ? "是" : "否");
    
    return lines.join('\n');
}

DeviceInfoManager::DeviceInfoManager(QObject *parent)
    : QObject(parent)
{
}

DeviceInfo DeviceInfoManager::getDeviceInfo(const QString &udid)
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        // 库未初始化，返回空的设备信息
        qWarning() << "libimobiledevice 库未初始化，无法获取设备信息";
        DeviceInfo info;
        info.udid = udid;
        return info;
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
        
        // ===== 基础标识信息 =====
        info.udid = udid;
        info.name = this->getStringValue(lockdown, nullptr, "DeviceName");
        info.serialNumber = this->getStringValue(lockdown, nullptr, "SerialNumber");
        
        // ===== 产品信息 =====
        info.deviceClass = this->getStringValue(lockdown, nullptr, "DeviceClass");
        info.productType = this->getStringValue(lockdown, nullptr, "ProductType");
        info.productName = this->getStringValue(lockdown, nullptr, "ProductName");
        info.modelNumber = this->getStringValue(lockdown, nullptr, "ModelNumber");
        info.regionInfo = this->getStringValue(lockdown, nullptr, "RegionInfo");
        
        // ===== 硬件信息 =====
        info.hardwareModel = this->getStringValue(lockdown, nullptr, "HardwareModel");
        info.hardwarePlatform = this->getStringValue(lockdown, nullptr, "HardwarePlatform");
        info.cpuArchitecture = this->getStringValue(lockdown, nullptr, "CPUArchitecture");
        info.chipID = this->getStringValue(lockdown, nullptr, "ChipID");
        info.boardId = this->getStringValue(lockdown, nullptr, "BoardId");
        info.uniqueChipID = this->getStringValue(lockdown, nullptr, "UniqueChipID");
        info.dieID = this->getStringValue(lockdown, nullptr, "DieID");
        info.firmwareVersion = static_cast<qint32>(this->getIntValue(lockdown, nullptr, "FirmwareVersion"));
        
        // ===== 系统版本信息 =====
        info.productVersion = this->getStringValue(lockdown, nullptr, "ProductVersion");
        info.buildVersion = this->getStringValue(lockdown, nullptr, "BuildVersion");
        info.basebandVersion = this->getStringValue(lockdown, nullptr, "BasebandVersion");
        info.firmwareRevision = this->getStringValue(lockdown, nullptr, "FirmwareRevision");
        
        // ===== 外观信息 =====
        info.deviceColor = this->getStringValue(lockdown, nullptr, "DeviceColor");
        info.enclosureColor = this->getStringValue(lockdown, nullptr, "EnclosureColor");
        
        // ===== 存储信息 (使用 com.apple.disk_usage 域) =====
        // TotalDiskCapacity 是物理磁盘总容量（如128GB）
        // TotalDataCapacity + TotalSystemCapacity 才是实际可用分区总容量
        info.totalCapacity = this->getIntValue(lockdown, "com.apple.disk_usage", "TotalDiskCapacity");
        // TotalDataAvailable 是数据分区可用空间，与手机设置中显示的"可用空间"一致
        info.availableCapacity = this->getIntValue(lockdown, "com.apple.disk_usage", "TotalDataAvailable");
        info.totalSystemCapacity = this->getIntValue(lockdown, "com.apple.disk_usage", "TotalSystemCapacity");
        info.totalSystemAvailable = this->getIntValue(lockdown, "com.apple.disk_usage", "TotalSystemAvailable");
        info.totalDataCapacity = this->getIntValue(lockdown, "com.apple.disk_usage", "TotalDataCapacity");
        info.totalDataAvailable = this->getIntValue(lockdown, "com.apple.disk_usage", "TotalDataAvailable");
        info.amountDataReserved = this->getIntValue(lockdown, "com.apple.disk_usage", "AmountDataReserved");
        info.amountDataAvailable = this->getIntValue(lockdown, "com.apple.disk_usage", "AmountDataAvailable");
        
        // ===== 网络信息 =====
        info.wifiAddress = this->getStringValue(lockdown, nullptr, "WiFiAddress");
        info.bluetoothAddress = this->getStringValue(lockdown, nullptr, "BluetoothAddress");
        info.ethernetAddress = this->getStringValue(lockdown, nullptr, "EthernetAddress");
        
        // ===== 电话/SIM信息 =====
        info.phoneNumber = this->getStringValue(lockdown, nullptr, "PhoneNumber");
        info.imei = this->getStringValue(lockdown, nullptr, "InternationalMobileEquipmentIdentity");
        info.imei2 = this->getStringValue(lockdown, nullptr, "InternationalMobileEquipmentIdentity2");
        info.meid = this->getStringValue(lockdown, nullptr, "MobileEquipmentIdentifier");
        info.iccid = this->getStringValue(lockdown, nullptr, "IntegratedCircuitCardIdentity");
        info.carrierBundleInfoVersion = this->getStringValue(lockdown, nullptr, "CarrierBundleInfoVersion");
        
        // ===== 电池信息 (使用 com.apple.mobile.battery 域) =====
        info.batteryCurrentCapacity = static_cast<qint32>(this->getIntValue(lockdown, "com.apple.mobile.battery", "BatteryCurrentCapacity"));
        info.batteryIsCharging = this->getBoolValue(lockdown, "com.apple.mobile.battery", "BatteryIsCharging");
        info.externalChargeCapable = this->getStringValue(lockdown, "com.apple.mobile.battery", "ExternalChargeCapable");
        info.fullyCharged = this->getBoolValue(lockdown, "com.apple.mobile.battery", "FullyCharged");
        
        // ===== 设备状态 =====
        info.activationState = this->getStringValue(lockdown, nullptr, "ActivationState");
        info.passwordProtected = this->getBoolValue(lockdown, nullptr, "PasswordProtected");
        info.deviceSupportsLockdown = this->getBoolValue(lockdown, nullptr, "DeviceSupportsLockdown");
        info.hostAttached = this->getBoolValue(lockdown, nullptr, "HostAttached");
        info.hasSimLockedStatus = this->getBoolValue(lockdown, nullptr, "HasSiMLock");
        info.iTunesHasConnected = this->getBoolValue(lockdown, nullptr, "iTunesHasConnected");
        
        // ===== 时间信息 =====
        info.timeZone = this->getStringValue(lockdown, nullptr, "TimeZone");
        info.timeIntervalSince1970 = this->getIntValue(lockdown, nullptr, "TimeIntervalSince1970");
        if (info.timeIntervalSince1970 > 0) {
            info.deviceTime = QDateTime::fromSecsSinceEpoch(info.timeIntervalSince1970);
        }
        
        // ===== 安全信息 =====
        info.trustedHostAttached = this->getBoolValue(lockdown, nullptr, "TrustedHostAttached");
        info.pairingState = this->getStringValue(lockdown, nullptr, "PairingState");
        
        // ===== 其他信息 =====
        info.mlbSerialNumber = this->getStringValue(lockdown, nullptr, "MLBSerialNumber");
        info.protocolVersion = this->getStringValue(lockdown, nullptr, "ProtocolVersion");
        info.supportsWirelessSync = this->getBoolValue(lockdown, nullptr, "SupportsWirelessSync");
        info.wirelessBuddyID = this->getStringValue(lockdown, nullptr, "WirelessBuddyID");
        
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

QVariantMap DeviceInfoManager::getBatteryInfo(const QString &udid)
{
    QVariantMap batteryInfo;
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        // 库未初始化，返回空的电池信息
        qWarning() << "libimobiledevice 库未初始化，无法获取电池信息";
        return batteryInfo;
    }
    
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    
    do {
        if (!loader.idevice_new || loader.idevice_new(&device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
            break;
        }
        
        if (!loader.lockdownd_client_new_with_handshake || 
            loader.lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") != LOCKDOWN_E_SUCCESS) {
            break;
        }
        
        batteryInfo["BatteryCurrentCapacity"] = static_cast<int>(getIntValue(lockdown, "com.apple.mobile.battery", "BatteryCurrentCapacity"));
        batteryInfo["BatteryIsCharging"] = getBoolValue(lockdown, "com.apple.mobile.battery", "BatteryIsCharging");
        batteryInfo["FullyCharged"] = getBoolValue(lockdown, "com.apple.mobile.battery", "FullyCharged");
        batteryInfo["ExternalChargeCapable"] = getBoolValue(lockdown, "com.apple.mobile.battery", "ExternalChargeCapable");
        batteryInfo["ExternalConnected"] = getBoolValue(lockdown, "com.apple.mobile.battery", "ExternalConnected");
        batteryInfo["GasGaugeBatteryCapacity"] = static_cast<int>(getIntValue(lockdown, "com.apple.mobile.battery", "GasGaugeBatteryCapacity"));
        batteryInfo["GasGaugeCapability"] = getBoolValue(lockdown, "com.apple.mobile.battery", "GasGaugeCapability");
        
    } while (false);
    
    if (lockdown && loader.lockdownd_client_free) {
        loader.lockdownd_client_free(lockdown);
    }
    if (device && loader.idevice_free) {
        loader.idevice_free(device);
    }
    
    return batteryInfo;
}

QVariantMap DeviceInfoManager::getDiskUsageInfo(const QString &udid)
{
    QVariantMap diskInfo;
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        // 库未初始化，返回空的存储信息
        qWarning() << "libimobiledevice 库未初始化，无法获取存储信息";
        return diskInfo;
    }
    
    idevice_t device = nullptr;
    lockdownd_client_t lockdown = nullptr;
    
    do {
        if (!loader.idevice_new || loader.idevice_new(&device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
            break;
        }
        
        if (!loader.lockdownd_client_new_with_handshake || 
            loader.lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") != LOCKDOWN_E_SUCCESS) {
            break;
        }
        
        qint64 totalDisk = getIntValue(lockdown, "com.apple.disk_usage", "TotalDiskCapacity");
        qint64 totalDataAvailable = getIntValue(lockdown, "com.apple.disk_usage", "TotalDataAvailable");
        qint64 totalSystemCapacity = getIntValue(lockdown, "com.apple.disk_usage", "TotalSystemCapacity");
        qint64 totalSystemAvailable = getIntValue(lockdown, "com.apple.disk_usage", "TotalSystemAvailable");
        qint64 totalDataCapacity = getIntValue(lockdown, "com.apple.disk_usage", "TotalDataCapacity");
        qint64 amountDataReserved = getIntValue(lockdown, "com.apple.disk_usage", "AmountDataReserved");
        qint64 amountDataAvailable = getIntValue(lockdown, "com.apple.disk_usage", "AmountDataAvailable");
        
        diskInfo["TotalDiskCapacity"] = totalDisk;
        diskInfo["TotalDataAvailable"] = totalDataAvailable;
        diskInfo["TotalSystemCapacity"] = totalSystemCapacity;
        diskInfo["TotalSystemAvailable"] = totalSystemAvailable;
        diskInfo["TotalDataCapacity"] = totalDataCapacity;
        diskInfo["AmountDataReserved"] = amountDataReserved;
        diskInfo["AmountDataAvailable"] = amountDataAvailable;
        
        diskInfo["TotalDiskCapacityFormatted"] = DeviceInfo::formatCapacity(totalDisk);
        diskInfo["TotalDataAvailableFormatted"] = DeviceInfo::formatCapacity(totalDataAvailable);
        diskInfo["TotalSystemCapacityFormatted"] = DeviceInfo::formatCapacity(totalSystemCapacity);
        diskInfo["TotalDataCapacityFormatted"] = DeviceInfo::formatCapacity(totalDataCapacity);
        
        // 计算使用百分比
        if (totalDisk > 0) {
            double usedPercent = static_cast<double>(totalDisk - totalDataAvailable) / totalDisk * 100;
            diskInfo["UsedPercentage"] = usedPercent;
        }
        
    } while (false);
    
    if (lockdown && loader.lockdownd_client_free) {
        loader.lockdownd_client_free(lockdown);
    }
    if (device && loader.idevice_free) {
        loader.idevice_free(device);
    }
    
    return diskInfo;
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
            // 使用 plist_get_string_ptr 获取字符串指针，无需手动释放内存
            // 该指针在 plist_free(value) 调用前有效
            if (loader.plist_get_string_ptr) {
                uint64_t length = 0;
                const char* str_ptr = loader.plist_get_string_ptr(value, &length);
                if (str_ptr) {
                    result = QString::fromUtf8(str_ptr, static_cast<int>(length));
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

double DeviceInfoManager::getDoubleValue(lockdownd_client_t lockdown, const char* domain, const char* key)
{
    // 注意: plist_get_real_val 函数暂未在动态库加载器中实现
    // 如果需要获取浮点数值，可以尝试作为整数获取
    Q_UNUSED(lockdown);
    Q_UNUSED(domain);
    Q_UNUSED(key);
    return 0.0;
}
