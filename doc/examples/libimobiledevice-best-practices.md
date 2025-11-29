# libimobiledevice 最佳实践指南

> 📋 开发高质量 iOS 设备管理应用的最佳实践与注意事项

## 错误处理最佳实践

### 1. 连接错误处理

```cpp
// 示例：完善的错误处理
class SafeDeviceConnector
{
public:
    enum class ConnectionError {
        None,
        DeviceNotFound,
        PairingFailed,
        HandshakeFailed,
        Timeout,
        UnknownError
    };
    
    struct ConnectionResult {
        bool success;
        ConnectionError error;
        QString errorMessage;
        QString deviceName;
    };
    
    static ConnectionResult connectToDevice(const QString &udid, int timeoutMs = 10000)
    {
        ConnectionResult result;
        result.success = false;
        result.error = ConnectionError::None;
        
        idevice_t device = nullptr;
        lockdownd_client_t lockdown = nullptr;
        
        do {
            // 1. 创建设备连接
            idevice_error_t device_err = idevice_new(&device, udid.toUtf8().constData());
            if (device_err != IDEVICE_E_SUCCESS) {
                result.error = (device_err == IDEVICE_E_NO_DEVICE) ? 
                              ConnectionError::DeviceNotFound : 
                              ConnectionError::UnknownError;
                result.errorMessage = QString("设备连接失败: %1").arg(device_err);
                break;
            }
            
            // 2. 建立 lockdown 连接（带超时）
            QElapsedTimer timer;
            timer.start();
            
            lockdownd_error_t lockdown_err = LOCKDOWN_E_UNKNOWN_ERROR;
            while (timer.elapsed() < timeoutMs) {
                lockdown_err = lockdownd_client_new_with_handshake(device, &lockdown, "QtDeviceManager");
                if (lockdown_err == LOCKDOWN_E_SUCCESS) {
                    break;
                }
                
                if (lockdown_err == LOCKDOWN_E_PAIRING_DIALOG_PENDING) {
                    // 等待用户在设备上确认信任
                    QThread::msleep(500);
                    continue;
                }
                
                if (lockdown_err == LOCKDOWN_E_USER_DENIED_PAIRING) {
                    result.error = ConnectionError::PairingFailed;
                    result.errorMessage = "用户拒绝了配对请求";
                    break;
                }
                
                QThread::msleep(100);
            }
            
            if (lockdown_err != LOCKDOWN_E_SUCCESS) {
                if (timer.elapsed() >= timeoutMs) {
                    result.error = ConnectionError::Timeout;
                    result.errorMessage = "连接超时";
                } else if (result.error == ConnectionError::None) {
                    result.error = ConnectionError::HandshakeFailed;
                    result.errorMessage = QString("握手失败: %1").arg(lockdown_err);
                }
                break;
            }
            
            // 3. 获取设备名称验证连接
            plist_t name_plist = nullptr;
            if (lockdownd_get_value(lockdown, nullptr, "DeviceName", &name_plist) == LOCKDOWN_E_SUCCESS && name_plist) {
                char *name_str = nullptr;
                plist_get_string_val(name_plist, &name_str);
                if (name_str) {
                    result.deviceName = QString::fromUtf8(name_str);
                    free(name_str);
                }
                plist_free(name_plist);
            }
            
            result.success = true;
            
        } while (false);
        
        // 清理资源
        if (!result.success) {
            if (lockdown) lockdownd_client_free(lockdown);
            if (device) idevice_free(device);
        }
        
        return result;
    }
};
```

### 2. 文件操作错误处理

```cpp
class SafeFileManager
{
public:
    enum class FileError {
        None,
        FileNotFound,
        PermissionDenied,
        DiskFull,
        ConnectionLost,
        InvalidPath,
        UnknownError
    };
    
    struct FileOperationResult {
        bool success;
        FileError error;
        QString errorMessage;
        qint64 bytesTransferred;
    };
    
    static FileOperationResult uploadFileWithRetry(
        afc_client_t afc,
        const QString &localPath,
        const QString &remotePath,
        int maxRetries = 3)
    {
        FileOperationResult result;
        result.success = false;
        result.error = FileError::None;
        result.bytesTransferred = 0;
        
        QFile localFile(localPath);
        if (!localFile.exists()) {
            result.error = FileError::FileNotFound;
            result.errorMessage = "本地文件不存在";
            return result;
        }
        
        if (!localFile.open(QIODevice::ReadOnly)) {
            result.error = FileError::PermissionDenied;
            result.errorMessage = "无法读取本地文件";
            return result;
        }
        
        qint64 totalSize = localFile.size();
        const int bufferSize = 65536; // 64KB 缓冲区
        
        for (int attempt = 0; attempt < maxRetries; attempt++) {
            uint64_t afc_file = 0;
            
            // 创建远程文件
            afc_error_t afc_err = afc_file_open(afc, remotePath.toUtf8().constData(), 
                                               AFC_FOPEN_WRONLY, &afc_file);
            if (afc_err != AFC_E_SUCCESS) {
                if (afc_err == AFC_E_PERM_DENIED) {
                    result.error = FileError::PermissionDenied;
                    result.errorMessage = "远程文件写入权限被拒绝";
                } else if (afc_err == AFC_E_NO_SPACE_LEFT) {
                    result.error = FileError::DiskFull;
                    result.errorMessage = "设备存储空间不足";
                } else {
                    result.error = FileError::InvalidPath;
                    result.errorMessage = QString("无法创建远程文件: %1").arg(afc_err);
                }
                
                if (attempt == maxRetries - 1) {
                    break;
                }
                
                // 重试前等待
                QThread::msleep(1000 * (attempt + 1));
                continue;
            }
            
            // 开始传输
            localFile.seek(0);
            result.bytesTransferred = 0;
            bool transferSuccess = true;
            
            while (!localFile.atEnd()) {
                QByteArray buffer = localFile.read(bufferSize);
                if (buffer.isEmpty()) {
                    break;
                }
                
                uint32_t bytesWritten = 0;
                afc_err = afc_file_write(afc_file, buffer.constData(), 
                                        buffer.size(), &bytesWritten);
                
                if (afc_err != AFC_E_SUCCESS || bytesWritten != static_cast<uint32_t>(buffer.size())) {
                    transferSuccess = false;
                    if (afc_err == AFC_E_NO_SPACE_LEFT) {
                        result.error = FileError::DiskFull;
                        result.errorMessage = "传输过程中设备存储空间不足";
                    } else {
                        result.error = FileError::ConnectionLost;
                        result.errorMessage = "传输过程中连接丢失";
                    }
                    break;
                }
                
                result.bytesTransferred += bytesWritten;
                
                // 发送进度更新
                int progress = static_cast<int>((result.bytesTransferred * 100) / totalSize);
                // emit transferProgress(result.bytesTransferred, totalSize);
            }
            
            afc_file_close(afc, afc_file);
            
            if (transferSuccess) {
                result.success = true;
                break;
            }
            
            // 清理部分传输的文件
            afc_remove_path(afc, remotePath.toUtf8().constData());
            
            if (attempt < maxRetries - 1) {
                QThread::msleep(2000); // 重试前等待2秒
            }
        }
        
        return result;
    }
};
```

## 内存管理最佳实践

### 1. RAII 资源管理

```cpp
// 自动资源管理类
class DeviceConnection
{
public:
    DeviceConnection(const QString &udid) 
        : m_device(nullptr), m_lockdown(nullptr), m_valid(false)
    {
        if (idevice_new(&m_device, udid.toUtf8().constData()) == IDEVICE_E_SUCCESS) {
            if (lockdownd_client_new_with_handshake(m_device, &m_lockdown, "QtApp") == LOCKDOWN_E_SUCCESS) {
                m_valid = true;
            } else {
                idevice_free(m_device);
                m_device = nullptr;
            }
        }
    }
    
    ~DeviceConnection()
    {
        if (m_lockdown) {
            lockdownd_client_free(m_lockdown);
        }
        if (m_device) {
            idevice_free(m_device);
        }
    }
    
    // 禁止复制
    DeviceConnection(const DeviceConnection&) = delete;
    DeviceConnection& operator=(const DeviceConnection&) = delete;
    
    // 支持移动
    DeviceConnection(DeviceConnection&& other) noexcept
        : m_device(other.m_device), m_lockdown(other.m_lockdown), m_valid(other.m_valid)
    {
        other.m_device = nullptr;
        other.m_lockdown = nullptr;
        other.m_valid = false;
    }
    
    bool isValid() const { return m_valid; }
    idevice_t device() const { return m_device; }
    lockdownd_client_t lockdown() const { return m_lockdown; }
    
private:
    idevice_t m_device;
    lockdownd_client_t m_lockdown;
    bool m_valid;
};

// 智能指针风格的 plist 管理
class PlistGuard
{
public:
    explicit PlistGuard(plist_t plist = nullptr) : m_plist(plist) {}
    
    ~PlistGuard()
    {
        if (m_plist) {
            plist_free(m_plist);
        }
    }
    
    PlistGuard(const PlistGuard&) = delete;
    PlistGuard& operator=(const PlistGuard&) = delete;
    
    PlistGuard(PlistGuard&& other) noexcept : m_plist(other.m_plist)
    {
        other.m_plist = nullptr;
    }
    
    PlistGuard& operator=(PlistGuard&& other) noexcept
    {
        if (this != &other) {
            if (m_plist) {
                plist_free(m_plist);
            }
            m_plist = other.m_plist;
            other.m_plist = nullptr;
        }
        return *this;
    }
    
    plist_t get() const { return m_plist; }
    plist_t release() { plist_t p = m_plist; m_plist = nullptr; return p; }
    void reset(plist_t plist = nullptr) 
    { 
        if (m_plist) plist_free(m_plist); 
        m_plist = plist; 
    }
    
    operator bool() const { return m_plist != nullptr; }
    
private:
    plist_t m_plist;
};
```

### 2. 线程安全

```cpp
class ThreadSafeDeviceManager
{
public:
    ThreadSafeDeviceManager() : m_isShuttingDown(false) {}
    
    ~ThreadSafeDeviceManager()
    {
        shutdown();
    }
    
    void shutdown()
    {
        {
            QWriteLocker locker(&m_lock);
            m_isShuttingDown = true;
            m_devices.clear();
        }
        
        // 等待所有工作线程完成
        for (auto& future : m_activeTasks) {
            if (future.isRunning()) {
                future.waitForFinished();
            }
        }
        m_activeTasks.clear();
    }
    
    QFuture<DeviceInfo> getDeviceInfoAsync(const QString &udid)
    {
        QReadLocker locker(&m_lock);
        if (m_isShuttingDown) {
            return QFuture<DeviceInfo>();
        }
        
        auto task = QtConcurrent::run([this, udid]() -> DeviceInfo {
            DeviceConnection conn(udid);
            if (!conn.isValid()) {
                return DeviceInfo(); // 返回空的设备信息
            }
            
            DeviceInfoManager infoManager;
            return infoManager.getDeviceInfo(conn.device(), conn.lockdown());
        });
        
        m_activeTasks.append(task);
        return task;
    }
    
    void addDevice(const QString &udid, const QString &name)
    {
        QWriteLocker locker(&m_lock);
        if (!m_isShuttingDown) {
            m_devices[udid] = name;
        }
    }
    
    void removeDevice(const QString &udid)
    {
        QWriteLocker locker(&m_lock);
        m_devices.remove(udid);
    }
    
    QStringList getConnectedDevices() const
    {
        QReadLocker locker(&m_lock);
        return m_devices.keys();
    }
    
private:
    mutable QReadWriteLock m_lock;
    QHash<QString, QString> m_devices;
    QList<QFuture<DeviceInfo>> m_activeTasks;
    bool m_isShuttingDown;
};
```

## 性能优化建议

### 1. 连接池管理

```cpp
class DeviceConnectionPool
{
public:
    static DeviceConnectionPool& instance()
    {
        static DeviceConnectionPool pool;
        return pool;
    }
    
    struct PooledConnection {
        std::unique_ptr<DeviceConnection> connection;
        QDateTime lastUsed;
        bool inUse;
        
        PooledConnection() : inUse(false) {}
    };
    
    std::shared_ptr<DeviceConnection> borrowConnection(const QString &udid)
    {
        QMutexLocker locker(&m_mutex);
        
        // 查找现有连接
        auto it = m_connections.find(udid);
        if (it != m_connections.end()) {
            auto& pooled = it.value();
            if (!pooled.inUse && pooled.connection && pooled.connection->isValid()) {
                pooled.inUse = true;
                pooled.lastUsed = QDateTime::currentDateTime();
                return std::shared_ptr<DeviceConnection>(
                    pooled.connection.get(),
                    [this, udid](DeviceConnection* conn) {
                        this->returnConnection(udid, conn);
                    }
                );
            }
        }
        
        // 创建新连接
        auto newConn = std::make_unique<DeviceConnection>(udid);
        if (newConn->isValid()) {
            auto rawPtr = newConn.get();
            
            PooledConnection pooled;
            pooled.connection = std::move(newConn);
            pooled.lastUsed = QDateTime::currentDateTime();
            pooled.inUse = true;
            
            m_connections[udid] = std::move(pooled);
            
            return std::shared_ptr<DeviceConnection>(
                rawPtr,
                [this, udid](DeviceConnection* conn) {
                    this->returnConnection(udid, conn);
                }
            );
        }
        
        return nullptr;
    }
    
    void cleanupIdleConnections()
    {
        QMutexLocker locker(&m_mutex);
        const int maxIdleMinutes = 10;
        QDateTime now = QDateTime::currentDateTime();
        
        auto it = m_connections.begin();
        while (it != m_connections.end()) {
            const auto& pooled = it.value();
            if (!pooled.inUse && 
                pooled.lastUsed.secsTo(now) > maxIdleMinutes * 60) {
                it = m_connections.erase(it);
            } else {
                ++it;
            }
        }
    }
    
private:
    DeviceConnectionPool()
    {
        // 定期清理空闲连接
        m_cleanupTimer.setInterval(5 * 60 * 1000); // 5分钟
        m_cleanupTimer.setSingleShot(false);
        QObject::connect(&m_cleanupTimer, &QTimer::timeout,
                        [this]() { cleanupIdleConnections(); });
        m_cleanupTimer.start();
    }
    
    void returnConnection(const QString &udid, DeviceConnection* conn)
    {
        QMutexLocker locker(&m_mutex);
        auto it = m_connections.find(udid);
        if (it != m_connections.end()) {
            it.value().inUse = false;
            it.value().lastUsed = QDateTime::currentDateTime();
        }
    }
    
    QMutex m_mutex;
    QHash<QString, PooledConnection> m_connections;
    QTimer m_cleanupTimer;
};
```

### 2. 缓存策略

```cpp
class DeviceInfoCache
{
public:
    struct CachedInfo {
        DeviceInfo info;
        QDateTime timestamp;
        bool isValid;
        
        CachedInfo() : isValid(false) {}
    };
    
    static DeviceInfoCache& instance()
    {
        static DeviceInfoCache cache;
        return cache;
    }
    
    bool getDeviceInfo(const QString &udid, DeviceInfo &info, int maxAgeSeconds = 300)
    {
        QReadLocker locker(&m_lock);
        
        auto it = m_cache.find(udid);
        if (it != m_cache.end()) {
            const auto& cached = it.value();
            if (cached.isValid && 
                cached.timestamp.secsTo(QDateTime::currentDateTime()) < maxAgeSeconds) {
                info = cached.info;
                return true;
            }
        }
        
        return false;
    }
    
    void setDeviceInfo(const QString &udid, const DeviceInfo &info)
    {
        QWriteLocker locker(&m_lock);
        
        CachedInfo cached;
        cached.info = info;
        cached.timestamp = QDateTime::currentDateTime();
        cached.isValid = true;
        
        m_cache[udid] = cached;
    }
    
    void invalidateDevice(const QString &udid)
    {
        QWriteLocker locker(&m_lock);
        m_cache.remove(udid);
    }
    
    void clearExpiredEntries(int maxAgeSeconds = 600)
    {
        QWriteLocker locker(&m_lock);
        QDateTime now = QDateTime::currentDateTime();
        
        auto it = m_cache.begin();
        while (it != m_cache.end()) {
            if (it.value().timestamp.secsTo(now) > maxAgeSeconds) {
                it = m_cache.erase(it);
            } else {
                ++it;
            }
        }
    }
    
private:
    mutable QReadWriteLock m_lock;
    QHash<QString, CachedInfo> m_cache;
};
```

## 调试与日志

### 1. 结构化日志

```cpp
class DeviceLogger
{
public:
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };
    
    static DeviceLogger& instance()
    {
        static DeviceLogger logger;
        return logger;
    }
    
    void log(LogLevel level, const QString &category, const QString &message, 
             const QString &udid = QString())
    {
        QJsonObject logEntry;
        logEntry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        logEntry["level"] = levelToString(level);
        logEntry["category"] = category;
        logEntry["message"] = message;
        
        if (!udid.isEmpty()) {
            logEntry["device_udid"] = udid;
        }
        
        // 添加调用堆栈信息（调试模式下）
#ifdef QT_DEBUG
        logEntry["thread_id"] = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
#endif
        
        QJsonDocument doc(logEntry);
        QString logLine = doc.toJson(QJsonDocument::Compact);
        
        // 输出到控制台
        qDebug().noquote() << logLine;
        
        // 写入文件
        writeToFile(logLine);
        
        // 发送到远程日志服务（如果配置了）
        if (m_remoteLoggingEnabled) {
            sendToRemoteLogger(logEntry);
        }
    }
    
    void setLogFile(const QString &filePath)
    {
        QMutexLocker locker(&m_mutex);
        m_logFile.reset(new QFile(filePath));
        if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
            m_logStream.reset(new QTextStream(m_logFile.get()));
        }
    }
    
    void enableRemoteLogging(const QString &endpoint)
    {
        m_remoteEndpoint = endpoint;
        m_remoteLoggingEnabled = true;
    }
    
private:
    DeviceLogger() : m_remoteLoggingEnabled(false) {}
    
    QString levelToString(LogLevel level)
    {
        switch (level) {
        case Debug: return "DEBUG";
        case Info: return "INFO";
        case Warning: return "WARNING";
        case Error: return "ERROR";
        case Critical: return "CRITICAL";
        }
        return "UNKNOWN";
    }
    
    void writeToFile(const QString &logLine)
    {
        QMutexLocker locker(&m_mutex);
        if (m_logStream) {
            *m_logStream << logLine << Qt::endl;
            m_logStream->flush();
        }
    }
    
    void sendToRemoteLogger(const QJsonObject &logEntry)
    {
        // 异步发送日志到远程服务器
        QtConcurrent::run([this, logEntry]() {
            QNetworkAccessManager manager;
            QNetworkRequest request(QUrl(m_remoteEndpoint));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            
            QJsonDocument doc(logEntry);
            auto reply = manager.post(request, doc.toJson());
            
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            
            reply->deleteLater();
        });
    }
    
    QMutex m_mutex;
    std::unique_ptr<QFile> m_logFile;
    std::unique_ptr<QTextStream> m_logStream;
    bool m_remoteLoggingEnabled;
    QString m_remoteEndpoint;
};

// 便捷宏
#define LOG_DEBUG(category, message, udid) \
    DeviceLogger::instance().log(DeviceLogger::Debug, category, message, udid)

#define LOG_INFO(category, message, udid) \
    DeviceLogger::instance().log(DeviceLogger::Info, category, message, udid)

#define LOG_WARNING(category, message, udid) \
    DeviceLogger::instance().log(DeviceLogger::Warning, category, message, udid)

#define LOG_ERROR(category, message, udid) \
    DeviceLogger::instance().log(DeviceLogger::Error, category, message, udid)
```

### 2. 性能监控

```cpp
class PerformanceMonitor
{
public:
    class Timer
    {
    public:
        Timer(const QString &operation, const QString &udid = QString())
            : m_operation(operation), m_udid(udid)
        {
            m_timer.start();
        }
        
        ~Timer()
        {
            qint64 elapsed = m_timer.elapsed();
            PerformanceMonitor::instance().recordOperation(m_operation, elapsed, m_udid);
        }
        
    private:
        QString m_operation;
        QString m_udid;
        QElapsedTimer m_timer;
    };
    
    static PerformanceMonitor& instance()
    {
        static PerformanceMonitor monitor;
        return monitor;
    }
    
    void recordOperation(const QString &operation, qint64 durationMs, 
                        const QString &udid = QString())
    {
        QMutexLocker locker(&m_mutex);
        
        OperationStats& stats = m_operationStats[operation];
        stats.totalCount++;
        stats.totalDuration += durationMs;
        stats.lastDuration = durationMs;
        
        if (durationMs > stats.maxDuration) {
            stats.maxDuration = durationMs;
        }
        
        if (stats.minDuration == 0 || durationMs < stats.minDuration) {
            stats.minDuration = durationMs;
        }
        
        // 记录到日志
        if (durationMs > 5000) { // 超过5秒的操作记录为警告
            LOG_WARNING("Performance", 
                       QString("Slow operation: %1 took %2ms").arg(operation).arg(durationMs),
                       udid);
        }
    }
    
    QJsonObject getStatistics() const
    {
        QReadLocker locker(&m_mutex);
        QJsonObject stats;
        
        for (auto it = m_operationStats.begin(); it != m_operationStats.end(); ++it) {
            QJsonObject opStats;
            const auto& data = it.value();
            
            opStats["total_count"] = static_cast<int>(data.totalCount);
            opStats["total_duration_ms"] = static_cast<int>(data.totalDuration);
            opStats["avg_duration_ms"] = static_cast<int>(data.totalDuration / data.totalCount);
            opStats["min_duration_ms"] = static_cast<int>(data.minDuration);
            opStats["max_duration_ms"] = static_cast<int>(data.maxDuration);
            opStats["last_duration_ms"] = static_cast<int>(data.lastDuration);
            
            stats[it.key()] = opStats;
        }
        
        return stats;
    }
    
private:
    struct OperationStats {
        qint64 totalCount = 0;
        qint64 totalDuration = 0;
        qint64 minDuration = 0;
        qint64 maxDuration = 0;
        qint64 lastDuration = 0;
    };
    
    mutable QReadWriteLock m_mutex;
    QHash<QString, OperationStats> m_operationStats;
};

// 便捷宏
#define PERF_TIMER(operation, udid) \
    PerformanceMonitor::Timer _perf_timer(operation, udid)
```

## 安全注意事项

### 1. 证书管理

```cpp
class SecurePairingManager
{
public:
    static bool pairDevice(const QString &udid, const QString &pairingDataPath)
    {
        // 1. 检查现有配对
        if (isDevicePaired(udid)) {
            LOG_INFO("Pairing", "设备已配对", udid);
            return true;
        }
        
        // 2. 安全存储配对数据
        QString securePath = getSecurePairingPath();
        if (!QDir(securePath).exists()) {
            if (!QDir().mkpath(securePath)) {
                LOG_ERROR("Pairing", "无法创建安全存储目录", udid);
                return false;
            }
            
            // 设置目录权限（仅所有者可访问）
#ifndef Q_OS_WIN
            QFile::setPermissions(securePath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
#endif
        }
        
        // 3. 执行配对
        DeviceConnection conn(udid);
        if (!conn.isValid()) {
            LOG_ERROR("Pairing", "无法连接到设备进行配对", udid);
            return false;
        }
        
        // 4. 验证配对结果
        if (verifyPairing(conn.lockdown())) {
            LOG_INFO("Pairing", "设备配对成功", udid);
            return true;
        }
        
        LOG_ERROR("Pairing", "设备配对验证失败", udid);
        return false;
    }
    
    static void clearPairingData(const QString &udid)
    {
        QString pairingFile = QString("%1/%2.plist").arg(getSecurePairingPath(), udid);
        if (QFile::exists(pairingFile)) {
            // 安全删除文件
            QFile file(pairingFile);
            if (file.open(QIODevice::WriteOnly)) {
                // 用随机数据覆盖文件内容
                QByteArray randomData(file.size(), 0);
                QRandomGenerator::global()->fillRange(reinterpret_cast<quint32*>(randomData.data()), 
                                                     randomData.size() / sizeof(quint32));
                file.write(randomData);
            }
            file.remove();
            
            LOG_INFO("Security", "已清除设备配对数据", udid);
        }
    }
    
private:
    static QString getSecurePairingPath()
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/secure_pairing";
    }
    
    static bool isDevicePaired(const QString &u
