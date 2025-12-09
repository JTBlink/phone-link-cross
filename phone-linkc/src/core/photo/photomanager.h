/**
 * @file photomanager.h
 * @brief 照片管理器头文件
 *
 * 提供 iOS 设备照片访问和管理功能。
 */

#ifndef PHOTOMANAGER_H
#define PHOTOMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVector>

/**
 * @brief 照片信息结构体
 */
struct PhotoInfo {
    QString path;           ///< 文件路径（设备上的完整路径）
    QString name;           ///< 文件名
    qint64 size;            ///< 文件大小（字节）
    QDateTime modifiedTime; ///< 修改时间
    bool isVideo;           ///< 是否为视频文件
    
    PhotoInfo() : size(0), isVideo(false) {}
};

/**
 * @brief 相册信息结构体
 */
struct AlbumInfo {
    QString path;           ///< 相册路径
    QString name;           ///< 相册名称
    int photoCount;         ///< 照片数量
    
    AlbumInfo() : photoCount(0) {}
};

/**
 * @brief 照片管理器类
 *
 * 使用 AFC 服务访问 iOS 设备上的照片。
 * iOS 设备照片存储在 /DCIM 目录下。
 */
class PhotoManager : public QObject
{
    Q_OBJECT

public:
    explicit PhotoManager(QObject *parent = nullptr);
    ~PhotoManager();

    /**
     * @brief 连接到设备
     * @param udid 设备 UDID
     * @return 是否成功连接
     */
    bool connectToDevice(const QString &udid);

    /**
     * @brief 断开设备连接
     */
    void disconnect();

    /**
     * @brief 检查是否已连接
     * @return 是否已连接
     */
    bool isConnected() const { return m_connected; }

    /**
     * @brief 获取相册列表
     * @return 相册信息列表
     */
    QVector<AlbumInfo> getAlbums();

    /**
     * @brief 获取指定相册中的照片列表
     * @param albumPath 相册路径，空字符串表示所有照片
     * @return 照片信息列表
     */
    QVector<PhotoInfo> getPhotos(const QString &albumPath = QString());

    /**
     * @brief 获取所有照片（递归扫描 DCIM 目录）
     * @return 照片信息列表
     */
    QVector<PhotoInfo> getAllPhotos();

    /**
     * @brief 读取照片缩略图数据
     * @param photoPath 照片路径
     * @param maxSize 最大读取大小（字节），0 表示读取全部
     * @return 照片数据
     */
    QByteArray readPhotoData(const QString &photoPath, qint64 maxSize = 0);

    /**
     * @brief 获取最后的错误信息
     * @return 错误信息
     */
    QString lastError() const { return m_lastError; }

signals:
    /**
     * @brief 扫描进度信号
     * @param current 当前进度
     * @param total 总数
     */
    void scanProgress(int current, int total);

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
     * @brief 递归扫描目录获取照片
     * @param path 目录路径
     * @param photos 照片列表（输出）
     */
    void scanDirectory(const QString &path, QVector<PhotoInfo> &photos);

    /**
     * @brief 获取文件信息
     * @param path 文件路径
     * @return 文件信息
     */
    PhotoInfo getFileInfo(const QString &path);

    /**
     * @brief 判断是否为图片或视频文件
     * @param filename 文件名
     * @param isVideo 是否为视频（输出）
     * @return 是否为媒体文件
     */
    static bool isMediaFile(const QString &filename, bool &isVideo);

    QString m_udid;                 ///< 当前设备 UDID
    bool m_connected;               ///< 连接状态
    QString m_lastError;            ///< 最后的错误信息
    
    // libimobiledevice 句柄
    void *m_device;                 ///< idevice_t
    void *m_lockdown;               ///< lockdownd_client_t
    void *m_afcClient;              ///< afc_client_t
};

#endif // PHOTOMANAGER_H