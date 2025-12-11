#ifndef CONTACTMANAGER_H
#define CONTACTMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QSqlDatabase>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/mobilebackup2.h>
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
 * 负责与 iOS 设备同步通讯录数据，使用 mobilebackup2 协议
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
    
    /**
     * @brief 清理资源
     */
    void cleanup();
    
    /**
     * @brief 执行备份并获取通讯录数据库（使用 mobilebackup2 API）
     * @param backupPath 备份保存路径
     * @return 数据库文件路径，失败返回空字符串
     */
    QString performBackupAndGetDB(const QString& backupPath);

    /**
     * @brief 从 SQLite 数据库读取联系人
     * @param dbPath 数据库文件路径
     * @return 是否读取成功
     */
    bool readContactsFromDB(const QString& dbPath);
    
    /**
     * @brief 处理 DLMessageDownloadFiles 消息（设备请求从电脑下载文件）
     * @param client mobilebackup2 客户端
     * @param message 消息 plist
     * @param backupDir 备份目录
     * @return 状态码，0表示成功，-1表示失败
     */
    int handleDownloadFilesRequest(mobilebackup2_client_t client, plist_t message,
                                    const QString& backupDir);
    
    /**
     * @brief 处理 DLMessageUploadFiles 消息（设备上传文件到电脑，用于备份）
     * @param client mobilebackup2 客户端
     * @param message 消息 plist
     * @param backupDir 备份目录
     * @param addressBookPath [out] 找到的通讯录数据库路径
     * @return 状态码，0表示成功，-1表示失败
     */
    int handleUploadFiles(mobilebackup2_client_t client, plist_t message,
                          const QString& backupDir, QString& addressBookPath);
    
    /**
     * @brief 发送文件给设备（用于 DLMessageDownloadFiles 响应）
     * @param client mobilebackup2 客户端
     * @param filePath 要发送的文件路径
     * @return 是否发送成功
     */
    bool sendFileToDevice(mobilebackup2_client_t client, const QString& filePath);
    
    /**
     * @brief 从设备接收文件数据（用于 DLMessageUploadFiles）
     * @param client mobilebackup2 客户端
     * @param targetPath 目标文件路径
     * @param fileInfo 文件信息 plist
     * @return 接收的字节数
     */
    uint64_t receiveFileFromDevice(mobilebackup2_client_t client, const QString& targetPath, plist_t fileInfo);
    
    /**
     * @brief 从设备接收文件数据（简化版本）
     * @param client mobilebackup2 客户端
     * @param targetPath 目标文件路径
     * @return 是否接收成功
     */
    bool receiveFileData(mobilebackup2_client_t client, const QString& targetPath);
    
    /**
     * @brief 处理 mobilebackup2 处理消息
     * @param message 消息 plist
     * @param backupComplete [out] 备份是否完成
     */
    void handleProcessMessage(plist_t message, bool& backupComplete);
    
    /**
     * @brief 在备份目录中查找通讯录数据库
     * @param backupDir 备份目录
     * @return 数据库文件路径，未找到返回空字符串
     */
    QString findAddressBookInBackup(const QString& backupDir);
    
    // 保留的辅助函数（用于 plist 解析）
    Contact parseContact(plist_t plist) const;
    QString getStringValue(plist_t dict, const char *key) const;
    QStringList extractStringArray(plist_t array) const;
};

#endif // CONTACTMANAGER_H