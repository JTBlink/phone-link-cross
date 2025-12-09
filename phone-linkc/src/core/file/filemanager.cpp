/**
 * @file filemanager.cpp
 * @brief 文件管理器实现
 */

#include "filemanager.h"
#include "platform/libimobiledevice_dynamic.h"
#include <QDebug>
#include <QFileInfo>

FileManager::FileManager(QObject *parent)
    : QObject(parent)
    , m_connected(false)
    , m_device(nullptr)
    , m_lockdown(nullptr)
    , m_afcClient(nullptr)
{
}

FileManager::~FileManager()
{
    disconnect();
}

bool FileManager::connectToDevice(const QString &udid)
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
        loader.lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc-file") != LOCKDOWN_E_SUCCESS) {
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
    qDebug() << "FileManager: 成功连接到设备" << udid;
    return true;
}

void FileManager::disconnectFromDevice()
{
    cleanup();
    m_connected = false;
    m_udid.clear();
}

bool FileManager::initAfcClient()
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
    qDebug() << "FileManager: AFC 客户端创建成功";
    return true;
}

void FileManager::cleanup()
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

QVector<FileNode> FileManager::listDirectory(const QString &path)
{
    QVector<FileNode> nodes;
    
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return nodes;
    }
    
    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_read_directory || !loader.afc_dictionary_free) {
        m_lastError = "AFC 目录读取函数不可用";
        return nodes;
    }
    
    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    
    // 读取目录
    char **directory_info = nullptr;
    // AFC路径必须以 / 开头
    QString safePath = path;
    if (!safePath.startsWith("/")) {
        safePath = "/" + safePath;
    }

    afc_error_t ret = loader.afc_read_directory(afcClient, safePath.toUtf8().constData(), &directory_info);
    
    if (ret != AFC_E_SUCCESS || !directory_info) {
        m_lastError = QString("无法读取目录，错误码: %1").arg(ret);
        return nodes;
    }
    
    // 遍历目录项
    for (int i = 0; directory_info[i]; i++) {
        QString name = QString::fromUtf8(directory_info[i]);
        
        // 跳过 . 和 ..
        if (name == "." || name == "..") {
            continue;
        }
        
        QString fullPath = QString("%1/%2").arg(safePath == "/" ? "" : safePath, name);
        FileNode node = getFileInfo(fullPath);
        node.name = name;
        nodes.append(node);
    }
    
    loader.afc_dictionary_free(directory_info);
    return nodes;
}

FileNode FileManager::getFileInfo(const QString &path)
{
    FileNode info;
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
        } else if (key == "st_ifmt") {
            if (value == "S_IFDIR") {
                info.isDir = true;
            }
        }
    }
    
    loader.afc_dictionary_free(file_info);
    
    return info;
}

bool FileManager::createDirectory(const QString &path)
{
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return false;
    }

    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_make_directory) {
        m_lastError = "AFC 创建目录函数不可用";
        return false;
    }

    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    QString safePath = path.startsWith("/") ? path : "/" + path;

    afc_error_t ret = loader.afc_make_directory(afcClient, safePath.toUtf8().constData());
    if (ret != AFC_E_SUCCESS) {
        m_lastError = QString("无法创建目录: %1 (错误码: %2)").arg(safePath).arg(ret);
        return false;
    }
    return true;
}

bool FileManager::deletePath(const QString &path)
{
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return false;
    }

    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_remove_path) {
        m_lastError = "AFC 删除函数不可用";
        return false;
    }

    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    QString safePath = path.startsWith("/") ? path : "/" + path;

    // afc_remove_path 可以删除文件或空目录
    // 如果是非空目录，通常需要递归删除，这里暂只支持单个文件或空目录
    afc_error_t ret = loader.afc_remove_path(afcClient, safePath.toUtf8().constData());
    if (ret != AFC_E_SUCCESS) {
        m_lastError = QString("无法删除: %1 (错误码: %2)").arg(safePath).arg(ret);
        return false;
    }
    return true;
}

bool FileManager::renamePath(const QString &oldPath, const QString &newPath)
{
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return false;
    }

    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_rename_path) {
        m_lastError = "AFC 重命名函数不可用";
        return false;
    }

    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    QString safeOld = oldPath.startsWith("/") ? oldPath : "/" + oldPath;
    QString safeNew = newPath.startsWith("/") ? newPath : "/" + newPath;

    afc_error_t ret = loader.afc_rename_path(afcClient, safeOld.toUtf8().constData(), safeNew.toUtf8().constData());
    if (ret != AFC_E_SUCCESS) {
        m_lastError = QString("无法重命名 (错误码: %1)").arg(ret);
        return false;
    }
    return true;
}

QByteArray FileManager::readFile(const QString &path)
{
    QByteArray data;
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return data;
    }

    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_file_open || !loader.afc_file_read || !loader.afc_file_close) {
        m_lastError = "AFC 文件读写函数不可用";
        return data;
    }

    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    QString safePath = path.startsWith("/") ? path : "/" + path;

    uint64_t handle = 0;
    afc_error_t ret = loader.afc_file_open(afcClient, safePath.toUtf8().constData(), AFC_FOPEN_RDONLY, &handle);
    if (ret != AFC_E_SUCCESS) {
        m_lastError = QString("无法打开文件: %1").arg(safePath);
        return data;
    }

    // 获取文件大小
    FileNode node = getFileInfo(safePath);
    qint64 fileSize = node.size;
    
    // 分块读取
    const uint32_t BUFFER_SIZE = 65536; // 64KB
    char buffer[BUFFER_SIZE];
    uint32_t bytesRead = 0;
    qint64 totalRead = 0;

    while (totalRead < fileSize) {
        uint32_t toRead = qMin(static_cast<qint64>(BUFFER_SIZE), fileSize - totalRead);
        ret = loader.afc_file_read(afcClient, handle, buffer, toRead, &bytesRead);
        if (ret != AFC_E_SUCCESS || bytesRead == 0) {
            break;
        }
        data.append(buffer, bytesRead);
        totalRead += bytesRead;
    }

    loader.afc_file_close(afcClient, handle);
    return data;
}

bool FileManager::writeFile(const QString &path, const QByteArray &data)
{
    if (!m_connected || !m_afcClient) {
        m_lastError = "未连接到设备";
        return false;
    }

    LibimobiledeviceDynamic& loader = LibimobiledeviceDynamic::instance();
    if (!loader.afc_file_open || !loader.afc_file_write || !loader.afc_file_close) {
        m_lastError = "AFC 文件读写函数不可用";
        return false;
    }

    afc_client_t afcClient = static_cast<afc_client_t>(m_afcClient);
    QString safePath = path.startsWith("/") ? path : "/" + path;

    uint64_t handle = 0;
    afc_error_t ret = loader.afc_file_open(afcClient, safePath.toUtf8().constData(), AFC_FOPEN_WR, &handle);
    if (ret != AFC_E_SUCCESS) {
        m_lastError = QString("无法打开文件以写入: %1").arg(safePath);
        return false;
    }

    uint32_t bytesWritten = 0;
    ret = loader.afc_file_write(afcClient, handle, data.constData(), data.size(), &bytesWritten);
    
    loader.afc_file_close(afcClient, handle);

    if (ret != AFC_E_SUCCESS || bytesWritten != (uint32_t)data.size()) {
        m_lastError = QString("写入文件失败 (错误码: %1)").arg(ret);
        return false;
    }

    return true;
}