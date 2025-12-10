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
    idevice_event_subscribe = nullptr;
    idevice_event_unsubscribe = nullptr;
    
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
            if (QFile::exists(path + "/imobiledevice.dll") && QFile::exists(path + "/plist.dll")) {
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
    
    // 加载imobiledevice.dll
    QString imobiledevicePath = thirdpartyDir + "/imobiledevice.dll";
    m_imobiledeviceLib = LoadLibraryA(imobiledevicePath.toLocal8Bit().constData());
    if (!m_imobiledeviceLib) {
        DWORD error = GetLastError();
        qWarning() << "无法加载imobiledevice.dll，错误代码:" << error;
        qWarning() << "尝试路径:" << imobiledevicePath;
        return false;
    }
    qDebug() << "成功加载imobiledevice.dll";
    
    // 加载plist.dll  
    QString plistPath = thirdpartyDir + "/plist.dll";
    m_plistLib = LoadLibraryA(plistPath.toLocal8Bit().constData());
    if (!m_plistLib) {
        DWORD error = GetLastError();
        qWarning() << "无法加载plist.dll，错误代码:" << error;
        qWarning() << "尝试路径:" << plistPath;
        FreeLibrary(m_imobiledeviceLib);
        m_imobiledeviceLib = nullptr;
        return false;
    }
    qDebug() << "成功加载plist.dll";
    
    // 加载libimobiledevice函数
    bool success = true;
    success &= loadFunction("idevice_get_device_list", idevice_get_device_list, m_imobiledeviceLib);
    success &= loadFunction("idevice_device_list_free", idevice_device_list_free, m_imobiledeviceLib);
    success &= loadFunction("idevice_new", idevice_new, m_imobiledeviceLib);
    success &= loadFunction("idevice_free", idevice_free, m_imobiledeviceLib);
    success &= loadFunction("idevice_event_subscribe", idevice_event_subscribe, m_imobiledeviceLib);
    success &= loadFunction("idevice_event_unsubscribe", idevice_event_unsubscribe, m_imobiledeviceLib);
    
    success &= loadFunction("lockdownd_client_new_with_handshake", lockdownd_client_new_with_handshake, m_imobiledeviceLib);
    success &= loadFunction("lockdownd_client_free", lockdownd_client_free, m_imobiledeviceLib);
    success &= loadFunction("lockdownd_get_value", lockdownd_get_value, m_imobiledeviceLib);
    success &= loadFunction("lockdownd_start_service", lockdownd_start_service, m_imobiledeviceLib);
    success &= loadFunction("lockdownd_service_descriptor_free", lockdownd_service_descriptor_free, m_imobiledeviceLib);
    
    // 加载 AFC 函数
    success &= loadFunction("afc_client_new", afc_client_new, m_imobiledeviceLib);
    success &= loadFunction("afc_client_start_service", afc_client_start_service, m_imobiledeviceLib);
    success &= loadFunction("afc_client_free", afc_client_free, m_imobiledeviceLib);
    success &= loadFunction("afc_get_device_info", afc_get_device_info, m_imobiledeviceLib);
    success &= loadFunction("afc_read_directory", afc_read_directory, m_imobiledeviceLib);
    success &= loadFunction("afc_get_file_info", afc_get_file_info, m_imobiledeviceLib);
    success &= loadFunction("afc_file_open", afc_file_open, m_imobiledeviceLib);
    success &= loadFunction("afc_file_close", afc_file_close, m_imobiledeviceLib);
    success &= loadFunction("afc_file_read", afc_file_read, m_imobiledeviceLib);
    success &= loadFunction("afc_file_write", afc_file_write, m_imobiledeviceLib);
    success &= loadFunction("afc_make_directory", afc_make_directory, m_imobiledeviceLib);
    success &= loadFunction("afc_remove_path", afc_remove_path, m_imobiledeviceLib);
    success &= loadFunction("afc_rename_path", afc_rename_path, m_imobiledeviceLib);
    success &= loadFunction("afc_dictionary_free", afc_dictionary_free, m_imobiledeviceLib);
    
    // 加载plist函数
    success &= loadFunction("plist_free", plist_free, m_plistLib);
    success &= loadFunction("plist_get_node_type", plist_get_node_type, m_plistLib);
    success &= loadFunction("plist_get_string_val", plist_get_string_val, m_plistLib);
    success &= loadFunction("plist_get_string_ptr", plist_get_string_ptr, m_plistLib);
    success &= loadFunction("plist_get_bool_val", plist_get_bool_val, m_plistLib);
    success &= loadFunction("plist_get_uint_val", plist_get_uint_val, m_plistLib);
    success &= loadFunction("plist_get_data_val", plist_get_data_val, m_plistLib);
    
    // 加载新增 plist 函数
    success &= loadFunction("plist_new_dict", plist_new_dict, m_plistLib);
    success &= loadFunction("plist_new_string", plist_new_string, m_plistLib);
    success &= loadFunction("plist_new_bool", plist_new_bool, m_plistLib);
    success &= loadFunction("plist_dict_set_item", plist_dict_set_item, m_plistLib);
    success &= loadFunction("plist_array_get_size", plist_array_get_size, m_plistLib);
    success &= loadFunction("plist_array_get_item", plist_array_get_item, m_plistLib);
    success &= loadFunction("plist_dict_get_item", plist_dict_get_item, m_plistLib);
    success &= loadFunction("plist_dict_new_iter", plist_dict_new_iter, m_plistLib);
    success &= loadFunction("plist_dict_next_item", plist_dict_next_item, m_plistLib);
    success &= loadFunction("plist_new_array", plist_new_array, m_plistLib);
    success &= loadFunction("plist_array_append_item", plist_array_append_item, m_plistLib);
    
    // 加载 instproxy 函数
    success &= loadFunction("instproxy_client_new", instproxy_client_new, m_imobiledeviceLib);
    success &= loadFunction("instproxy_client_free", instproxy_client_free, m_imobiledeviceLib);
    success &= loadFunction("instproxy_browse", instproxy_browse, m_imobiledeviceLib);
    success &= loadFunction("instproxy_install", instproxy_install, m_imobiledeviceLib);
    success &= loadFunction("instproxy_uninstall", instproxy_uninstall, m_imobiledeviceLib);
    success &= loadFunction("instproxy_lookup", instproxy_lookup, m_imobiledeviceLib);
    
    if (!success) {
        qWarning() << "部分函数加载失败";
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
    idevice_event_subscribe = nullptr;
    idevice_event_unsubscribe = nullptr;
    
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