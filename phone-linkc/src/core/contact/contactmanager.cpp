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

    // 创建临时目录用于存储提取的数据库
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        emit errorOccurred("无法创建临时目录");
        return false;
    }
    
    QString backupRoot = tempDir.path();
    qDebug() << "ContactManager: 准备备份到临时目录:" << backupRoot;

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
    
    // 构建备份选项
    plist_t options = lib.plist_new_dict();
    
    // 强制完整备份设置为 false
    lib.plist_dict_set_item(options, "ForceFullBackup", lib.plist_new_bool(0));
    // 不添加 BackupFileList - 让设备决定备份哪些文件
    
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
            // 注意: dlmessage 是由 DLL 分配的内存。
            // 由于 Windows 跨模块 CRT 堆隔离问题，在 EXE 中调用 free(dlmessage) 会导致堆崩溃。
            // 因此不释放 dlmessage，会有少量内存泄漏，但比崩溃更可接受。
            // 只释放 msg_plist，因为它通过 lib.plist_free() 使用 DLL 的释放函数
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
                // 设备上传文件给我们（用于备份）- 这是我们需要接收文件数据的地方！
                qDebug() << "ContactManager: 处理文件上传（设备发送文件给我们，用于备份）...";
                int result = handleUploadFiles(mb2_client, msg_plist, deviceBackupDir, addressBookPath);
                lib.mobilebackup2_send_status_response(mb2_client, result, nullptr, nullptr);
                
            } else if (msgType == "DLMessageGetFreeDiskSpace") {
                // 返回足够的磁盘空间
                qDebug() << "ContactManager: 收到磁盘空间查询";
                plist_t space_dict = lib.plist_new_dict();
                // 返回 10GB 可用空间
                lib.plist_dict_set_item(space_dict, "FreeSpace", lib.plist_new_string("10737418240"));
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
            // 参考: appmanager.cpp 中对 plist_dict_next_item 返回的 key 的处理方式
            // free(dlmessage);  // 不能调用！会导致堆崩溃
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
                
                // 处理文件上传
                // 这里需要根据实际协议处理文件数据
                
                // 注意: key 是 DLL 分配的内存，不释放
            }
        }
    }
    
    qDebug() << "ContactManager: handleUploadFiles - 处理完成，共接收" << fileCount << "个文件";
    return 0;  // 返回成功
}

int ContactManager::handleDownloadFilesRequest(mobilebackup2_client_t client, plist_t message,
                                                const QString& backupDir)
{
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    qDebug() << "ContactManager: handleDownloadFilesRequest - 开始处理";
    
    if (!message) {
        qWarning() << "ContactManager: handleDownloadFilesRequest - message 为空";
        return -1;
    }
    
    // 从消息中获取文件列表
    // DLMessageDownloadFiles 消息格式可能有多种：
    // 格式1: [msgtype, files_dict, ...]
    // 格式2: [msgtype, files_array, ...]
    plist_type msgType = lib.plist_get_node_type(message);
    qDebug() << "ContactManager: handleDownloadFilesRequest - message 类型:" << msgType << "(期望 PLIST_ARRAY=" << PLIST_ARRAY << ")";
    
    if (msgType != PLIST_ARRAY) {
        qWarning() << "ContactManager: handleDownloadFilesRequest - message 不是 ARRAY 类型";
        return -1;
    }
    
    uint32_t array_size = lib.plist_array_get_size(message);
    qDebug() << "ContactManager: handleDownloadFilesRequest - 数组大小:" << array_size;
    
    // 打印所有数组元素的类型
    for (uint32_t i = 0; i < array_size; i++) {
        plist_t item = lib.plist_array_get_item(message, i);
        if (item) {
            plist_type itemType = lib.plist_get_node_type(item);
            qDebug() << "ContactManager: handleDownloadFilesRequest - 元素[" << i << "] 类型:" << itemType;
            
            // 如果是字符串，打印内容
            if (itemType == PLIST_STRING) {
                const char* strVal = lib.plist_get_string_ptr(item, nullptr);
                if (strVal) {
                    qDebug() << "ContactManager: handleDownloadFilesRequest - 元素[" << i << "] 字符串值:" << strVal;
                }
            }
        }
    }
    
    if (array_size < 2) {
        qWarning() << "ContactManager: handleDownloadFilesRequest - 数组大小不足 (需要至少2个元素)";
        return -1;
    }
    
    // DLMessageDownloadFiles 消息格式: [msgtype, files_array, error_dict, ...]
    // 索引1应该是文件列表（ARRAY 类型）
    plist_t files_node = lib.plist_array_get_item(message, 1);
    if (!files_node) {
        qWarning() << "ContactManager: handleDownloadFilesRequest - 无法获取索引1的元素";
        return -1;
    }
    
    plist_type filesNodeType = lib.plist_get_node_type(files_node);
    qDebug() << "ContactManager: handleDownloadFilesRequest - 索引1元素类型:" << filesNodeType;
    
    // 如果索引1是数组，打印其大小和前几个元素
    if (filesNodeType == PLIST_ARRAY) {
        uint32_t arr_size = lib.plist_array_get_size(files_node);
        qDebug() << "ContactManager: handleDownloadFilesRequest - 文件数组大小:" << arr_size;
        
        // 打印前5个元素的详细信息
        for (uint32_t i = 0; i < arr_size && i < 5; i++) {
            plist_t arrItem = lib.plist_array_get_item(files_node, i);
            if (arrItem) {
                plist_type itemType = lib.plist_get_node_type(arrItem);
                qDebug() << "ContactManager: handleDownloadFilesRequest - 文件数组[" << i << "] 类型:" << itemType;
                
                if (itemType == PLIST_STRING) {
                    const char* strVal = lib.plist_get_string_ptr(arrItem, nullptr);
                    if (strVal) {
                        qDebug() << "ContactManager: handleDownloadFilesRequest - 文件数组[" << i << "] 字符串:" << strVal;
                    }
                } else if (itemType == PLIST_DICT) {
                    // 打印字典的键
                    plist_dict_iter diter = nullptr;
                    lib.plist_dict_new_iter(arrItem, &diter);
                    if (diter) {
                        char* dkey = nullptr;
                        plist_t dval = nullptr;
                        int keyCount = 0;
                        while (keyCount < 5) {
                            lib.plist_dict_next_item(arrItem, diter, &dkey, &dval);
                            if (!dkey) break;
                            qDebug() << "ContactManager: handleDownloadFilesRequest - 文件数组[" << i << "] 字典键:" << dkey;
                            keyCount++;
                        }
                    }
                }
            }
        }
    }
    
    // 根据 files_node 的类型处理
    int fileCount = 0;
    
    if (filesNodeType == PLIST_DICT) {
        // 遍历文件字典
        plist_dict_iter iter = nullptr;
        lib.plist_dict_new_iter(files_node, &iter);
        
        if (!iter) {
            qWarning() << "ContactManager: handleDownloadFilesRequest - 无法创建字典迭代器";
            // 尝试打印字典的第一个键来调试
            plist_t testKey = lib.plist_dict_get_item(files_node, "DLFileDest");
            if (testKey) {
                qDebug() << "ContactManager: handleDownloadFilesRequest - 发现 DLFileDest 键";
            }
            return -1;
        }
        
        char* key = nullptr;
        plist_t value = nullptr;
        
        qDebug() << "ContactManager: handleDownloadFilesRequest - 开始遍历文件字典";
        
        while (true) {
            lib.plist_dict_next_item(files_node, iter, &key, &value);
            if (!key) {
                qDebug() << "ContactManager: handleDownloadFilesRequest - 遍历结束，共处理" << fileCount << "个文件";
                break;
            }
            
            fileCount++;
            QString keyStr = QString::fromUtf8(key);
            qDebug() << "ContactManager: handleDownloadFilesRequest - 字典键 #" << fileCount << ":" << keyStr;
            
            // 打印值的类型
            if (value) {
                plist_type valType = lib.plist_get_node_type(value);
                qDebug() << "ContactManager: handleDownloadFilesRequest - 值类型:" << valType;
            }
            
            // DLMessageDownloadFiles 是设备请求从电脑获取文件（用于恢复）
            // 对于备份操作，通常返回文件不存在即可
            qDebug() << "ContactManager: handleDownloadFilesRequest - 文件请求（恢复场景）:" << keyStr;
            
            // 注意: key 是由 libimobiledevice DLL 的 plist_dict_next_item 分配的内存。
            // 由于 Windows 跨模块 CRT 堆隔离问题，不释放 key。
        }
        
        // 如果字典为空，可能需要检查索引1的数组
        if (fileCount == 0) {
            qDebug() << "ContactManager: handleDownloadFilesRequest - 字典为空，尝试检查数组 (索引1)";
        }
        
        // iter 也是 DLL 分配的内存，不释放
        
    } else if (filesNodeType == PLIST_ARRAY) {
        // 遍历文件数组
        uint32_t files_count = lib.plist_array_get_size(files_node);
        qDebug() << "ContactManager: handleDownloadFilesRequest - 文件数组大小:" << files_count;
        
        for (uint32_t i = 0; i < files_count; i++) {
            plist_t fileItem = lib.plist_array_get_item(files_node, i);
            if (!fileItem) continue;
            
            plist_type fileItemType = lib.plist_get_node_type(fileItem);
            qDebug() << "ContactManager: handleDownloadFilesRequest - 文件项[" << i << "] 类型:" << fileItemType;
            
            QString filePath;
            
            if (fileItemType == PLIST_STRING) {
                const char* strVal = lib.plist_get_string_ptr(fileItem, nullptr);
                if (strVal) {
                    filePath = QString::fromUtf8(strVal);
                }
            } else if (fileItemType == PLIST_DICT) {
                // 可能是包含文件路径的字典
                plist_t pathNode = lib.plist_dict_get_item(fileItem, "DLFileDest");
                if (!pathNode) {
                    pathNode = lib.plist_dict_get_item(fileItem, "Path");
                }
                if (!pathNode) {
                    pathNode = lib.plist_dict_get_item(fileItem, "BackupPath");
                }
                if (pathNode && lib.plist_get_node_type(pathNode) == PLIST_STRING) {
                    const char* strVal = lib.plist_get_string_ptr(pathNode, nullptr);
                    if (strVal) {
                        filePath = QString::fromUtf8(strVal);
                    }
                }
            }
            
            if (!filePath.isEmpty()) {
                fileCount++;
                qDebug() << "ContactManager: 备份文件 #" << fileCount << ":" << filePath;
                
                // 计算保存路径
                QString targetDir = backupDir;
                QString targetPath;
                
                // 如果路径包含设备UDID，使用完整路径
                if (filePath.contains("/")) {
                    QStringList parts = filePath.split("/");
                    if (parts.size() > 1) {
                        // 创建子目录
                        QString subDir = parts[0];
                        targetDir = QDir(backupDir).filePath(subDir);
                        QDir().mkpath(targetDir);
                        targetPath = QDir(targetDir).filePath(parts.mid(1).join("/"));
                    }
                }
                
                if (targetPath.isEmpty()) {
                    targetPath = QDir(targetDir).filePath(filePath);
                }
                
                // 确保目录存在
                QFileInfo fi(targetPath);
                QDir().mkpath(fi.absolutePath());
                
                qDebug() << "ContactManager: handleDownloadFilesRequest - 文件请求:" << targetPath;
                
                // 特殊处理 Status.plist
                if (filePath.endsWith("Status.plist", Qt::CaseInsensitive)) {
                    qDebug() << "ContactManager: handleDownloadFilesRequest - 创建默认 Status.plist";
                    
                    // 创建一个空的状态文件
                    plist_t status_dict = lib.plist_new_dict();
                    lib.plist_dict_set_item(status_dict, "IsFullBackup", lib.plist_new_bool(0));
                    lib.plist_dict_set_item(status_dict, "Version", lib.plist_new_string("2.0"));
                    lib.plist_dict_set_item(status_dict, "Date", lib.plist_new_date(0, 0));
                    
                    char* plist_xml = nullptr;
                    uint32_t xml_length = 0;
                    lib.plist_to_xml(status_dict, &plist_xml, &xml_length);
                    
                    if (plist_xml && xml_length > 0) {
                        // 发送文件大小（按大端序）
                        auto to_be32 = [](uint32_t v) -> uint32_t {
                            return ((v & 0x000000FFu) << 24) |
                                   ((v & 0x0000FF00u) << 8)  |
                                   ((v & 0x00FF0000u) >> 8)  |
                                   ((v & 0xFF000000u) >> 24);
                        };
                        uint32_t bytes_sent = 0;
                        uint32_t be_len = to_be32(static_cast<uint32_t>(xml_length));
                        lib.mobilebackup2_send_raw(client, (const char*)&be_len, sizeof(be_len), &bytes_sent);
                        
                        // 发送文件内容
                        bytes_sent = 0;
                        lib.mobilebackup2_send_raw(client, plist_xml, xml_length, &bytes_sent);
                        qDebug() << "ContactManager: handleDownloadFilesRequest - 已发送 Status.plist，大小:" << bytes_sent;
                        
                        // 发送 0 作为文件结束标记（按大端序；值为0端序无影响，这里保持一致）
                        uint32_t be_zero = 0;
                        bytes_sent = 0;
                        lib.mobilebackup2_send_raw(client, (const char*)&be_zero, sizeof(be_zero), &bytes_sent);
                    }
                    
                    // 使用 DLL 的释放函数释放 XML 缓冲区，避免跨模块释放问题
                    if (plist_xml && lib.plist_mem_free) {
                        lib.plist_mem_free(plist_xml);
                        plist_xml = nullptr;
                    }
                    
                    lib.plist_free(status_dict);
                } else {
                    // 其他文件检查是否存在
                    if (QFile::exists(targetPath)) {
                        qDebug() << "ContactManager: handleDownloadFilesRequest - 文件存在，发送文件";
                        // TODO: 实现完整的文件发送
                    } else {
                        qDebug() << "ContactManager: handleDownloadFilesRequest - 文件不存在";
                        // 发送 0 字节长度表示文件不存在（按大端序；值为0端序无影响，这里保持一致）
                        uint32_t be_zero = 0;
                        uint32_t bytes_sent_len = 0;
                        lib.mobilebackup2_send_raw(client, (const char*)&be_zero, sizeof(be_zero), &bytes_sent_len);
                    }
                }
            }
        }
        
        qDebug() << "ContactManager: handleDownloadFilesRequest - 遍历结束，共处理" << fileCount << "个文件";
    }
    
    qDebug() << "ContactManager: handleDownloadFilesRequest - 处理完成";
    // 发送一次最终的 0 长度（大端序值为 0，端序无影响，这里保持一致）作为整个文件清单结束标记
    {
        uint32_t be_zero = 0;
        uint32_t bytes_sent_len = 0;
        lib.mobilebackup2_send_raw(client, (const char*)&be_zero, sizeof(be_zero), &bytes_sent_len);
    }
    // 对 DownloadFiles 请求在发送完所有原始数据后补发一次状态响应，提示设备可以继续
    lib.mobilebackup2_send_status_response(client, 0, nullptr, nullptr);
    return 0;  // 返回成功
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
    
    // 检查消息内容，判断备份是否完成
    if (lib.plist_get_node_type(message) == PLIST_ARRAY) {
        uint32_t array_size = lib.plist_array_get_size(message);
        if (array_size >= 2) {
            plist_t status_dict = lib.plist_array_get_item(message, 1);
            if (status_dict && lib.plist_get_node_type(status_dict) == PLIST_DICT) {
                plist_t finished = lib.plist_dict_get_item(status_dict, "BackupFinished");
                if (finished) {
                    uint8_t val = 0;
                    lib.plist_get_bool_val(finished, &val);
                    if (val) {
                        qDebug() << "ContactManager: 备份已完成";
                        backupComplete = true;
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
    
    // 检查不同层级的目录结构
    // 结构1: <backupDir>/<hash> (旧版)
    // 结构2: <backupDir>/<hash_first_2_chars>/<hash> (新版 iOS 10+)
    
    QString folder2 = hashStr.left(2);
    QString pathV2 = QDir(backupDir).filePath(folder2 + "/" + hashStr);
    QString pathV1 = QDir(backupDir).filePath(hashStr);
    
    if (QFile::exists(pathV2)) {
        return pathV2;
    } else if (QFile::exists(pathV1)) {
        return pathV1;
    }
    
    qWarning() << "ContactManager: 未在备份中找到通讯录数据库文件";
    qWarning() << "ContactManager: 搜索 Hash:" << hashStr;
    qWarning() << "ContactManager: 搜索路径 V2:" << pathV2;
    qWarning() << "ContactManager: 搜索路径 V1:" << pathV1;
    
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