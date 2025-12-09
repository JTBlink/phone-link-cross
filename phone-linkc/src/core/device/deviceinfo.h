#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QObject>
#include <QVariantMap>
#include <QDateTime>

#include <libimobiledevice/lockdown.h>

/**
 * @brief 设备详细信息结构体
 * 包含 iOS 设备的完整硬件和软件信息
 */
struct DeviceInfo {
    // ===== 基础标识信息 =====
    QString udid;                   // 设备唯一标识符 (Unique Device Identifier)
    QString name;                   // 用户设置的设备名称 (DeviceName)
    QString serialNumber;           // 设备序列号 (SerialNumber)
    
    // ===== 产品信息 =====
    QString deviceClass;            // 设备类别 (DeviceClass: iPhone/iPad/iPod)
    QString productType;            // 产品类型 (ProductType: iPhone15,2)
    QString productName;            // 产品名称 (ProductName: iPhone OS)
    QString modelNumber;            // 型号编号 (ModelNumber: A2482)
    QString regionInfo;             // 区域信息 (RegionInfo: CH/A)
    
    // ===== 硬件信息 =====
    QString hardwareModel;          // 硬件型号 (HardwareModel: D73AP)
    QString hardwarePlatform;       // 硬件平台 (HardwarePlatform)
    QString cpuArchitecture;        // CPU架构 (CPUArchitecture: arm64e)
    QString chipID;                 // 芯片ID (ChipID)
    QString boardId;                // 主板ID (BoardId)
    QString uniqueChipID;           // 唯一芯片ID (UniqueChipID)
    QString dieID;                  // Die ID (DieID)
    qint32 firmwareVersion;         // 固件版本 (FirmwareVersion)
    
    // ===== 系统版本信息 =====
    QString productVersion;         // iOS版本 (ProductVersion: 17.1.1)
    QString buildVersion;           // 构建版本 (BuildVersion: 21B91)
    QString basebandVersion;        // 基带版本 (BasebandVersion)
    QString firmwareRevision;       // 固件修订版 (FirmwareRevision)
    
    // ===== 外观信息 =====
    QString deviceColor;            // 设备颜色 (DeviceColor)
    QString enclosureColor;         // 外壳颜色 (EnclosureColor)
    
    // ===== 存储信息 =====
    qint64 totalCapacity;           // 总存储容量 (字节)
    qint64 availableCapacity;       // 可用存储容量 (字节)
    qint64 totalSystemCapacity;     // 系统分区总容量 (字节)
    qint64 totalSystemAvailable;    // 系统分区可用容量 (字节)
    qint64 totalDataCapacity;       // 数据分区总容量 (字节)
    qint64 totalDataAvailable;      // 数据分区可用容量 (字节)
    qint64 amountDataReserved;      // 预留数据空间 (字节)
    qint64 amountDataAvailable;     // 实际可用数据空间 (字节)
    
    // ===== 网络信息 =====
    QString wifiAddress;            // WiFi MAC地址 (WiFiAddress)
    QString bluetoothAddress;       // 蓝牙MAC地址 (BluetoothAddress)
    QString ethernetAddress;        // 以太网MAC地址 (EthernetAddress)
    
    // ===== 电话/SIM信息 =====
    QString phoneNumber;            // 电话号码 (PhoneNumber)
    QString imei;                   // IMEI号码 (InternationalMobileEquipmentIdentity)
    QString imei2;                  // 第二个IMEI (双卡设备)
    QString meid;                   // MEID号码 (MobileEquipmentIdentifier)
    QString iccid;                  // SIM卡ICCID (IntegratedCircuitCardIdentity)
    QString carrierBundleInfoVersion; // 运营商包版本
    
    // ===== 电池信息 =====
    qint32 batteryCurrentCapacity;  // 当前电量百分比 (0-100)
    bool batteryIsCharging;         // 是否正在充电
    QString externalChargeCapable;  // 外部充电能力
    bool fullyCharged;              // 是否已充满
    
    // ===== 设备状态 =====
    QString activationState;        // 激活状态 (ActivationState: Activated)
    bool passwordProtected;         // 是否设置密码 (PasswordProtected)
    bool deviceSupportsLockdown;    // 是否支持锁定协议
    bool hostAttached;              // 主机是否已连接
    bool hasSimLockedStatus;        // SIM锁状态
    bool iTunesHasConnected;        // iTunes是否曾连接
    
    // ===== 时间信息 =====
    QString timeZone;               // 时区 (TimeZone: Asia/Shanghai)
    qint64 timeIntervalSince1970;   // 设备时间戳
    QDateTime deviceTime;           // 设备当前时间
    
    // ===== 安全信息 =====
    bool trustedHostAttached;       // 信任主机是否已连接
    QString pairingState;           // 配对状态
    
    // ===== 其他信息 =====
    QString mlbSerialNumber;        // MLB序列号 (MLBSerialNumber)
    QString protocolVersion;        // 协议版本 (ProtocolVersion)
    bool supportsWirelessSync;      // 是否支持无线同步
    QString wirelessBuddyID;        // 无线配对ID
    
    /**
     * @brief 将设备信息转换为 QVariantMap
     * @return 包含所有设备信息的 QVariantMap
     */
    QVariantMap toMap() const;
    
    /**
     * @brief 将设备信息转换为可读的字符串
     * @return 格式化的设备信息字符串
     */
    QString toString() const;
    
    /**
     * @brief 获取格式化的存储容量字符串
     * @param bytes 字节数
     * @return 格式化的容量字符串 (如: "128.00 GB")
     */
    static QString formatCapacity(qint64 bytes);
    
    /**
     * @brief 获取设备的友好型号名称
     * @return 设备友好名称 (如: "iPhone 14 Pro")
     */
    QString getFriendlyModelName() const;
};

class DeviceInfoManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceInfoManager(QObject *parent = nullptr);
    
    /**
     * @brief 获取设备基本信息
     * @param udid 设备唯一标识符
     * @return DeviceInfo 结构体
     */
    DeviceInfo getDeviceInfo(const QString &udid);
    
    /**
     * @brief 获取设备详细信息（包含所有可用字段）
     * @param udid 设备唯一标识符
     * @return 包含所有设备信息的 QVariantMap
     */
    QVariantMap getDetailedInfo(const QString &udid);
    
    /**
     * @brief 获取电池信息
     * @param udid 设备唯一标识符
     * @return 包含电池信息的 QVariantMap
     */
    QVariantMap getBatteryInfo(const QString &udid);
    
    /**
     * @brief 获取存储使用情况
     * @param udid 设备唯一标识符
     * @return 包含存储信息的 QVariantMap
     */
    QVariantMap getDiskUsageInfo(const QString &udid);

private:
    // libimobiledevice辅助方法（动态加载模式）
    QString getStringValue(lockdownd_client_t lockdown, const char* domain, const char* key);
    qint64 getIntValue(lockdownd_client_t lockdown, const char* domain, const char* key);
    bool getBoolValue(lockdownd_client_t lockdown, const char* domain, const char* key);
    double getDoubleValue(lockdownd_client_t lockdown, const char* domain, const char* key);
};

#endif // DEVICEINFO_H