/**
 * @file appmanager.h
 * @brief 应用管理器头文件
 *
 * 提供 iOS 设备应用管理功能（安装、卸载、列表）。
 */

#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QIcon>

/**
 * @brief 应用信息结构体
 */
struct AppInfo {
    QString name;           ///< 应用名称 (CFBundleDisplayName)
    QString version;        ///< 应用版本 (CFBundleShortVersionString)
    QString bundleId;       ///< 包名 (CFBundleIdentifier)
    QString iconPath;       ///< 图标路径（如果有）
    QIcon icon;             ///< 应用图标
    QString type;           ///< 应用类型 (System, User)
    QString appSize;        ///< 应用大小 (格式化字符串)
    QString docSize;        ///< 文档大小 (格式化字符串)
    
    // 比较操作符，用于排序等
    bool operator<(const AppInfo& other) const {
        return name < other.name;
    }
};

/**
 * @brief 应用管理器类
 *
 * 使用 installation_proxy 服务访问 iOS 设备上的应用列表。
 */
class AppManager : public QObject
{
    Q_OBJECT

public:
    explicit AppManager(QObject *parent = nullptr);
    ~AppManager();

    /**
     * @brief 连接到设备
     * @param udid 设备 UDID
     * @return 是否成功连接
     */
    bool connectToDevice(const QString &udid);

    /**
     * @brief 断开设备连接
     */
    void disconnectFromDevice();

    /**
     * @brief 检查是否已连接
     * @return 是否已连接
     */
    bool isConnected() const { return m_connected; }

    /**
     * @brief 获取应用列表（同步方法）
     * @param includeSize 是否包含磁盘使用信息（默认 true）
     * @return 应用信息列表
     */
    QVector<AppInfo> listApps(bool includeSize = true);
    
    /**
     * @brief 异步获取应用列表（分阶段加载）
     *
     * 第一阶段：快速加载基本信息（不含磁盘使用），发出 appsLoaded 信号
     * 第二阶段：后台异步加载磁盘使用信息，发出 appSizeUpdated 信号
     */
    void listAppsAsync();
    
    /**
     * @brief 异步获取单个应用的磁盘使用信息
     * @param bundleId 应用的 Bundle ID
     */
    void getAppSizeAsync(const QString &bundleId);

    /**
     * @brief 安装应用
     * @param path ipa文件路径
     * @return 是否成功
     */
    bool installApp(const QString &path);

    /**
     * @brief 卸载应用
     * @param bundleId 应用包名
     * @return 是否成功
     */
    bool uninstallApp(const QString &bundleId);

    /**
     * @brief 获取最后的错误信息
     * @return 错误信息
     */
    QString lastError() const { return m_lastError; }

signals:
    /**
     * @brief 发生错误
     * @param error 错误信息
     */
    void errorOccurred(const QString &error);

    /**
     * @brief 进度更新
     * @param message 消息
     * @param percent 进度百分比 (0-100)
     */
    void progressUpdated(const QString &message, int percent);
    
    /**
     * @brief 应用列表加载完成（基本信息）
     * @param apps 应用信息列表
     */
    void appsLoaded(const QVector<AppInfo> &apps);
    
    /**
     * @brief 单个应用的磁盘使用信息更新
     * @param bundleId 应用的 Bundle ID
     * @param appSize 应用大小
     * @param docSize 文档大小
     */
    void appSizeUpdated(const QString &bundleId, const QString &appSize, const QString &docSize);

private:
    /**
     * @brief 初始化 instproxy 客户端
     * @return 是否成功
     */
    bool initClient();

    /**
     * @brief 清理资源
     */
    void cleanup();

    QString m_udid;                 ///< 当前设备 UDID
    bool m_connected;               ///< 连接状态
    QString m_lastError;            ///< 最后的错误信息
    
    // libimobiledevice 句柄
    void *m_device;                 ///< idevice_t
    void *m_lockdown;               ///< lockdownd_client_t
    void *m_instproxy;              ///< instproxy_client_t
};

#endif // APPMANAGER_H