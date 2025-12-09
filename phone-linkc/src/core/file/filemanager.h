/**
 * @file filemanager.h
 * @brief 文件管理器头文件
 *
 * 提供 iOS 设备文件系统访问和管理功能。
 */

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>

/**
 * @brief 文件节点信息结构体
 */
struct FileNode {
    QString name;           ///< 文件/目录名称
    QString path;           ///< 完整路径
    qint64 size;            ///< 文件大小（字节）
    bool isDir;             ///< 是否为目录
    QDateTime modifiedTime; ///< 修改时间
    
    FileNode() : size(0), isDir(false) {}
};

/**
 * @brief 文件管理器类
 *
 * 使用 AFC 服务访问 iOS 设备上的文件系统。
 */
class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QObject *parent = nullptr);
    ~FileManager();

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
     * @brief 列出目录内容
     * @param path 目录路径
     * @return 文件节点列表
     */
    QVector<FileNode> listDirectory(const QString &path);

    /**
     * @brief 创建目录
     * @param path 目录路径
     * @return 是否成功
     */
    bool createDirectory(const QString &path);

    /**
     * @brief 删除文件或目录
     * @param path 文件/目录路径
     * @return 是否成功
     */
    bool deletePath(const QString &path);

    /**
     * @brief 重命名文件或目录
     * @param oldPath 原路径
     * @param newPath 新路径
     * @return 是否成功
     */
    bool renamePath(const QString &oldPath, const QString &newPath);
    
    /**
     * @brief 从设备读取文件
     * @param path 文件路径
     * @return 文件内容
     */
    QByteArray readFile(const QString &path);

    /**
     * @brief 写入文件到设备
     * @param path 文件路径
     * @param data 文件内容
     * @return 是否成功
     */
    bool writeFile(const QString &path, const QByteArray &data);

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

private:
    /**
     * @brief 初始化 AFC 客户端
     * @return 是否成功
     */
    bool initAfcClient();

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 获取文件信息
     * @param path 文件路径
     * @return 文件信息
     */
    FileNode getFileInfo(const QString &path);

    QString m_udid;                 ///< 当前设备 UDID
    bool m_connected;               ///< 连接状态
    QString m_lastError;            ///< 最后的错误信息
    
    // libimobiledevice 句柄
    void *m_device;                 ///< idevice_t
    void *m_lockdown;               ///< lockdownd_client_t
    void *m_afcClient;              ///< afc_client_t
};

#endif // FILEMANAGER_H