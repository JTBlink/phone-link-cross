#ifndef CONTACTMANAGER_H
#define CONTACTMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/mobilesync.h>
#include <plist/plist.h>

/**
 * @brief 联系人数据结构
 */
struct Contact {
    QString id;              ///< 联系人唯一标识
    QString firstName;       ///< 名
    QString lastName;        ///< 姓
    QString displayName;     ///< 显示名称
    QString organization;    ///< 组织/公司
    QStringList phoneNumbers; ///< 电话号码列表
    QStringList emails;      ///< 邮箱列表
    QString note;            ///< 备注
    
    /**
     * @brief 获取完整姓名
     */
    QString fullName() const {
        if (!lastName.isEmpty() && !firstName.isEmpty()) {
            return lastName + firstName;
        } else if (!lastName.isEmpty()) {
            return lastName;
        } else if (!firstName.isEmpty()) {
            return firstName;
        } else if (!displayName.isEmpty()) {
            return displayName;
        }
        return "未命名联系人";
    }
};

/**
 * @brief 通讯录管理器类
 * 
 * 负责与 iOS 设备同步通讯录数据，使用 mobilesync 协议
 * 通过动态加载的 libimobiledevice DLL 函数实现
 */
class ContactManager : public QObject
{
    Q_OBJECT

public:
    explicit ContactManager(QObject *parent = nullptr);
    ~ContactManager();

    /**
     * @brief 连接到设备
     * @param udid 设备 UDID
     * @return 是否连接成功
     */
    bool connectToDevice(const QString &udid);
    
    /**
     * @brief 断开设备连接
     */
    void disconnectFromDevice();
    
    /**
     * @brief 是否已连接到设备
     */
    bool isConnected() const { return m_isConnected; }
    
    /**
     * @brief 同步通讯录（从设备获取所有联系人）
     * @return 是否同步成功
     */
    bool syncContacts();
    
    /**
     * @brief 获取所有联系人
     */
    QVector<Contact> getContacts() const { return m_contacts; }
    
    /**
     * @brief 搜索联系人
     * @param keyword 关键词（搜索姓名、电话、邮箱）
     */
    QVector<Contact> searchContacts(const QString &keyword) const;
    
    /**
     * @brief 导出联系人到 vCard 格式
     * @param filePath 导出文件路径
     * @return 是否导出成功
     */
    bool exportToVCard(const QString &filePath) const;

signals:
    /**
     * @brief 同步进度信号
     * @param current 当前进度
     * @param total 总数
     */
    void syncProgress(int current, int total);
    
    /**
     * @brief 同步完成信号
     * @param success 是否成功
     * @param message 消息
     */
    void syncCompleted(bool success, const QString &message);
    
    /**
     * @brief 错误信号
     */
    void errorOccurred(const QString &error);

private:
    bool m_isConnected;
    QString m_currentUdid;
    QVector<Contact> m_contacts;
    
    // libimobiledevice 句柄
    idevice_t m_device;
    
    // 保存的锚点（用于增量同步）
    QString m_deviceAnchor;
    QString m_computerAnchor;
    
    /**
     * @brief 清理资源
     */
    void cleanup();
    
    /**
     * @brief 使用 mobilesync 协议同步通讯录
     * @return 是否同步成功
     */
    bool syncContactsViaMobileSync();
    
    /**
     * @brief 解析联系人实体（从 mobilesync plist 数据）
     * @param entities 实体 plist
     * @return 解析的联系人列表
     */
    QVector<Contact> parseContactEntities(plist_t entities);
    
    /**
     * @brief 从 plist 字典解析单个联系人
     * @param contactDict 联系人字典
     * @return 解析的联系人
     */
    Contact parseContactFromPlist(plist_t contactDict) const;
    
    /**
     * @brief 获取 plist 字典中的字符串值
     * @param dict plist 字典
     * @param key 键名
     * @return 字符串值
     */
    QString getStringValue(plist_t dict, const char *key) const;
    
    /**
     * @brief 从 plist 数组提取字符串列表
     * @param array plist 数组
     * @return 字符串列表
     */
    QStringList extractStringArray(plist_t array) const;
    
    /**
     * @brief 生成新的计算机锚点
     * @return 锚点字符串
     */
    QString generateComputerAnchor() const;
    
    /**
     * @brief 加载保存的锚点
     */
    void loadAnchors();
    
    /**
     * @brief 保存锚点到本地
     */
    void saveAnchors();
};

#endif // CONTACTMANAGER_H