/**
 * @file photomanager.cpp
 * @brief 照片管理器实现
 */

#include "photomanager.h"
#include "platform/libimobiledevice_dynamic.h"
#include <QDebug>
#include <QFileInfo>

// DCIM 目录路径 - iOS 设备照片存储位置
static const char* DCIM_PATH = "/DCIM";

PhotoManager::PhotoManager(QObject *parent)
    : QObject(parent)
    , m_connected(false)
    , m_device(nullptr)
    , m_lockdown(nullptr)
    , m_afcClient(nullptr)
{
}

PhotoManager::~PhotoManager()
{
    disconnect();
}

bool PhotoManager::connectToDevice(const QString &udid)
{
    if (m_connected) {
        disconnect();
    }
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.isInitialized()) {
        m_lastError = "libimobiledevice 动态库未初始化";
        emit errorOccurred(m_lastError);
        return false;
    }
    
    m_udid = udid;
    
    // 创建设备连接
    idevice_t device = nullptr;
    if (!loader.idevice_new || loader.idevice_new(&device, udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
        m_lastError = QString("无法连接到设备: %1").arg(udid);
        emit errorOccurred(m_lastError);
        return false;
    }
    m_device = device;
    
    // 创建 lockdown 客户端
    lockdownd_client_t lockdown = nullptr;
    if (!loader.lockdownd_client_new_with_handshake || 
        loader.lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc-photo") != LOCKDOWN_E_SUCCESS) {
        m_lastError = "无法创建 lockdown 客户端";
        emit errorOccurred(m_lastError);
        cleanup();
        return false;
    }
    m_lockdown = lockdown;
    
    // 初始化 AFC 客户端
    if (!initAfcClient()) {
        cleanup();
        return false;
    }
    
    m_connected = true;
    qDebug() << "PhotoManager: 成功连接到设备" << udid;
    return true;
}

void PhotoManager::disconnect()
{
    cleanup();
    m_connected = false;
    m_udid.clear();
}

bool PhotoManager::initAfcClient()
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    
    if (!loader.lockdownd_start_service || !loader.afc_client_new) {
        m_lastError = "AFC 服务函数不可用";
        emit errorOccurred(m_lastError);
        return false;
    }
    
    lockdownd_client_t lockdown = static_cast<lockdownd_client_t>(m_lockdown);
    idevice_t device = static_cast<idevice_t>(m_device);
    
    // 启动 AFC 服务
    lockdownd_service_descriptor_t service = nullptr;
    lockdownd_error_t lockdown_ret = loader.lockdownd_start_service(lockdown, AFC_SERVICE_NAME, &service);
    if (lockdown_ret != LOCKDOWN_E_SUCCESS || !service) {
        m_lastError = QString("无法启动 AFC 服务，错误码: %1").arg(lockdown_ret);
        emit errorOccurred(m_lastError);
        return false;
    }
    
    // 创建 AFC 客户端
    afc_client_t afcClient = nullptr;
    afc_error_t afc_ret = loader.afc_client_new(device, service, &afcClient);
    
    // 释放服务描述符
    if (loader.lockdownd_service_descriptor_free && service) {
        loader.lockdownd_service_descriptor_free(service);
    }
    
    if (afc_ret != AFC_E_SUCCESS || !afcClient) {
        m_lastError = QString("无法创建 AFC 客户端，错误码: %1").arg(afc_ret);
        emit errorOccurred(m_lastError);
        return false;
    }
    
    m_afcClient = afcClient;
    qDebug() << "PhotoManager: AFC 客户端创建成功";
    return true;
}

void PhotoManager::cleanup()
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    
    if (m_afcClient && loader.afc_client_free) {
        loader.afc_client_free(static_cast<afc_client_t>(m_afcClient));
        m_afcClient = nullptr;
    }
    
    if (m_lockdown && loader.lockdownd_client_free) {
        loader.lockdownd_client_free(static_cast<lockdownd_client_t>(m_lockdown));
        m_lockdown = nullptr;
    }
    
    if (m_device && loader.idevice_free) {
        loader.idevice_free(static_cast<idevice_t>(m_device));
        m_device = nullptr;
    }
}

QVector<AlbumInfo> PhotoManager::getAlbums()
{
    QVector<AlbumInfo> albums;
    
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return albums;
    }
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_read_directory || !loader.afc_dictionary_free) {
        m_lastError = "AFC 目录读取函数不可用";
        return albums;
    }
    
    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    
    // 读取 DCIM 目录
    char **directory_info = nullptr;
    afc_error_t ret = loader.afc_read_directory(afcClient, DCIM_PATH, &directory_info);
    
    if (ret != AFC_E_SUCCESS || !directory_info) {
        m_lastError = QString("无法读取 DCIM 目录，错误码: %1").arg(ret);
        return albums;
    }
    
    // 遍历目录项
    for (int i = 0; directory_info[i]; i++) {
        QString name = QString::fromUtf8(directory_info[i]);
        
        // 跳过 . 和 ..
        if (name == "." || name == "..") {
            continue;
        }
        
        // 检查是否是目录（相册通常是 100APPLE, 101APPLE 等格式）
        // 注：iOS 系统相册的中文名称存储在 PhotoLibrary 数据库中，
        // 通过 AFC 服务只能访问 DCIM 目录下的物理文件夹结构，
        // 无法获取系统相册的元数据（如中文名称、智能相册等）
        QString albumPath = QString("%1/%2").arg(DCIM_PATH, name);
        
        AlbumInfo album;
        album.path = albumPath;
        album.name = name;  // 使用文件夹原始名称
        
        // 获取相册中的照片数量
        char **album_contents = nullptr;
        if (loader.afc_read_directory(afcClient, albumPath.toUtf8().constData(), &album_contents) == AFC_E_SUCCESS) {
            int count = 0;
            for (int j = 0; album_contents[j]; j++) {
                QString fileName = QString::fromUtf8(album_contents[j]);
                if (fileName != "." && fileName != "..") {
                    bool isVideo;
                    if (isMediaFile(fileName, isVideo)) {
                        count++;
                    }
                }
            }
            album.photoCount = count;
            loader.afc_dictionary_free(album_contents);
        }
        
        if (album.photoCount > 0) {
            albums.append(album);
            qDebug() << "PhotoManager: 发现相册:" << album.name << "路径:" << album.path << "照片数:" << album.photoCount;
        }
    }
    
    loader.afc_dictionary_free(directory_info);
    
    qDebug() << "PhotoManager: 找到" << albums.size() << "个相册";
    return albums;
}

QVector<PhotoInfo> PhotoManager::getPhotos(const QString &albumPath)
{
    QVector<PhotoInfo> photos;
    
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return photos;
    }
    
    QString path = albumPath.isEmpty() ? QString(DCIM_PATH) : albumPath;
    scanDirectory(path, photos);
    
    return photos;
}

QVector<PhotoInfo> PhotoManager::getAllPhotos()
{
    QVector<PhotoInfo> photos;
    
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return photos;
    }
    
    // 递归扫描整个 DCIM 目录
    scanDirectory(DCIM_PATH, photos);
    
    qDebug() << "PhotoManager: 总共找到" << photos.size() << "个媒体文件";
    return photos;
}

void PhotoManager::scanDirectory(const QString &path, QVector<PhotoInfo> &photos)
{
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_read_directory || !loader.afc_dictionary_free) {
        return;
    }
    
    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    
    // 读取目录内容
    char **directory_info = nullptr;
    afc_error_t ret = loader.afc_read_directory(afcClient, path.toUtf8().constData(), &directory_info);
    
    if (ret != AFC_E_SUCCESS || !directory_info) {
        qDebug() << "PhotoManager: 无法读取目录" << path;
        return;
    }
    
    // 遍历目录项
    for (int i = 0; directory_info[i]; i++) {
        QString name = QString::fromUtf8(directory_info[i]);
        
        // 跳过 . 和 ..
        if (name == "." || name == "..") {
            continue;
        }
        
        QString fullPath = QString("%1/%2").arg(path, name);
        
        // 检查是否为媒体文件
        bool isVideo;
        if (isMediaFile(name, isVideo)) {
            PhotoInfo info = getFileInfo(fullPath);
            info.name = name;
            info.isVideo = isVideo;
            photos.append(info);
        } else {
            // 可能是子目录，递归扫描
            // 检查是否为目录
            char **file_info = nullptr;
            if (loader.afc_get_file_info && 
                loader.afc_get_file_info(afcClient, fullPath.toUtf8().constData(), &file_info) == AFC_E_SUCCESS) {
                
                bool isDir = false;
                for (int j = 0; file_info[j]; j += 2) {
                    if (QString::fromUtf8(file_info[j]) == "st_ifmt" &&
                        QString::fromUtf8(file_info[j + 1]) == "S_IFDIR") {
                        isDir = true;
                        break;
                    }
                }
                loader.afc_dictionary_free(file_info);
                
                if (isDir) {
                    scanDirectory(fullPath, photos);
                }
            }
        }
    }
    
    loader.afc_dictionary_free(directory_info);
}

PhotoInfo PhotoManager::getFileInfo(const QString &path)
{
    PhotoInfo info;
    info.path = path;
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_get_file_info || !loader.afc_dictionary_free) {
        return info;
    }
    
    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    
    char **file_info = nullptr;
    if (loader.afc_get_file_info(afcClient, path.toUtf8().constData(), &file_info) != AFC_E_SUCCESS) {
        return info;
    }
    
    // 解析文件信息
    for (int i = 0; file_info[i]; i += 2) {
        QString key = QString::fromUtf8(file_info[i]);
        QString value = QString::fromUtf8(file_info[i + 1]);
        
        if (key == "st_size") {
            info.size = value.toLongLong();
        } else if (key == "st_mtime") {
            // 时间戳（纳秒）
            qint64 timestamp = value.toLongLong() / 1000000000LL;
            info.modifiedTime = QDateTime::fromSecsSinceEpoch(timestamp);
        }
    }
    
    loader.afc_dictionary_free(file_info);
    
    return info;
}

QByteArray PhotoManager::readPhotoData(const QString &photoPath, qint64 maxSize)
{
    QByteArray data;
    
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return data;
    }
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_file_open || !loader.afc_file_read || !loader.afc_file_close) {
        m_lastError = "AFC 文件操作函数不可用";
        return data;
    }
    
    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    
    // 打开文件
    uint64_t handle = 0;
    afc_error_t ret = loader.afc_file_open(afcClient, photoPath.toUtf8().constData(), AFC_FOPEN_RDONLY, &handle);
    if (ret != AFC_E_SUCCESS) {
        m_lastError = QString("无法打开文件: %1").arg(photoPath);
        return data;
    }
    
    // 获取文件大小
    PhotoInfo fileInfo = getFileInfo(photoPath);
    qint64 readSize = fileInfo.size;
    if (maxSize > 0 && maxSize < readSize) {
        readSize = maxSize;
    }
    
    // 读取文件内容
    const uint32_t BUFFER_SIZE = 65536; // 64KB 缓冲区
    char buffer[BUFFER_SIZE];
    uint32_t bytesRead = 0;
    qint64 totalRead = 0;
    
    while (totalRead < readSize) {
        uint32_t toRead = qMin(static_cast<qint64>(BUFFER_SIZE), readSize - totalRead);
        ret = loader.afc_file_read(afcClient, handle, buffer, toRead, &bytesRead);
        if (ret != AFC_E_SUCCESS || bytesRead == 0) {
            break;
        }
        data.append(buffer, bytesRead);
        totalRead += bytesRead;
    }
    
    // 关闭文件
    loader.afc_file_close(afcClient, handle);
    
    return data;
}

bool PhotoManager::isMediaFile(const QString &filename, bool &isVideo)
{
    // 获取文件扩展名
    QString ext = QFileInfo(filename).suffix().toLower();
    
    // 图片格式
    static const QStringList imageExtensions = {
        "jpg", "jpeg", "png", "gif", "heic", "heif", "tiff", "tif", "bmp", "raw", "cr2", "nef", "arw", "dng"
    };
    
    // 视频格式
    static const QStringList videoExtensions = {
        "mov", "mp4", "m4v", "avi", "3gp", "mkv"
    };
    
    if (imageExtensions.contains(ext)) {
        isVideo = false;
        return true;
    }
    
    if (videoExtensions.contains(ext)) {
        isVideo = true;
        return true;
    }
    
    return false;
}