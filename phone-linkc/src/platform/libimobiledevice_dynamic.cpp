#include "libimobiledevice_dynamic.h"
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>

LibimobiledeviceDynamic::LibimobiledeviceDynamic()
    : m_initialized(false)
#ifdef _WIN32
    , m_imobiledeviceLib(nullptr)
    , m_plistLib(nullptr)
#endif
{
    // 初始化所有函数指针为nullptr
    idevice_get_device_list = nullptr;
    idevice_device_list_free = nullptr;
    idevice_new = nullptr;
    idevice_free = nullptr;
    
    // v1.4.0+ API
    idevice_events_subscribe = nullptr;
    idevice_events_unsubscribe = nullptr;
    idevice_get_device_list_extended = nullptr;
    idevice_device_list_extended_free = nullptr;
    idevice_new_with_options = nullptr;
    
    lockdownd_client_new_with_handshake = nullptr;
    lockdownd_client_free = nullptr;
    lockdownd_get_value = nullptr;
    lockdownd_start_service = nullptr;
    lockdownd_service_descriptor_free = nullptr;
    
    plist_free = nullptr;
    plist_get_node_type = nullptr;
    plist_get_string_val = nullptr;
    plist_get_string_ptr = nullptr;
    plist_get_bool_val = nullptr;
    plist_get_uint_val = nullptr;
    
    plist_get_data_val = nullptr;
    
    // 新增 plist 函数指针
    plist_new_dict = nullptr;
    plist_new_string = nullptr;
    plist_new_bool = nullptr;
    plist_dict_set_item = nullptr;
    plist_array_get_size = nullptr;
    plist_array_get_item = nullptr;
    plist_dict_get_item = nullptr;
    plist_dict_new_iter = nullptr;
    plist_dict_next_item = nullptr;
    plist_new_array = nullptr;
    plist_array_append_item = nullptr;
    plist_new_uint = nullptr;
    plist_new_int = nullptr;
    plist_new_date = nullptr;
    plist_to_xml = nullptr;
    plist_to_bin = nullptr;
    plist_mem_free = nullptr;
    
    // installation_proxy 函数指针
    instproxy_client_new = nullptr;
    instproxy_client_free = nullptr;
    instproxy_browse = nullptr;
    instproxy_install = nullptr;
    instproxy_uninstall = nullptr;
    instproxy_lookup = nullptr;
    
    // AFC 函数指针
    afc_client_new = nullptr;
    afc_client_start_service = nullptr;
    afc_client_free = nullptr;
    afc_get_device_info = nullptr;
    afc_read_directory = nullptr;
    afc_get_file_info = nullptr;
    afc_file_open = nullptr;
    afc_file_close = nullptr;
    afc_file_read = nullptr;
    afc_file_write = nullptr;
    afc_make_directory = nullptr;
    afc_remove_path = nullptr;
    afc_rename_path = nullptr;
    afc_dictionary_free = nullptr;
    
    // mobilesync 函数指针
    mobilesync_client_start_service = nullptr;
    mobilesync_client_new = nullptr;
    mobilesync_client_free = nullptr;
    mobilesync_receive = nullptr;
    mobilesync_send = nullptr;
    mobilesync_start = nullptr;
    mobilesync_finish = nullptr;
    mobilesync_get_all_records_from_device = nullptr;
    mobilesync_receive_changes = nullptr;
    mobilesync_acknowledge_changes_from_device = nullptr;
    mobilesync_anchors_new = nullptr;
    mobilesync_anchors_free = nullptr;
    mobilesync_cancel = nullptr;
    
    // mobilebackup2 函数指针
    mobilebackup2_client_new = nullptr;
    mobilebackup2_client_start_service = nullptr;
    mobilebackup2_client_free = nullptr;
    mobilebackup2_send_message = nullptr;
    mobilebackup2_receive_message = nullptr;
    mobilebackup2_send_raw = nullptr;
    mobilebackup2_receive_raw = nullptr;
    mobilebackup2_version_exchange = nullptr;
    mobilebackup2_send_request = nullptr;
    mobilebackup2_send_status_response = nullptr;
}

LibimobiledeviceDynamic::~LibimobiledeviceDynamic()
{
    cleanup();
}

LibimobiledeviceDynamic& LibimobiledeviceDynamic::instance()
{
    static LibimobiledeviceDynamic instance;
    return instance;
}

bool LibimobiledeviceDynamic::initialize()
{
    if (m_initialized) {
        return true;
    }

#ifdef _WIN32
    qDebug() << "开始初始化动态库加载器...";
    
    // 获取应用程序目录
    QString appDir = QCoreApplication::applicationDirPath();
    QString thirdpartyDir;
    
    // 搜索路径优先级：
    // 1. 应用程序目录本身（构建目录中的库文件）
    // 2. 部署目录中的libimobiledevice
    // 3. 应用程序目录下的thirdparty/libimobiledevice
    // 4. 相对路径的thirdparty/libimobiledevice
    // 5. 项目根目录的thirdparty/libimobiledevice
    
    QStringList searchPaths = {
        appDir,                                          // 应用程序目录本身（最高优先级）
        appDir + "/libimobiledevice",                    // 部署目录
        appDir + "/../thirdparty/libimobiledevice",      // 相对路径
        QDir::current().absoluteFilePath("thirdparty/libimobiledevice")  // 项目根目录
    };
    
    bool found = false;
    for (const QString& path : searchPaths) {
        if (QDir(path).exists()) {
            // 检查关键文件是否存在
            if (QFile::exists(path + "/libimobiledevice-1.0.dll") && QFile::exists(path + "/libplist-2.0.dll")) {
                thirdpartyDir = path;
                found = true;
                qDebug() << "找到libimobiledevice库:" << thirdpartyDir;
                break;
            }
        }
    }
    
    if (!found) {
        qWarning() << "未找到libimobiledevice库文件";
        qWarning() << "搜索路径:";
        for (const QString& path : searchPaths) {
            qWarning() << "  -" << path;
        }
        return false;
    }
    
    qDebug() << "使用libimobiledevice目录:" << thirdpartyDir;
    
    // 临时添加DLL搜索路径
    QString nativePath = QDir::toNativeSeparators(thirdpartyDir);
    SetDllDirectoryA(nativePath.toLocal8Bit().constData());
    
    // 加载libimobiledevice-1.0.dll (v1.4.0)
    QString imobiledevicePath = thirdpartyDir + "/libimobiledevice-1.0.dll";
    m_imobiledeviceLib = LoadLibraryA(imobiledevicePath.toLocal8Bit().constData());
    if (!m_imobiledeviceLib) {
        DWORD error = GetLastError();
        qWarning() << "无法加载libimobiledevice-1.0.dll，错误代码:" << error;
        qWarning() << "尝试路径:" << imobiledevicePath;
        return false;
    }
    qDebug() << "成功加载libimobiledevice-1.0.dll";
    
    // 加载libplist-2.0.dll (v2.7.0)
    QString plistPath = thirdpartyDir + "/libplist-2.0.dll";
    m_plistLib = LoadLibraryA(plistPath.toLocal8Bit().constData());
    if (!m_plistLib) {
        DWORD error = GetLastError();
        qWarning() << "无法加载libplist-2.0.dll，错误代码:" << error;
        qWarning() << "尝试路径:" << plistPath;
        FreeLibrary(m_imobiledeviceLib);
        m_imobiledeviceLib = nullptr;
        return false;
    }
    qDebug() << "成功加载libplist-2.0.dll";
    
    // 加载libimobiledevice函数
    QStringList failedFunctions;
    int totalFunctions = 0;
    int successCount = 0;
    
    auto loadAndTrack = [&](const QString& name, auto& funcPtr, HMODULE lib) -> bool {
        totalFunctions++;
        if (loadFunction(name, funcPtr, lib)) {
            successCount++;
            return true;
        } else {
            failedFunctions << name;
            return false;
        }
    };
    
    bool success = true;
    
    // 基础设备 API（必需）
    success &= loadAndTrack("idevice_get_device_list", idevice_get_device_list, m_imobiledeviceLib);
    success &= loadAndTrack("idevice_device_list_free", idevice_device_list_free, m_imobiledeviceLib);
    success &= loadAndTrack("idevice_new", idevice_new, m_imobiledeviceLib);
    success &= loadAndTrack("idevice_free", idevice_free, m_imobiledeviceLib);
    
    // v1.4.0+ 新版事件 API（必需）
    success &= loadAndTrack("idevice_events_subscribe", idevice_events_subscribe, m_imobiledeviceLib);
    success &= loadAndTrack("idevice_events_unsubscribe", idevice_events_unsubscribe, m_imobiledeviceLib);
    
    // v1.4.0+ 网络设备支持 API（必需）
    success &= loadAndTrack("idevice_get_device_list_extended", idevice_get_device_list_extended, m_imobiledeviceLib);
    success &= loadAndTrack("idevice_device_list_extended_free", idevice_device_list_extended_free, m_imobiledeviceLib);
    success &= loadAndTrack("idevice_new_with_options", idevice_new_with_options, m_imobiledeviceLib);
    
    qDebug() << "使用 libimobiledevice v1.4.0+ API，支持网络设备和新事件系统";
    
    success &= loadAndTrack("lockdownd_client_new_with_handshake", lockdownd_client_new_with_handshake, m_imobiledeviceLib);
    success &= loadAndTrack("lockdownd_client_free", lockdownd_client_free, m_imobiledeviceLib);
    success &= loadAndTrack("lockdownd_get_value", lockdownd_get_value, m_imobiledeviceLib);
    success &= loadAndTrack("lockdownd_start_service", lockdownd_start_service, m_imobiledeviceLib);
    success &= loadAndTrack("lockdownd_service_descriptor_free", lockdownd_service_descriptor_free, m_imobiledeviceLib);
    
    // 加载 AFC 函数
    success &= loadAndTrack("afc_client_new", afc_client_new, m_imobiledeviceLib);
    success &= loadAndTrack("afc_client_start_service", afc_client_start_service, m_imobiledeviceLib);
    success &= loadAndTrack("afc_client_free", afc_client_free, m_imobiledeviceLib);
    success &= loadAndTrack("afc_get_device_info", afc_get_device_info, m_imobiledeviceLib);
    success &= loadAndTrack("afc_read_directory", afc_read_directory, m_imobiledeviceLib);
    success &= loadAndTrack("afc_get_file_info", afc_get_file_info, m_imobiledeviceLib);
    success &= loadAndTrack("afc_file_open", afc_file_open, m_imobiledeviceLib);
    success &= loadAndTrack("afc_file_close", afc_file_close, m_imobiledeviceLib);
    success &= loadAndTrack("afc_file_read", afc_file_read, m_imobiledeviceLib);
    success &= loadAndTrack("afc_file_write", afc_file_write, m_imobiledeviceLib);
    success &= loadAndTrack("afc_make_directory", afc_make_directory, m_imobiledeviceLib);
    success &= loadAndTrack("afc_remove_path", afc_remove_path, m_imobiledeviceLib);
    success &= loadAndTrack("afc_rename_path", afc_rename_path, m_imobiledeviceLib);
    success &= loadAndTrack("afc_dictionary_free", afc_dictionary_free, m_imobiledeviceLib);
    
    // 加载plist函数
    success &= loadAndTrack("plist_free", plist_free, m_plistLib);
    success &= loadAndTrack("plist_get_node_type", plist_get_node_type, m_plistLib);
    success &= loadAndTrack("plist_get_string_val", plist_get_string_val, m_plistLib);
    success &= loadAndTrack("plist_get_string_ptr", plist_get_string_ptr, m_plistLib);
    success &= loadAndTrack("plist_get_bool_val", plist_get_bool_val, m_plistLib);
    success &= loadAndTrack("plist_get_uint_val", plist_get_uint_val, m_plistLib);
    success &= loadAndTrack("plist_get_data_val", plist_get_data_val, m_plistLib);
    
    // 加载新增 plist 函数
    success &= loadAndTrack("plist_new_dict", plist_new_dict, m_plistLib);
    success &= loadAndTrack("plist_new_string", plist_new_string, m_plistLib);
    success &= loadAndTrack("plist_new_bool", plist_new_bool, m_plistLib);
    success &= loadAndTrack("plist_dict_set_item", plist_dict_set_item, m_plistLib);
    success &= loadAndTrack("plist_array_get_size", plist_array_get_size, m_plistLib);
    success &= loadAndTrack("plist_array_get_item", plist_array_get_item, m_plistLib);
    success &= loadAndTrack("plist_dict_get_item", plist_dict_get_item, m_plistLib);
    success &= loadAndTrack("plist_dict_new_iter", plist_dict_new_iter, m_plistLib);
    success &= loadAndTrack("plist_dict_next_item", plist_dict_next_item, m_plistLib);
    success &= loadAndTrack("plist_new_array", plist_new_array, m_plistLib);
    success &= loadAndTrack("plist_array_append_item", plist_array_append_item, m_plistLib);
    success &= loadAndTrack("plist_new_uint", plist_new_uint, m_plistLib);
    success &= loadAndTrack("plist_new_int", plist_new_int, m_plistLib);
    success &= loadAndTrack("plist_new_date", plist_new_date, m_plistLib);
    success &= loadAndTrack("plist_to_xml", plist_to_xml, m_plistLib);
    success &= loadAndTrack("plist_to_bin", plist_to_bin, m_plistLib);
    success &= loadAndTrack("plist_mem_free", plist_mem_free, m_plistLib);
    
    // 加载 instproxy 函数
    success &= loadAndTrack("instproxy_client_new", instproxy_client_new, m_imobiledeviceLib);
    success &= loadAndTrack("instproxy_client_free", instproxy_client_free, m_imobiledeviceLib);
    success &= loadAndTrack("instproxy_browse", instproxy_browse, m_imobiledeviceLib);
    success &= loadAndTrack("instproxy_install", instproxy_install, m_imobiledeviceLib);
    success &= loadAndTrack("instproxy_uninstall", instproxy_uninstall, m_imobiledeviceLib);
    success &= loadAndTrack("instproxy_lookup", instproxy_lookup, m_imobiledeviceLib);
    
    // 加载 mobilesync 函数
    success &= loadAndTrack("mobilesync_client_start_service", mobilesync_client_start_service, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_client_new", mobilesync_client_new, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_client_free", mobilesync_client_free, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_receive", mobilesync_receive, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_send", mobilesync_send, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_start", mobilesync_start, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_finish", mobilesync_finish, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_get_all_records_from_device", mobilesync_get_all_records_from_device, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_receive_changes", mobilesync_receive_changes, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_acknowledge_changes_from_device", mobilesync_acknowledge_changes_from_device, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_anchors_new", mobilesync_anchors_new, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_anchors_free", mobilesync_anchors_free, m_imobiledeviceLib);
    success &= loadAndTrack("mobilesync_cancel", mobilesync_cancel, m_imobiledeviceLib);
    
    // 加载 mobilebackup2 函数
    success &= loadAndTrack("mobilebackup2_client_new", mobilebackup2_client_new, m_imobiledeviceLib);
    success &= loadAndTrack("mobilebackup2_client_start_service", mobilebackup2_client_start_service, m_imobiledeviceLib);
    success &= loadAndTrack("mobilebackup2_client_free", mobilebackup2_client_free, m_imobiledeviceLib);
    success &= loadAndTrack("mobilebackup2_send_message", mobilebackup2_send_message, m_imobiledeviceLib);
    success &= loadAndTrack("mobilebackup2_receive_message", mobilebackup2_receive_message, m_imobiledeviceLib);
    success &= loadAndTrack("mobilebackup2_send_raw", mobilebackup2_send_raw, m_imobiledeviceLib);
    success &= loadAndTrack("mobilebackup2_receive_raw", mobilebackup2_receive_raw, m_imobiledeviceLib);
    success &= loadAndTrack("mobilebackup2_version_exchange", mobilebackup2_version_exchange, m_imobiledeviceLib);
    success &= loadAndTrack("mobilebackup2_send_request", mobilebackup2_send_request, m_imobiledeviceLib);
    success &= loadAndTrack("mobilebackup2_send_status_response", mobilebackup2_send_status_response, m_imobiledeviceLib);
    
    // 打印函数加载统计信息
    qDebug() << "========== 函数加载统计 ==========";
    qDebug() << "总函数数:" << totalFunctions;
    qDebug() << "成功加载:" << successCount;
    qDebug() << "加载失败:" << failedFunctions.size();
    
    if (!success) {
        qWarning() << "========== 加载失败的函数列表 ==========";
        for (const QString& funcName : failedFunctions) {
            qWarning() << "  -" << funcName;
        }
        qWarning() << "=====================================";
        cleanup();
        return false;
    }
    
    m_initialized = true;
    qDebug() << "动态库加载器初始化成功";
    return true;
    
#else
    qWarning() << "动态加载仅在Windows上支持";
    return false;
#endif
}

void LibimobiledeviceDynamic::cleanup()
{
    if (!m_initialized) {
        return;
    }
    
#ifdef _WIN32
    if (m_imobiledeviceLib) {
        FreeLibrary(m_imobiledeviceLib);
        m_imobiledeviceLib = nullptr;
    }
    
    if (m_plistLib) {
        FreeLibrary(m_plistLib);
        m_plistLib = nullptr;
    }
    
    // 重置DLL搜索路径
    SetDllDirectoryA(nullptr);
#endif
    
    // 重置所有函数指针
    idevice_get_device_list = nullptr;
    idevice_device_list_free = nullptr;
    idevice_new = nullptr;
    idevice_free = nullptr;
    
    // v1.4.0+ API
    idevice_events_subscribe = nullptr;
    idevice_events_unsubscribe = nullptr;
    idevice_get_device_list_extended = nullptr;
    idevice_device_list_extended_free = nullptr;
    idevice_new_with_options = nullptr;
    
    lockdownd_client_new_with_handshake = nullptr;
    lockdownd_client_free = nullptr;
    lockdownd_get_value = nullptr;
    lockdownd_start_service = nullptr;
    lockdownd_service_descriptor_free = nullptr;
    
    plist_free = nullptr;
    plist_get_node_type = nullptr;
    plist_get_string_val = nullptr;
    plist_get_string_ptr = nullptr;
    plist_get_bool_val = nullptr;
    plist_get_uint_val = nullptr;
    plist_get_data_val = nullptr;
    
    plist_new_dict = nullptr;
    plist_new_string = nullptr;
    plist_new_bool = nullptr;
    plist_dict_set_item = nullptr;
    plist_array_get_size = nullptr;
    plist_array_get_item = nullptr;
    plist_dict_get_item = nullptr;
    plist_dict_new_iter = nullptr;
    plist_dict_next_item = nullptr;
    plist_new_array = nullptr;
    plist_array_append_item = nullptr;
    plist_new_uint = nullptr;
    plist_new_date = nullptr;
    plist_to_xml = nullptr;
    plist_to_bin = nullptr;
    plist_mem_free = nullptr;
    
    instproxy_client_new = nullptr;
    instproxy_client_free = nullptr;
    instproxy_browse = nullptr;
    instproxy_install = nullptr;
    instproxy_uninstall = nullptr;
    instproxy_lookup = nullptr;
    
    mobilesync_client_start_service = nullptr;
    mobilesync_client_new = nullptr;
    mobilesync_client_free = nullptr;
    mobilesync_receive = nullptr;
    mobilesync_send = nullptr;
    mobilesync_start = nullptr;
    mobilesync_finish = nullptr;
    mobilesync_get_all_records_from_device = nullptr;
    mobilesync_receive_changes = nullptr;
    mobilesync_acknowledge_changes_from_device = nullptr;
    mobilesync_anchors_new = nullptr;
    mobilesync_anchors_free = nullptr;
    mobilesync_cancel = nullptr;
    
    mobilebackup2_client_new = nullptr;
    mobilebackup2_client_start_service = nullptr;
    mobilebackup2_client_free = nullptr;
    mobilebackup2_send_message = nullptr;
    mobilebackup2_receive_message = nullptr;
    mobilebackup2_send_raw = nullptr;
    mobilebackup2_receive_raw = nullptr;
    mobilebackup2_version_exchange = nullptr;
    mobilebackup2_send_request = nullptr;
    mobilebackup2_send_status_response = nullptr;
    
    // AFC 函数指针
    afc_client_new = nullptr;
    afc_client_start_service = nullptr;
    afc_client_free = nullptr;
    afc_get_device_info = nullptr;
    afc_read_directory = nullptr;
    afc_get_file_info = nullptr;
    afc_file_open = nullptr;
    afc_file_close = nullptr;
    afc_file_read = nullptr;
    afc_file_write = nullptr;
    afc_make_directory = nullptr;
    afc_remove_path = nullptr;
    afc_rename_path = nullptr;
    afc_dictionary_free = nullptr;
    
    m_initialized = false;
    qDebug() << "动态库加载器已清理";
}

template<typename T>
bool LibimobiledeviceDynamic::loadFunction(const QString& functionName, T& functionPtr, HMODULE library)
{
#ifdef _WIN32
    FARPROC proc = GetProcAddress(library, functionName.toLocal8Bit().constData());
    if (!proc) {
        qWarning() << "无法加载函数:" << functionName;
        return false;
    }
    
    functionPtr = reinterpret_cast<T>(proc);
    qDebug() << "成功加载函数:" << functionName;
    return true;
#else
    Q_UNUSED(functionName)
    Q_UNUSED(functionPtr)
    Q_UNUSED(library)
    return false;
#endif
}