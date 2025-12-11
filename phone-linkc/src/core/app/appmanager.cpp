#include "appmanager.h"
#include "../../platform/libimobiledevice_dynamic.h"
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>

namespace {
    QString formatSize(uint64_t bytes) {
        if (bytes == 0) return "0 B";
        const char* suffixes[] = {"B", "KB", "MB", "GB", "TB"};
        int i = 0;
        double dblBytes = bytes;
        while (dblBytes >= 1024 && i < 4) {
            dblBytes /= 1024.0;
            i++;
        }
        return QString::asprintf("%.2f %s", dblBytes, suffixes[i]);
    }
}

AppManager::AppManager(QObject *parent)
    : QObject(parent)
    , m_connected(false)
    , m_device(nullptr)
    , m_lockdown(nullptr)
    , m_instproxy(nullptr)
{
}

AppManager::~AppManager()
{
    disconnectFromDevice();
}

bool AppManager::connectToDevice(const QString &udid)
{
    if (m_connected && m_udid == udid) {
        return true;
    }

    disconnectFromDevice();

    m_udid = udid;
    return initClient();
}

void AppManager::disconnectFromDevice()
{
    cleanup();
    m_connected = false;
    m_udid.clear();
}

void AppManager::cleanup()
{
    auto& lib = LibimobiledeviceDynamic::instance();
    if (!lib.isInitialized()) return;

    if (m_instproxy) {
        lib.instproxy_client_free(static_cast<instproxy_client_t>(m_instproxy));
        m_instproxy = nullptr;
    }

    if (m_lockdown) {
        lib.lockdownd_client_free(static_cast<lockdownd_client_t>(m_lockdown));
        m_lockdown = nullptr;
    }

    if (m_device) {
        lib.idevice_free(static_cast<idevice_t>(m_device));
        m_device = nullptr;
    }
}

bool AppManager::initClient()
{
    auto& lib = LibimobiledeviceDynamic::instance();
    if (!lib.isInitialized()) {
        m_lastError = "动态库未初始化";
        return false;
    }

    // 1. 创建设备对象
    idevice_t device = nullptr;
    if (lib.idevice_new(&device, m_udid.toUtf8().constData()) != IDEVICE_E_SUCCESS) {
        m_lastError = "无法连接设备: " + m_udid;
        return false;
    }
    m_device = device;

    // 2. 创建 lockdown 客户端
    lockdownd_client_t lockdown = nullptr;
    if (lib.lockdownd_client_new_with_handshake(device, &lockdown, "phone-linkc") != LOCKDOWN_E_SUCCESS) {
        m_lastError = "无法建立 lockdown 连接";
        lib.idevice_free(device);
        m_device = nullptr;
        return false;
    }
    m_lockdown = lockdown;

    // 3. 启动 installation_proxy 服务
    lockdownd_service_descriptor_t service = nullptr;
    if (lib.lockdownd_start_service(lockdown, "com.apple.mobile.installation_proxy", &service) != LOCKDOWN_E_SUCCESS || !service) {
        m_lastError = "无法启动 installation_proxy 服务";
        lib.lockdownd_client_free(lockdown);
        m_lockdown = nullptr;
        lib.idevice_free(device);
        m_device = nullptr;
        return false;
    }

    // 4. 创建 instproxy 客户端
    instproxy_client_t instproxy = nullptr;
    if (lib.instproxy_client_new(device, service, &instproxy) != INSTPROXY_E_SUCCESS) {
        m_lastError = "无法创建 application proxy 客户端";
        lib.lockdownd_service_descriptor_free(service);
        lib.lockdownd_client_free(lockdown);
        m_lockdown = nullptr;
        lib.idevice_free(device);
        m_device = nullptr;
        return false;
    }
    m_instproxy = instproxy;

    lib.lockdownd_service_descriptor_free(service);
    m_connected = true;
    return true;
}

QVector<AppInfo> AppManager::listApps(bool includeSize)
{
    QElapsedTimer totalTimer;
    totalTimer.start();
    
    QVector<AppInfo> apps;
    if (!m_connected || !m_instproxy) {
        m_lastError = "未连接设备";
        emit errorOccurred(m_lastError);
        return apps;
    }

    auto& lib = LibimobiledeviceDynamic::instance();
    
    QElapsedTimer optionsTimer;
    optionsTimer.start();
    
    // 设置选项：查询用户应用
    plist_t client_opts = lib.plist_new_dict();
    qDebug() << "[性能] plist_new_dict 耗时:" << optionsTimer.elapsed() << "ms";
    
    optionsTimer.restart();
    lib.plist_dict_set_item(client_opts, "ApplicationType", lib.plist_new_string("User"));
    qDebug() << "[性能] 设置 ApplicationType 耗时:" << optionsTimer.elapsed() << "ms";
    
    // 设置返回属性
    optionsTimer.restart();
    plist_t return_attributes = lib.plist_new_array();
    qDebug() << "[性能] plist_new_array 耗时:" << optionsTimer.elapsed() << "ms";
    
    optionsTimer.restart();
    // 基本属性（总是包含）
    lib.plist_array_append_item(return_attributes, lib.plist_new_string("CFBundleIdentifier"));
    lib.plist_array_append_item(return_attributes, lib.plist_new_string("CFBundleDisplayName"));
    lib.plist_array_append_item(return_attributes, lib.plist_new_string("CFBundleName"));
    lib.plist_array_append_item(return_attributes, lib.plist_new_string("CFBundleShortVersionString"));
    
    // 磁盘使用信息（可选，这是最耗时的部分）
    if (includeSize) {
        lib.plist_array_append_item(return_attributes, lib.plist_new_string("StaticDiskUsage"));
        lib.plist_array_append_item(return_attributes, lib.plist_new_string("DynamicDiskUsage"));
        qDebug() << "[性能] 添加所有属性（含磁盘使用）耗时:" << optionsTimer.elapsed() << "ms";
    } else {
        qDebug() << "[性能] 添加基本属性（不含磁盘使用）耗时:" << optionsTimer.elapsed() << "ms";
    }
    
    optionsTimer.restart();
    lib.plist_dict_set_item(client_opts, "ReturnAttributes", return_attributes);
    qDebug() << "[性能] 设置 ReturnAttributes 耗时:" << optionsTimer.elapsed() << "ms";
    
    plist_t apps_plist = nullptr;
    
    // 获取应用列表
    optionsTimer.restart();
    if (lib.instproxy_browse(static_cast<instproxy_client_t>(m_instproxy), client_opts, &apps_plist) != INSTPROXY_E_SUCCESS) {
        m_lastError = "无法获取应用列表";
        emit errorOccurred(m_lastError);
        lib.plist_free(client_opts);
        return apps;
    }
    
    qDebug() << "[性能] instproxy_browse 耗时:" << optionsTimer.elapsed() << "ms";
    
    lib.plist_free(client_opts);
    
    if (!apps_plist) {
        return apps;
    }
    
    optionsTimer.restart();
    uint32_t count = lib.plist_array_get_size(apps_plist);
    qDebug() << "[性能] 获取应用数量 (" << count << " 个) 耗时:" << optionsTimer.elapsed() << "ms";
    
    optionsTimer.restart();
    for (uint32_t i = 0; i < count; i++) {
        plist_t app_node = lib.plist_array_get_item(apps_plist, i);
        if (!app_node) continue;
        
        AppInfo info;
        info.type = "User";
        
#ifdef QT_DEBUG
        // DEBUG: 打印完整应用属性
        qDebug() << "--- App Node Properties ---";
        if (lib.plist_dict_new_iter && lib.plist_dict_next_item) {
            plist_dict_iter iter = nullptr;
            lib.plist_dict_new_iter(app_node, &iter);
            if (iter) {
                char *key = nullptr;
                plist_t val_node = nullptr;
                do {
                    lib.plist_dict_next_item(app_node, iter, &key, &val_node);
                    if (key) {
                        QString valStr;
                        plist_type t = lib.plist_get_node_type(val_node);
                        if (t == PLIST_STRING) {
                            const char *s = nullptr;
                            if (lib.plist_get_string_ptr) {
                                s = lib.plist_get_string_ptr(val_node, nullptr);
                            }
                            if (s) valStr = QString::fromUtf8(s);
                            else valStr = "(string)";
                        } else if (t == PLIST_BOOLEAN) {
                            uint8_t b = 0;
                            lib.plist_get_bool_val(val_node, &b);
                            valStr = b ? "true" : "false";
                        } else if (t == PLIST_UINT) {
                             uint64_t u = 0;
                             lib.plist_get_uint_val(val_node, &u);
                             valStr = QString::number(u);
                        } else if (t == PLIST_ARRAY) {
                            uint32_t size = lib.plist_array_get_size(val_node);
                            valStr = QString("(Array, size=%1)").arg(size);
                        } else if (t == PLIST_DICT) {
                             valStr = "(Dictionary)";
                        } else {
                             valStr = QString("(type %1)").arg(t);
                        }
                        qDebug() << "  Key:" << key << "Val:" << valStr;
                        
                        // 注意: key 是由 DLL 分配的内存。
                        // 由于 Windows 跨模块 CRT 堆隔离问题，在 EXE 中调用 free(key) 可能会导致崩溃。
                        // 且我们没有导出 DLL 的 free 函数。
                        // 因此这里为了稳定性暂时不释放 key，这会造成少量内存泄漏，但在调试场景下可接受。
                        // free(key);
                    }
                } while (val_node != nullptr);
                
                // 同理，iter 也不释放
                // free(iter);
            }
        }
        qDebug() << "---------------------------";
#endif

        // 获取包名
        plist_t id_node = lib.plist_dict_get_item(app_node, "CFBundleIdentifier");
        if (id_node) {
            const char *val = nullptr;
            if (lib.plist_get_string_ptr) {
                val = lib.plist_get_string_ptr(id_node, nullptr);
            } else {
                // Fallback if get_string_ptr is not available (though unlikely if init succeeded)
                // Avoid using free() on Windows with potentially different CRTs
                char *temp = nullptr;
                lib.plist_get_string_val(id_node, &temp);
                val = temp;
                // Note: We are leaking 'temp' here intentionally to avoid crashing due to CRT mismatch.
                // In a proper production env we'd need a way to free it or ensure CRT match.
                // But given we prefer plist_get_string_ptr, this path shouldn't be taken.
            }
            
            if (val) {
                info.bundleId = QString::fromUtf8(val);
                // No free(val) needed for plist_get_string_ptr
            }
        }
        
        // 获取显示名称
        plist_t name_node = lib.plist_dict_get_item(app_node, "CFBundleDisplayName");
        if (!name_node) {
             name_node = lib.plist_dict_get_item(app_node, "CFBundleName");
        }
        
        if (name_node) {
            const char *val = nullptr;
            if (lib.plist_get_string_ptr) {
                val = lib.plist_get_string_ptr(name_node, nullptr);
            } else {
                char *temp = nullptr;
                lib.plist_get_string_val(name_node, &temp);
                val = temp;
            }

            if (val) {
                info.name = QString::fromUtf8(val);
            }
        } else {
            info.name = info.bundleId; // Fallback
        }
        
        // 获取版本
        plist_t ver_node = lib.plist_dict_get_item(app_node, "CFBundleShortVersionString");
        if (ver_node) {
            const char *val = nullptr;
            if (lib.plist_get_string_ptr) {
                val = lib.plist_get_string_ptr(ver_node, nullptr);
            } else {
                char *temp = nullptr;
                lib.plist_get_string_val(ver_node, &temp);
                val = temp;
            }

            if (val) {
                info.version = QString::fromUtf8(val);
            }
        }

        // 获取应用大小 (StaticDiskUsage)
        plist_t static_size_node = lib.plist_dict_get_item(app_node, "StaticDiskUsage");
        if (static_size_node) {
            uint64_t size = 0;
            lib.plist_get_uint_val(static_size_node, &size);
            info.appSize = formatSize(size);
        } else {
            info.appSize = "未知";
        }

        // 获取文档大小 (DynamicDiskUsage)
        plist_t dynamic_size_node = lib.plist_dict_get_item(app_node, "DynamicDiskUsage");
        if (dynamic_size_node) {
            uint64_t size = 0;
            lib.plist_get_uint_val(dynamic_size_node, &size);
            info.docSize = formatSize(size);
        } else {
            info.docSize = "未知";
        }
        
        // TODO: 获取图标 (StaticDiskImage?)
        // 图标通常在 .app 目录下的 Icon 文件中，需要通过 AFC 读取。
        // 这比较复杂，因为需要知道 .app 的路径，然后用 AFC 读取。
        // instproxy_browse 返回的字典中有一个 "Path" 键，指向 .app 目录。
        
        apps.append(info);
    }
    qDebug() << "[性能] 解析所有应用信息耗时:" << optionsTimer.elapsed() << "ms";
    
    lib.plist_free(apps_plist);
    
    qDebug() << "[性能] listApps 总耗时:" << totalTimer.elapsed() << "ms";
    return apps;
}
void AppManager::listAppsAsync()
{
    if (!m_connected) {
        emit errorOccurred("设备未连接");
        emit appsLoaded(QVector<AppInfo>());
        return;
    }
    
    // 第一阶段：快速加载基本信息（不含磁盘使用）
    QtConcurrent::run([this]() {
        qDebug() << "[分阶段加载] 第一阶段开始：加载基本信息";
        QElapsedTimer stage1Timer;
        stage1Timer.start();
        
        // 不包含磁盘使用信息，速度快
        QVector<AppInfo> apps = listApps(false);
        
        qDebug() << "[分阶段加载] 第一阶段完成，耗时:" << stage1Timer.elapsed() << "ms，应用数量:" << apps.size();
        
        // 返回主线程发送信号
        emit appsLoaded(apps);
        
        // 第二阶段：后台异步获取每个应用的磁盘使用信息
        qDebug() << "[分阶段加载] 第二阶段开始：加载磁盘使用信息";
        QElapsedTimer stage2Timer;
        stage2Timer.start();
        
        // for (const AppInfo &app : apps) {
        //     getAppSizeAsync(app.bundleId);
        // }
        
        qDebug() << "[分阶段加载] 第二阶段触发完成，总耗时:" << stage2Timer.elapsed() << "ms";
    });
}

void AppManager::getAppSizeAsync(const QString &bundleId)
{
    if (!m_connected || !m_instproxy) {
        return;
    }
    
    // 为单个应用获取磁盘使用信息
    QtConcurrent::run([this, bundleId]() {
        auto& lib = LibimobiledeviceDynamic::instance();
        
        // 创建选项：只查询指定的应用
        plist_t client_opts = lib.plist_new_dict();
        lib.plist_dict_set_item(client_opts, "BundleIDs", lib.plist_new_array());
        plist_t bundle_ids = lib.plist_dict_get_item(client_opts, "BundleIDs");
        lib.plist_array_append_item(bundle_ids, lib.plist_new_string(bundleId.toUtf8().constData()));
        
        // 只请求磁盘使用信息
        plist_t return_attributes = lib.plist_new_array();
        lib.plist_array_append_item(return_attributes, lib.plist_new_string("StaticDiskUsage"));
        lib.plist_array_append_item(return_attributes, lib.plist_new_string("DynamicDiskUsage"));
        lib.plist_dict_set_item(client_opts, "ReturnAttributes", return_attributes);
        
        plist_t apps_plist = nullptr;
        
        // 获取应用信息
        if (lib.instproxy_browse(static_cast<instproxy_client_t>(m_instproxy), client_opts, &apps_plist) == INSTPROXY_E_SUCCESS) {
            if (apps_plist && lib.plist_get_node_type(apps_plist) == PLIST_ARRAY) {
                uint32_t count = lib.plist_array_get_size(apps_plist);
                if (count > 0) {
                    plist_t app_node = lib.plist_array_get_item(apps_plist, 0);
                    
                    QString appSize;
                    QString docSize;
                    
                    // 获取应用大小
                    plist_t static_size_node = lib.plist_dict_get_item(app_node, "StaticDiskUsage");
                    if (static_size_node) {
                        uint64_t size = 0;
                        lib.plist_get_uint_val(static_size_node, &size);
                        appSize = formatSize(size);
                    }
                    
                    // 获取文档大小
                    plist_t dynamic_size_node = lib.plist_dict_get_item(app_node, "DynamicDiskUsage");
                    if (dynamic_size_node) {
                        uint64_t size = 0;
                        lib.plist_get_uint_val(dynamic_size_node, &size);
                        docSize = formatSize(size);
                    }
                    
                    // 发送更新信号
                    emit appSizeUpdated(bundleId, appSize, docSize);
                }
                lib.plist_free(apps_plist);
            }
        }
        
        lib.plist_free(client_opts);
    });
}

bool AppManager::uninstallApp(const QString &bundleId)
{
    if (!m_connected || !m_instproxy) {
        m_lastError = "未连接设备";
        emit errorOccurred(m_lastError);
        return false;
    }
    
    auto& lib = LibimobiledeviceDynamic::instance();
    
    if (lib.instproxy_uninstall(static_cast<instproxy_client_t>(m_instproxy), bundleId.toUtf8().constData(), nullptr, nullptr, nullptr) != INSTPROXY_E_SUCCESS) {
        m_lastError = "卸载失败";
        emit errorOccurred(m_lastError);
        return false;
    }
    
    return true;
}

bool AppManager::installApp(const QString &path)
{
    if (!m_connected || !m_instproxy) {
        m_lastError = "未连接设备";
        emit errorOccurred(m_lastError);
        return false;
    }
    
    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        m_lastError = "文件不存在: " + path;
        emit errorOccurred(m_lastError);
        return false;
    }
    
    if (fileInfo.suffix().toLower() != "ipa") {
        m_lastError = "只支持 .ipa 文件";
        emit errorOccurred(m_lastError);
        return false;
    }
    
    auto& lib = LibimobiledeviceDynamic::instance();
    
    emit progressUpdated("正在连接AFC服务...", 0);
    
    // 1. 启动AFC服务
    lockdownd_service_descriptor_t afc_service = nullptr;
    if (lib.lockdownd_start_service(static_cast<lockdownd_client_t>(m_lockdown),
                                     "com.apple.afc", &afc_service) != LOCKDOWN_E_SUCCESS || !afc_service) {
        m_lastError = "无法启动AFC服务";
        emit errorOccurred(m_lastError);
        return false;
    }
    
    // 2. 创建AFC客户端
    afc_client_t afc = nullptr;
    if (lib.afc_client_new(static_cast<idevice_t>(m_device), afc_service, &afc) != AFC_E_SUCCESS) {
        m_lastError = "无法创建AFC客户端";
        lib.lockdownd_service_descriptor_free(afc_service);
        emit errorOccurred(m_lastError);
        return false;
    }
    lib.lockdownd_service_descriptor_free(afc_service);
    
    emit progressUpdated("正在准备上传目录...", 10);
    
    // 3. 创建上传目录（PublicStaging）
    const char* staging_dir = "PublicStaging";
    lib.afc_make_directory(afc, staging_dir);
    
    // 4. 构造设备上的目标路径
    QString fileName = fileInfo.fileName();
    QString devicePath = QString("%1/%2").arg(staging_dir).arg(fileName);
    
    emit progressUpdated("正在上传IPA文件...", 20);
    
    // 5. 打开本地文件
    QFile localFile(path);
    if (!localFile.open(QIODevice::ReadOnly)) {
        m_lastError = "无法打开本地文件: " + path;
        lib.afc_client_free(afc);
        emit errorOccurred(m_lastError);
        return false;
    }
    
    // 6. 在设备上创建文件
    uint64_t afc_file = 0;
    if (lib.afc_file_open(afc, devicePath.toUtf8().constData(), AFC_FOPEN_WRONLY, &afc_file) != AFC_E_SUCCESS) {
        m_lastError = "无法在设备上创建文件";
        localFile.close();
        lib.afc_client_free(afc);
        emit errorOccurred(m_lastError);
        return false;
    }
    
    // 7. 分块上传文件
    const int chunkSize = 8192; // 8KB每块
    qint64 totalSize = localFile.size();
    qint64 uploadedSize = 0;
    
    char buffer[chunkSize];
    bool uploadSuccess = true;
    
    while (!localFile.atEnd()) {
        qint64 bytesRead = localFile.read(buffer, chunkSize);
        if (bytesRead <= 0) break;
        
        uint32_t bytesWritten = 0;
        if (lib.afc_file_write(afc, afc_file, buffer, bytesRead, &bytesWritten) != AFC_E_SUCCESS) {
            m_lastError = "文件上传失败";
            uploadSuccess = false;
            break;
        }
        
        uploadedSize += bytesWritten;
        
        // 更新进度（20%-70%）
        int progress = 20 + static_cast<int>((uploadedSize * 50.0) / totalSize);
        emit progressUpdated(QString("正在上传: %1%").arg(progress - 20), progress);
    }
    
    localFile.close();
    lib.afc_file_close(afc, afc_file);
    
    if (!uploadSuccess) {
        lib.afc_remove_path(afc, devicePath.toUtf8().constData());
        lib.afc_client_free(afc);
        emit errorOccurred(m_lastError);
        return false;
    }
    
    emit progressUpdated("正在安装应用...", 70);
    
    // 8. 调用instproxy_install安装
    QString installPath = QString("/") + devicePath;
    
    // 创建安装选项
    plist_t client_opts = lib.plist_new_dict();
    lib.plist_dict_set_item(client_opts, "PackageType", lib.plist_new_string("Developer"));
    
    // 安装应用（带进度回调）
    struct InstallContext {
        AppManager* manager;
        QString* lastError;
    };
    
    InstallContext context;
    context.manager = this;
    context.lastError = &m_lastError;
    
    auto status_callback = [](plist_t command, plist_t status, void* user_data) {
        InstallContext* ctx = static_cast<InstallContext*>(user_data);
        if (!status) return;
        
        auto& lib = LibimobiledeviceDynamic::instance();
        
        // 解析状态信息
        plist_t status_node = lib.plist_dict_get_item(status, "Status");
        if (status_node) {
            const char* status_str = nullptr;
            if (lib.plist_get_string_ptr) {
                status_str = lib.plist_get_string_ptr(status_node, nullptr);
            }
            
            if (status_str) {
                QString statusMsg = QString::fromUtf8(status_str);
                qDebug() << "安装状态:" << statusMsg;
                
                // 更新进度
                if (statusMsg == "CreatingStagingDirectory") {
                    emit ctx->manager->progressUpdated("正在创建临时目录...", 75);
                } else if (statusMsg == "ExtractingPackage") {
                    emit ctx->manager->progressUpdated("正在解压安装包...", 80);
                } else if (statusMsg == "InspectingPackage") {
                    emit ctx->manager->progressUpdated("正在检查安装包...", 85);
                } else if (statusMsg == "TakingInstallLock") {
                    emit ctx->manager->progressUpdated("正在准备安装...", 90);
                } else if (statusMsg == "PreflightingApplication") {
                    emit ctx->manager->progressUpdated("正在验证应用...", 92);
                } else if (statusMsg == "InstallingEmbeddedProfile") {
                    emit ctx->manager->progressUpdated("正在安装配置文件...", 94);
                } else if (statusMsg == "VerifyingApplication") {
                    emit ctx->manager->progressUpdated("正在验证应用签名...", 96);
                } else if (statusMsg == "CreatingContainer") {
                    emit ctx->manager->progressUpdated("正在创建应用容器...", 97);
                } else if (statusMsg == "InstallingApplication") {
                    emit ctx->manager->progressUpdated("正在安装应用...", 98);
                } else if (statusMsg == "PostflightingApplication") {
                    emit ctx->manager->progressUpdated("正在完成安装...", 99);
                } else if (statusMsg == "Complete") {
                    emit ctx->manager->progressUpdated("安装完成", 100);
                }
            }
        }
        
        // 检查错误
        plist_t error_node = lib.plist_dict_get_item(status, "Error");
        if (error_node) {
            const char* error_str = nullptr;
            if (lib.plist_get_string_ptr) {
                error_str = lib.plist_get_string_ptr(error_node, nullptr);
            }
            
            if (error_str) {
                *ctx->lastError = QString("安装失败: %1").arg(QString::fromUtf8(error_str));
                qWarning() << *ctx->lastError;
            }
        }
        
        // 检查错误描述
        plist_t error_desc = lib.plist_dict_get_item(status, "ErrorDescription");
        if (error_desc) {
            const char* desc_str = nullptr;
            if (lib.plist_get_string_ptr) {
                desc_str = lib.plist_get_string_ptr(error_desc, nullptr);
            }
            
            if (desc_str) {
                *ctx->lastError += QString(" - %1").arg(QString::fromUtf8(desc_str));
            }
        }
    };
    
    instproxy_error_t install_err = lib.instproxy_install(
        static_cast<instproxy_client_t>(m_instproxy),
        installPath.toUtf8().constData(),
        client_opts,
        status_callback,
        &context
    );
    
    lib.plist_free(client_opts);
    
    // 9. 清理：删除临时文件
    lib.afc_remove_path(afc, devicePath.toUtf8().constData());
    lib.afc_client_free(afc);
    
    if (install_err != INSTPROXY_E_SUCCESS) {
        if (m_lastError.isEmpty()) {
            m_lastError = QString("安装失败，错误代码: %1").arg(install_err);
        }
        emit errorOccurred(m_lastError);
        return false;
    }
    
    emit progressUpdated("安装成功", 100);
    return true;
}