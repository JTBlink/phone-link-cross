// contactmanager.cpp  — 修改版（按方案C）
// 主要修复：Backup 请求 options 补齐；Status.plist 生成改为触发完整备份；保持协议的长度帧发送

#include "contactmanager.h"
#include "platform/libimobiledevice_dynamic.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDataStream>
#include <QDateTime>
#include <QUuid>

// 支持的协议版本
static const int MB2_VERSION_COUNT = 2;

ContactManager::ContactManager(QObject *parent)
    : QObject(parent)
    , m_isConnected(false)
    , m_device(nullptr)
{
}

ContactManager::~ContactManager()
{
    cleanup();
}

bool ContactManager::connectToDevice(const QString &udid)
{
    qDebug() << "ContactManager: 正在连接设备..." << udid;
    if (m_isConnected) {
        qDebug() << "ContactManager: 断开现有连接";
        disconnectFromDevice();
    }
    
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    if (!lib.isInitialized()) {
        qCritical() << "ContactManager: libimobiledevice 未初始化";
        emit errorOccurred("libimobiledevice 未初始化");
        return false;
    }
    
    // 连接设备
    qDebug() << "ContactManager: 创建设备句柄...";
    idevice_error_t ret = lib.idevice_new(&m_device, udid.toUtf8().constData());
    if (ret != IDEVICE_E_SUCCESS) {
        qWarning() << "ContactManager: idevice_new 失败，错误代码:" << ret;
        emit errorOccurred("无法连接到设备");
        return false;
    }
    
    m_currentUdid = udid;
    m_isConnected = true;
    
    qDebug() << "ContactManager: 已成功连接到设备" << udid;
    return true;
}

void ContactManager::disconnectFromDevice()
{
    cleanup();
    m_isConnected = false;
    m_currentUdid.clear();
    m_contacts.clear();
    qDebug() << "ContactManager: 已断开设备连接";
}

bool ContactManager::syncContacts()
{
    if (!m_isConnected) {
        emit errorOccurred("设备未连接");
        return false;
    }

    // 使用持久化备份目录（与 idevicebackup2 兼容）
    // 这样 Status.plist 等备份状态文件可以被保留，支持增量备份
    QString backupRoot = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/iOSBackups";
    
    // 确保备份目录存在
    QDir backupDir(backupRoot);
    if (!backupDir.exists()) {
        if (!backupDir.mkpath(".")) {
            emit errorOccurred("无法创建备份目录");
            return false;
        }
    }
    
    qDebug() << "ContactManager: 准备备份到持久化目录:" << backupRoot;

    emit syncProgress(0, 100);
    qDebug() << "ContactManager: 开始执行备份，这可能需要一些时间...";

    // 执行备份并获取数据库路径
    QString dbPath = performBackupAndGetDB(backupRoot);
    if (dbPath.isEmpty()) {
        // 错误信息已经在 performBackupAndGetDB 中发出
        return false;
    }

    emit syncProgress(50, 100);
    qDebug() << "ContactManager: 数据库已提取:" << dbPath << "，开始读取联系人...";

    // 从数据库读取联系人
    if (!readContactsFromDB(dbPath)) {
        emit errorOccurred("读取通讯录数据库失败");
        return false;
    }

    emit syncProgress(100, 100);
    
    int totalContacts = m_contacts.size();
    QString message = QString("成功同步 %1 个联系人").arg(totalContacts);
    qDebug() << "ContactManager:" << message;
    emit syncCompleted(true, message);
    
    return true;
}

QString ContactManager::performBackupAndGetDB(const QString& backupPath)
{
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    // 检查 mobilebackup2 函数是否可用
    if (!lib.mobilebackup2_client_start_service || 
        !lib.mobilebackup2_version_exchange ||
        !lib.mobilebackup2_send_request ||
        !lib.mobilebackup2_receive_message ||
        !lib.mobilebackup2_send_status_response ||
        !lib.mobilebackup2_client_free) {
        qWarning() << "ContactManager: mobilebackup2 函数未加载";
        emit errorOccurred("mobilebackup2 服务不可用");
        return QString();
    }
    
    mobilebackup2_client_t mb2_client = nullptr;
    
    // 启动 mobilebackup2 服务
    qDebug() << "ContactManager: 启动 mobilebackup2 服务...";
    mobilebackup2_error_t mb2_err = lib.mobilebackup2_client_start_service(
        m_device, &mb2_client, "phone-linkc");
    
    if (mb2_err != MOBILEBACKUP2_E_SUCCESS || !mb2_client) {
        qWarning() << "ContactManager: 启动 mobilebackup2 服务失败，错误码:" << mb2_err;
        emit errorOccurred(QString("启动备份服务失败，错误码: %1").arg(mb2_err));
        return QString();
    }
    
    qDebug() << "ContactManager: mobilebackup2 服务已启动";
    
    // 版本握手
    double remote_version = 0.0;
    double versions[] = { 2.0, 2.1 };
    qDebug() << "ContactManager: 进行版本握手...";
    mb2_err = lib.mobilebackup2_version_exchange(mb2_client, versions, MB2_VERSION_COUNT, &remote_version);
    
    if (mb2_err != MOBILEBACKUP2_E_SUCCESS) {
        qWarning() << "ContactManager: 版本握手失败，错误码:" << mb2_err;
        lib.mobilebackup2_client_free(mb2_client);
        emit errorOccurred(QString("备份服务版本握手失败，错误码: %1").arg(mb2_err));
        return QString();
    }
    
    qDebug() << "ContactManager: 版本握手成功，远程版本:" << remote_version;
    
    // 创建设备备份目录
    QString deviceBackupDir = QDir(backupPath).filePath(m_currentUdid);
    QDir().mkpath(deviceBackupDir);
    
    // 构建备份选项（与 iTunes / idevicebackup2 保持兼容）
    plist_t options = lib.plist_new_dict();
    
    // Sources = [] 空数组，让设备自行决定备份范围（最兼容的方式）
    plist_t sources = lib.plist_new_array();
    lib.plist_dict_set_item(options, "Sources", sources);
    
    // 强制完整备份（确保设备会上传所有文件）
    lib.plist_dict_set_item(options, "ForceFullBackup", lib.plist_new_bool(1));
    
    // 禁用 Keychain 备份以避免权限问题
    lib.plist_dict_set_item(options, "BackupKeybag", lib.plist_new_bool(0));
    
    // 客户端版本标识
    lib.plist_dict_set_item(options, "ClientVersion", lib.plist_new_string("3.3"));
    
    qDebug() << "ContactManager: 配置备份选项 - Sources=[], ForceFullBackup=true, BackupKeybag=false, ClientVersion=3.3";
    
    // 发送备份请求
    qDebug() << "ContactManager: 发送备份请求...";
    mb2_err = lib.mobilebackup2_send_request(
        mb2_client,
        "Backup",
        m_currentUdid.toUtf8().constData(),
        NULL,  // source 通常为 NULL
        options
    );
    
    lib.plist_free(options);
    
    if (mb2_err != MOBILEBACKUP2_E_SUCCESS) {
        qWarning() << "ContactManager: 发送备份请求失败，错误码:" << mb2_err;
        lib.mobilebackup2_client_free(mb2_client);
        emit errorOccurred(QString("发送备份请求失败，错误码: %1").arg(mb2_err));
        return QString();
    }
    
    qDebug() << "ContactManager: 备份请求已发送，开始处理备份数据...";
    
    // 处理备份协议消息循环
    QString addressBookPath;
    bool backupComplete = false;
    int messageCount = 0;
    const int maxMessages = 10000; // 安全限制
    
    while (!backupComplete && messageCount < maxMessages) {
        plist_t msg_plist = nullptr;
        char* dlmessage = nullptr;
        
        mb2_err = lib.mobilebackup2_receive_message(mb2_client, &msg_plist, &dlmessage);
        
        if (mb2_err != MOBILEBACKUP2_E_SUCCESS) {
            if (msg_plist) {
                lib.plist_free(msg_plist);
                msg_plist = nullptr;
            }
            
            if (mb2_err == MOBILEBACKUP2_E_RECEIVE_TIMEOUT) {
                qDebug() << "ContactManager: 接收超时，继续等待...";
                continue;
            }
            qWarning() << "ContactManager: 接收消息失败，错误码:" << mb2_err;
            break;
        }
        
        messageCount++;
        
        if (dlmessage) {
            QString msgType = QString::fromUtf8(dlmessage);
            qDebug() << "ContactManager: 收到消息类型:" << msgType;
            
            if (msgType == "DLMessageDownloadFiles") {
                // 设备请求下载文件（从电脑到设备，用于恢复或检查状态）
                qDebug() << "ContactManager: 处理文件下载请求（设备请求从电脑获取文件）...";
                int result = handleDownloadFilesRequest(mb2_client, msg_plist, deviceBackupDir);
                Q_UNUSED(result); // DLMessageDownloadFiles 仅需发送原始文件数据，不需要额外的 status 响应
                
            } else if (msgType == "DLMessageUploadFiles") {
                // 设备上传文件给我们（用于备份）
                qDebug() << "ContactManager: 处理文件上传（设备发送文件给我们，用于备份）...";
                int result = handleUploadFiles(mb2_client, msg_plist, deviceBackupDir, addressBookPath);
                // 上传后返回状态
                lib.mobilebackup2_send_status_response(mb2_client, result, nullptr, nullptr);
                
            } else if (msgType == "DLMessageGetFreeDiskSpace") {
                // 返回足够的磁盘空间
                qDebug() << "ContactManager: 收到磁盘空间查询";
                plist_t space_dict = lib.plist_new_dict();
                // 返回 10GB 可用空间（使用 uint 类型）
                lib.plist_dict_set_item(space_dict, "FreeSpace", lib.plist_new_uint(10737418240ULL));
                lib.mobilebackup2_send_status_response(mb2_client, 0, nullptr, space_dict);
                lib.plist_free(space_dict);
                
            } else if (msgType == "DLMessageCreateDirectory") {
                // 创建目录请求
                qDebug() << "ContactManager: 收到创建目录请求";
                lib.mobilebackup2_send_status_response(mb2_client, 0, nullptr, nullptr);
                
            } else if (msgType == "DLMessageProcessMessage") {
                // 处理一般消息
                qDebug() << "ContactManager: 收到处理消息请求";
                handleProcessMessage(msg_plist, backupComplete);
                lib.mobilebackup2_send_status_response(mb2_client, 0, nullptr, nullptr);
                
            } else if (msgType == "DLMessageDisconnect") {
                qDebug() << "ContactManager: 收到断开连接消息";
                backupComplete = true;
                
            } else {
                qDebug() << "ContactManager: 未处理的消息类型:" << msgType;
                // 对未知消息发送成功响应以继续
                lib.mobilebackup2_send_status_response(mb2_client, 0, nullptr, nullptr);
            }
            
            // 注意: dlmessage 是由 libimobiledevice DLL 分配的内存。
            // 由于 Windows 跨模块 CRT 堆隔离问题，在 EXE 中调用 free(dlmessage) 会导致崩溃。
            // 因此这里不释放 dlmessage，会造成少量内存泄漏，但在实际使用场景下可接受。
        }
        
        if (msg_plist) {
            lib.plist_free(msg_plist);
            msg_plist = nullptr;
        }
        
        // 定期处理事件以防止 UI 卡死
        if (messageCount % 10 == 0) {
            QCoreApplication::processEvents();
        }
    }
    
    // 清理
    lib.mobilebackup2_client_free(mb2_client);
    
    qDebug() << "ContactManager: 备份过程完成，处理了" << messageCount << "条消息";
    
    // 如果没有通过协议获取到文件，尝试在备份目录中查找
    if (addressBookPath.isEmpty() || !QFile::exists(addressBookPath)) {
        addressBookPath = findAddressBookInBackup(deviceBackupDir);
    }
    
    if (addressBookPath.isEmpty() || !QFile::exists(addressBookPath)) {
        qWarning() << "ContactManager: 未找到通讯录数据库文件";
        emit errorOccurred("未在备份中找到通讯录数据库文件");
        return QString();
    }
    
    qDebug() << "ContactManager: 找到通讯录数据库:" << addressBookPath;
    return addressBookPath;
}

int ContactManager::handleUploadFiles(mobilebackup2_client_t client, plist_t message,
                                       const QString& backupDir, QString& addressBookPath)
{
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    qDebug() << "ContactManager: handleUploadFiles - 开始处理设备上传文件";
    
    if (!message) {
        qWarning() << "ContactManager: handleUploadFiles - message 为空";
        return -1;
    }
    
    // DLMessageUploadFiles 消息格式: [msgtype, files_array, ...]
    plist_type msgType = lib.plist_get_node_type(message);
    if (msgType != PLIST_ARRAY) {
        qWarning() << "ContactManager: handleUploadFiles - message 不是 ARRAY 类型";
        return -1;
    }
    
    uint32_t array_size = lib.plist_array_get_size(message);
    qDebug() << "ContactManager: handleUploadFiles - 数组大小:" << array_size;
    
    if (array_size < 2) {
        qWarning() << "ContactManager: handleUploadFiles - 数组大小不足";
        return -1;
    }
    
    // 获取文件列表（索引1）
    plist_t files_node = lib.plist_array_get_item(message, 1);
    if (!files_node) {
        qWarning() << "ContactManager: handleUploadFiles - 无法获取文件列表";
        return -1;
    }
    
    plist_type filesNodeType = lib.plist_get_node_type(files_node);
    qDebug() << "ContactManager: handleUploadFiles - 文件列表类型:" << filesNodeType;
    
    int fileCount = 0;
    
    // 处理文件列表
    if (filesNodeType == PLIST_ARRAY) {
        uint32_t files_count = lib.plist_array_get_size(files_node);
        qDebug() << "ContactManager: handleUploadFiles - 文件数量:" << files_count;
        
        for (uint32_t i = 0; i < files_count; i++) {
            plist_t fileItem = lib.plist_array_get_item(files_node, i);
            if (!fileItem) continue;
            
            plist_type fileItemType = lib.plist_get_node_type(fileItem);
            
            // 文件信息通常是字典类型
            if (fileItemType != PLIST_DICT) {
                qDebug() << "ContactManager: handleUploadFiles - 文件项[" << i << "]不是字典类型";
                continue;
            }
            
            // 获取文件路径
            plist_t pathNode = lib.plist_dict_get_item(fileItem, "BackupFileInfo");
            if (!pathNode) {
                pathNode = lib.plist_dict_get_item(fileItem, "Path");
            }
            if (!pathNode) {
                pathNode = lib.plist_dict_get_item(fileItem, "DLFileSource");
            }
            
            QString filePath;
            if (pathNode && lib.plist_get_node_type(pathNode) == PLIST_STRING) {
                const char* strVal = lib.plist_get_string_ptr(pathNode, nullptr);
                if (strVal) {
                    filePath = QString::fromUtf8(strVal);
                }
            }
            
            if (filePath.isEmpty()) {
                qDebug() << "ContactManager: handleUploadFiles - 文件项[" << i << "]没有路径信息";
                continue;
            }
            
            fileCount++;
            qDebug() << "ContactManager: handleUploadFiles - 接收文件 #" << fileCount << ":" << filePath;
            
            // 计算目标路径
            QString domain = "HomeDomain";
            QString relativePath = filePath;
            
            // 移除可能的域前缀
            if (relativePath.contains("-")) {
                int dashPos = relativePath.indexOf("-");
                relativePath = relativePath.mid(dashPos + 1);
            }
            
            QString fileKey = domain + "-" + relativePath;
            QByteArray hash = QCryptographicHash::hash(fileKey.toUtf8(), QCryptographicHash::Sha1);
            QString hashStr = hash.toHex();
            
            // 创建目标目录（使用前两位作为子目录）
            QString folder2 = hashStr.left(2);
            QString targetDir = QDir(backupDir).filePath(folder2);
            QDir().mkpath(targetDir);
            
            QString targetPath = QDir(targetDir).filePath(hashStr);
            qDebug() << "ContactManager: handleUploadFiles - 目标路径:" << targetPath;
            
            // 接收文件数据
            if (receiveFileData(client, targetPath)) {
                qDebug() << "ContactManager: handleUploadFiles - 文件接收成功:" << targetPath;
                
                // 检查是否是通讯录数据库
                if (filePath.contains("AddressBook.sqlitedb", Qt::CaseInsensitive) &&
                    !filePath.contains("-shm") && !filePath.contains("-wal")) {
                    qDebug() << "ContactManager: handleUploadFiles - 发现通讯录数据库!";
                    addressBookPath = targetPath;
                }
            } else {
                qWarning() << "ContactManager: handleUploadFiles - 文件接收失败:" << targetPath;
            }
        }
    } else if (filesNodeType == PLIST_DICT) {
        qDebug() << "ContactManager: handleUploadFiles - 文件列表是字典类型";
        
        // 遍历字典
        plist_dict_iter iter = nullptr;
        lib.plist_dict_new_iter(files_node, &iter);
        
        if (iter) {
            char* key = nullptr;
            plist_t value = nullptr;
            
            while (true) {
                lib.plist_dict_next_item(files_node, iter, &key, &value);
                if (!key) break;
                
                fileCount++;
                QString keyStr = QString::fromUtf8(key);
                qDebug() << "ContactManager: handleUploadFiles - 处理文件键:" << keyStr;
                
                // 处理文件上传（字典形式），这里保持日志，具体处理视协议而定
            }
        }
    }
    
    qDebug() << "ContactManager: handleUploadFiles - 处理完成，共接收" << fileCount << "个文件";
    return 0;  // 返回成功
}

int ContactManager::handleDownloadFilesRequest(mobilebackup2_client_t client, plist_t message,
                                                const QString& backupDir)
{
    // 按照 idevicebackup2 的 mb2_handle_send_files() 实现
    // 参考: tools/idevicebackup2.c:922
    //
    // 协议常量（来自 idevicebackup2.c）:
    // CODE_SUCCESS = 0x00      - 文件传输成功
    // CODE_FILE_DATA = 0x01    - 文件数据块
    // CODE_ERROR_LOCAL = 0x0A  - 本地错误（文件不存在等）
    
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    constexpr uint8_t CODE_SUCCESS = 0x00;
    constexpr uint8_t CODE_FILE_DATA = 0x01;
    constexpr uint8_t CODE_ERROR_LOCAL = 0x0A;
    
    // htobe32 辅助函数（主机字节序转大端）
    auto htobe32_local = [](uint32_t val) -> uint32_t {
        return ((val & 0xFF000000) >> 24) |
               ((val & 0x00FF0000) >> 8) |
               ((val & 0x0000FF00) << 8) |
               ((val & 0x000000FF) << 24);
    };
    
    qDebug() << "ContactManager: handleDownloadFilesRequest - 开始处理";
    
    plist_t errplist = nullptr;
    
    // 辅助函数：添加文件错误到 errplist
    auto add_file_error = [&](const char* path, int error_code, const char* error_message) {
        if (!errplist) {
            errplist = lib.plist_new_dict();
        }
        plist_t filedict = lib.plist_new_dict();
        lib.plist_dict_set_item(filedict, "DLFileErrorString", lib.plist_new_string(error_message));
        lib.plist_dict_set_item(filedict, "DLFileErrorCode", lib.plist_new_int((int64_t)error_code));
        lib.plist_dict_set_item(errplist, path, filedict);
    };
    
    // errno 到设备错误码的映射
    auto errno_to_device_error = [](int errno_value) -> int {
        switch (errno_value) {
            case ENOENT: return -6;
            case EEXIST: return -7;
            default: return -1;
        }
    };
    
    if (!message) {
        qWarning() << "ContactManager: handleDownloadFilesRequest - message 为空";
        return -1;
    }
    
    // 验证消息格式
    plist_type msgType = lib.plist_get_node_type(message);
    if (msgType != PLIST_ARRAY) {
        qWarning() << "ContactManager: handleDownloadFilesRequest - message 不是 ARRAY 类型";
        return -1;
    }
    
    uint32_t array_size = lib.plist_array_get_size(message);
    if (array_size < 2) {
        qWarning() << "ContactManager: handleDownloadFilesRequest - 数组大小不足";
        return -1;
    }
    
    // 获取文件列表（索引1）
    plist_t files = lib.plist_array_get_item(message, 1);
    if (!files) {
        qWarning() << "ContactManager: handleDownloadFilesRequest - 无法获取文件列表";
        return -1;
    }
    
    uint32_t cnt = lib.plist_array_get_size(files);
    qDebug() << "ContactManager: handleDownloadFilesRequest - 文件数量:" << cnt;
    
    // 处理每个文件请求
    for (uint32_t i = 0; i < cnt; i++) {
        plist_t val = lib.plist_array_get_item(files, i);
        if (!val || lib.plist_get_node_type(val) != PLIST_STRING) {
            continue;
        }
        
        const char* path = lib.plist_get_string_ptr(val, nullptr);
        if (!path) {
            continue;
        }
        
        QString pathStr = QString::fromUtf8(path);
        qDebug() << "ContactManager: handleDownloadFilesRequest - 处理文件:" << pathStr;
        
        uint32_t pathlen = strlen(path);
        uint32_t bytes = 0;
        
        // === 步骤1：发送路径长度（4字节 BE） ===
        uint32_t nlen = htobe32_local(pathlen);
        if (lib.mobilebackup2_send_raw(client, (const char*)&nlen, 4, &bytes) != MOBILEBACKUP2_E_SUCCESS || bytes != 4) {
            qWarning() << "ContactManager: 发送路径长度失败";
            continue;
        }
        
        // === 步骤2：发送路径字符串 ===
        if (lib.mobilebackup2_send_raw(client, path, pathlen, &bytes) != MOBILEBACKUP2_E_SUCCESS || bytes != pathlen) {
            qWarning() << "ContactManager: 发送路径失败";
            continue;
        }
        
        // === 步骤3：检查文件是否存在并发送内容或错误 ===
        // 构造本地文件路径
        QString localPath;
        if (pathStr.contains("/")) {
            QStringList parts = pathStr.split("/");
            // 如果路径以 UDID 开头，跳过 UDID 部分
            if (parts.size() > 1 && parts[0] == m_currentUdid) {
                localPath = QDir(backupDir).filePath(parts.mid(1).join("/"));
            } else {
                localPath = QDir(backupDir).filePath(pathStr);
            }
        } else {
            localPath = QDir(backupDir).filePath(pathStr);
        }
        
        qDebug() << "ContactManager: handleDownloadFilesRequest - 本地路径:" << localPath;
        
        QFileInfo fileInfo(localPath);
        
        if (fileInfo.exists() && fileInfo.isFile()) {
            // 文件存在，发送文件内容
            QFile f(localPath);
            if (!f.open(QIODevice::ReadOnly)) {
                qWarning() << "ContactManager: 无法打开文件:" << localPath;
                // 发送错误帧
                const char* errmsg = "Cannot open file";
                uint32_t errlen = strlen(errmsg);
                nlen = htobe32_local(errlen + 1);
                char header[5];
                memcpy(header, &nlen, 4);
                header[4] = CODE_ERROR_LOCAL;
                lib.mobilebackup2_send_raw(client, header, 5, &bytes);
                lib.mobilebackup2_send_raw(client, errmsg, errlen, &bytes);
                add_file_error(path, errno_to_device_error(errno), errmsg);
                continue;
            }
            
            qint64 total = f.size();
            qDebug() << "ContactManager: handleDownloadFilesRequest - 文件大小:" << total;
            
            if (total == 0) {
                // 空文件，直接发送成功帧
                nlen = htobe32_local(1);
                char header[5];
                memcpy(header, &nlen, 4);
                header[4] = CODE_SUCCESS;
                lib.mobilebackup2_send_raw(client, header, 5, &bytes);
            } else {
                // 发送文件内容
                char buf[32768];
                qint64 sent_total = 0;
                
                while (sent_total < total) {
                    qint64 to_read = qMin((qint64)sizeof(buf), total - sent_total);
                    qint64 r = f.read(buf, to_read);
                    if (r <= 0) {
                        qWarning() << "ContactManager: 读取文件失败";
                        break;
                    }
                    
                    // 发送数据帧头：长度+1, CODE_FILE_DATA
                    nlen = htobe32_local((uint32_t)r + 1);
                    char header[5];
                    memcpy(header, &nlen, 4);
                    header[4] = CODE_FILE_DATA;
                    
                    if (lib.mobilebackup2_send_raw(client, header, 5, &bytes) != MOBILEBACKUP2_E_SUCCESS || bytes != 5) {
                        qWarning() << "ContactManager: 发送数据帧头失败";
                        break;
                    }
                    
                    // 发送文件数据
                    if (lib.mobilebackup2_send_raw(client, buf, (uint32_t)r, &bytes) != MOBILEBACKUP2_E_SUCCESS || bytes != (uint32_t)r) {
                        qWarning() << "ContactManager: 发送文件数据失败";
                        break;
                    }
                    
                    sent_total += r;
                }
                
                // 发送成功帧
                nlen = htobe32_local(1);
                char header[5];
                memcpy(header, &nlen, 4);
                header[4] = CODE_SUCCESS;
                lib.mobilebackup2_send_raw(client, header, 5, &bytes);
            }
            
            f.close();
            qDebug() << "ContactManager: handleDownloadFilesRequest - 文件发送完成:" << pathStr;
        } else {
            // 文件不存在，发送错误帧
            qDebug() << "ContactManager: handleDownloadFilesRequest - 文件不存在:" << localPath;
            const char* errmsg = "No such file or directory";
            uint32_t errlen = strlen(errmsg);
            
            // 按照参考实现，将帧头和错误消息合并成一次发送
            char buf[256];
            nlen = htobe32_local(errlen + 1);
            memcpy(buf, &nlen, 4);
            buf[4] = CODE_ERROR_LOCAL;
            uint32_t slen = 5;
            memcpy(buf + slen, errmsg, errlen);
            slen += errlen;
            
            lib.mobilebackup2_send_raw(client, buf, slen, &bytes);
            
            // 按照参考实现，将错误添加到 errplist
            add_file_error(path, errno_to_device_error(ENOENT), errmsg);
        }
    }
    
    // === 发送终止标记（4字节零） ===
    uint32_t zero = 0;
    uint32_t bytes = 0;
    lib.mobilebackup2_send_raw(client, (const char*)&zero, 4, &bytes);
    qDebug() << "ContactManager: handleDownloadFilesRequest - 已发送终止标记";
    
    // === 发送状态响应 ===
    if (!errplist) {
        plist_t emptydict = lib.plist_new_dict();
        lib.mobilebackup2_send_status_response(client, 0, NULL, emptydict);
        lib.plist_free(emptydict);
        qDebug() << "ContactManager: handleDownloadFilesRequest - 已发送成功状态响应";
    } else {
        lib.mobilebackup2_send_status_response(client, -13, "Multi status", errplist);
        lib.plist_free(errplist);
        qDebug() << "ContactManager: handleDownloadFilesRequest - 已发送 Multi status 错误响应";
    }
    
    qDebug() << "ContactManager: handleDownloadFilesRequest - 处理完成";
    return 0;
}

bool ContactManager::receiveFileData(mobilebackup2_client_t client, const QString& targetPath)
{
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "ContactManager: 无法创建文件:" << targetPath;
        return false;
    }
    
    // 接收文件数据
    char buffer[65536];
    uint32_t bytes_received = 0;
    uint64_t total_received = 0;
    
    while (true) {
        mobilebackup2_error_t err = lib.mobilebackup2_receive_raw(
            client, buffer, sizeof(buffer), &bytes_received);
        
        if (err != MOBILEBACKUP2_E_SUCCESS || bytes_received == 0) {
            break;
        }
        
        file.write(buffer, bytes_received);
        total_received += bytes_received;
    }
    
    file.close();
    
    qDebug() << "ContactManager: 接收文件完成，大小:" << total_received << "字节";
    return total_received > 0;
}

void ContactManager::handleProcessMessage(plist_t message, bool& backupComplete)
{
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    if (!message) return;
    
    // 打印完整的消息内容以便调试
    char* xml_out = nullptr;
    uint32_t xml_len = 0;
    lib.plist_to_xml(message, &xml_out, &xml_len);
    if (xml_out) {
        qDebug() << "ContactManager: DLMessageProcessMessage 内容:" << QString::fromUtf8(xml_out, xml_len);
        if (lib.plist_mem_free) {
            lib.plist_mem_free(xml_out);
        }
    }
    
    // 检查消息内容，判断备份是否完成或出错
    if (lib.plist_get_node_type(message) == PLIST_ARRAY) {
        uint32_t array_size = lib.plist_array_get_size(message);
        qDebug() << "ContactManager: ProcessMessage 数组大小:" << array_size;
        
        if (array_size >= 2) {
            plist_t status_dict = lib.plist_array_get_item(message, 1);
            if (status_dict && lib.plist_get_node_type(status_dict) == PLIST_DICT) {
                // 检查是否有错误
                plist_t errorNode = lib.plist_dict_get_item(status_dict, "ErrorCode");
                if (errorNode) {
                    uint64_t errorCode = 0;
                    lib.plist_get_uint_val(errorNode, &errorCode);
                    qDebug() << "ContactManager: ProcessMessage 错误码:" << errorCode;
                    
                    plist_t errorMsg = lib.plist_dict_get_item(status_dict, "ErrorMessage");
                    if (errorMsg && lib.plist_get_node_type(errorMsg) == PLIST_STRING) {
                        const char* msg = lib.plist_get_string_ptr(errorMsg, nullptr);
                        if (msg) {
                            qDebug() << "ContactManager: ProcessMessage 错误信息:" << msg;
                        }
                    }
                }
                
                // 检查备份是否完成
                plist_t finished = lib.plist_dict_get_item(status_dict, "BackupFinished");
                if (finished) {
                    uint8_t val = 0;
                    lib.plist_get_bool_val(finished, &val);
                    if (val) {
                        qDebug() << "ContactManager: 备份已完成";
                        backupComplete = true;
                    }
                }
                
                // 检查备份状态
                plist_t backupState = lib.plist_dict_get_item(status_dict, "BackupState");
                if (backupState && lib.plist_get_node_type(backupState) == PLIST_STRING) {
                    const char* state = lib.plist_get_string_ptr(backupState, nullptr);
                    if (state) {
                        qDebug() << "ContactManager: 备份状态:" << state;
                    }
                }
            }
        }
    }
}

QString ContactManager::findAddressBookInBackup(const QString& backupDir)
{
    // 计算 AddressBook.sqlitedb 的哈希路径
    QString domain = "HomeDomain";
    QString relativePath = "Library/AddressBook/AddressBook.sqlitedb";
    QString fileKey = domain + "-" + relativePath;
    
    QByteArray hash = QCryptographicHash::hash(fileKey.toUtf8(), QCryptographicHash::Sha1);
    QString hashStr = hash.toHex();
    
    qDebug() << "ContactManager: 搜索通讯录数据库，Hash:" << hashStr;
    
    // 检查不同层级的目录结构
    // 结构1: <backupDir>/<hash> (旧版)
    // 结构2: <backupDir>/<hash_first_2_chars>/<hash> (新版 iOS 10+)
    // 结构3: <backupDir>/Snapshot/<hash_first_2_chars>/<hash> (idevicebackup2 风格)
    
    QString folder2 = hashStr.left(2);
    
    // 候选路径列表（按优先级排序）
    QStringList candidatePaths = {
        QDir(backupDir).filePath("Snapshot/" + folder2 + "/" + hashStr),  // idevicebackup2 风格
        QDir(backupDir).filePath(folder2 + "/" + hashStr),                // 新版 iOS 10+
        QDir(backupDir).filePath(hashStr),                                 // 旧版
    };
    
    for (const QString& path : candidatePaths) {
        qDebug() << "ContactManager: 检查路径:" << path;
        if (QFile::exists(path)) {
            qDebug() << "ContactManager: 找到通讯录数据库:" << path;
            return path;
        }
    }
    
    qWarning() << "ContactManager: 未在备份中找到通讯录数据库文件";
    qWarning() << "ContactManager: 搜索 Hash:" << hashStr;
    for (const QString& path : candidatePaths) {
        qWarning() << "ContactManager: 已尝试路径:" << path;
    }
    
    return QString();
}

bool ContactManager::readContactsFromDB(const QString& dbPath)
{
    m_contacts.clear();
    
    // 添加 SQLite 数据库连接
    // 使用唯一的连接名称以避免冲突
    QString connectionName = "ContactDB_" + QString::number(QCoreApplication::applicationPid());
    
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbPath);
        
        if (!db.open()) {
            qCritical() << "ContactManager: 无法打开数据库:" << db.lastError().text();
            return false;
        }
        
        // 查询联系人
        // ABPerson 表包含联系人基本信息
        QSqlQuery query(db);
        
        // 尝试查询 ABPerson 表
        QString sql = "SELECT ROWID, First, Last, Organization, Note FROM ABPerson";
        
        if (!query.exec(sql)) {
            qCritical() << "ContactManager: 查询失败:" << query.lastError().text();
            // 尝试列出所有表以便调试
            qDebug() << "ContactManager: 数据库表列表:" << db.tables();
            return false;
        }
        
        while (query.next()) {
            Contact contact;
            int rowId = query.value(0).toInt();
            contact.id = QString::number(rowId);
            contact.firstName = query.value(1).toString();
            contact.lastName = query.value(2).toString();
            contact.organization = query.value(3).toString();
            contact.note = query.value(4).toString();
            
            // 获取电话号码
            QSqlQuery phoneQuery(db);
            phoneQuery.prepare("SELECT value FROM ABMultiValue WHERE record_id = ? AND property = 3");
            phoneQuery.addBindValue(rowId);
            if (phoneQuery.exec()) {
                while (phoneQuery.next()) {
                    QString phone = phoneQuery.value(0).toString();
                    // 清理电话号码中的特殊字符
                    phone = phone.remove(QRegularExpression("[^0-9+*#]"));
                    if (!phone.isEmpty()) {
                        contact.phoneNumbers.append(phone);
                    }
                }
            }
            
            // 获取邮箱
            QSqlQuery emailQuery(db);
            emailQuery.prepare("SELECT value FROM ABMultiValue WHERE record_id = ? AND property = 4");
            emailQuery.addBindValue(rowId);
            if (emailQuery.exec()) {
                while (emailQuery.next()) {
                    contact.emails.append(emailQuery.value(0).toString());
                }
            }
            
            // 仅添加有效的联系人
            if (!contact.fullName().isEmpty() || !contact.phoneNumbers.isEmpty()) {
                m_contacts.append(contact);
            }
        }
        
        db.close();
    }
    
    // 移除连接
    QSqlDatabase::removeDatabase(connectionName);
    
    return true;
}

// 保留辅助函数以满足接口，但暂时未使用
Contact ContactManager::parseContact(plist_t plist) const
{
    Q_UNUSED(plist);
    return Contact();
}

QString ContactManager::getStringValue(plist_t dict, const char *key) const
{
    Q_UNUSED(dict);
    Q_UNUSED(key);
    return QString();
}

QStringList ContactManager::extractStringArray(plist_t array) const
{
    Q_UNUSED(array);
    return QStringList();
}

QVector<Contact> ContactManager::searchContacts(const QString &keyword) const
{
    QVector<Contact> results;
    
    if (keyword.isEmpty()) {
        return m_contacts;
    }
    
    QString lowerKeyword = keyword.toLower();
    
    for (const Contact& contact : m_contacts) {
        bool matched = false;
        
        // 搜索姓名
        if (contact.fullName().toLower().contains(lowerKeyword)) {
            matched = true;
        }
        
        // 搜索电话号码
        if (!matched) {
            for (const QString& phone : contact.phoneNumbers) {
                if (phone.contains(keyword)) {
                    matched = true;
                    break;
                }
            }
        }
        
        // 搜索邮箱
        if (!matched) {
            for (const QString& email : contact.emails) {
                if (email.toLower().contains(lowerKeyword)) {
                    matched = true;
                    break;
                }
            }
        }
        
        // 搜索组织
        if (!matched && contact.organization.toLower().contains(lowerKeyword)) {
            matched = true;
        }
        
        if (matched) {
            results.append(contact);
        }
    }
    
    return results;
}

bool ContactManager::exportToVCard(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    for (const Contact& contact : m_contacts) {
        // vCard 3.0 格式
        out << "BEGIN:VCARD\n";
        out << "VERSION:3.0\n";
        
        // 姓名
        QString fullName = contact.fullName();
        out << "FN:" << fullName << "\n";
        out << "N:" << contact.lastName << ";" << contact.firstName << ";;;\n";
        
        // 组织
        if (!contact.organization.isEmpty()) {
            out << "ORG:" << contact.organization << "\n";
        }
        
        // 电话号码
        for (const QString& phone : contact.phoneNumbers) {
            out << "TEL:" << phone << "\n";
        }
        
        // 邮箱
        for (const QString& email : contact.emails) {
            out << "EMAIL:" << email << "\n";
        }
        
        // 备注
        if (!contact.note.isEmpty()) {
            out << "NOTE:" << contact.note << "\n";
        }
        
        out << "END:VCARD\n";
    }
    
    file.close();
    return true;
}

void ContactManager::cleanup()
{
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    if (m_device) {
        lib.idevice_free(m_device);
        m_device = nullptr;
    }
}
