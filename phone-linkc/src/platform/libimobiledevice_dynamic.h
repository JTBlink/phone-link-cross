/**
 * @file libimobiledevice_dynamic.h
 * @brief libimobiledevice 动态库加载器头文件
 *
 * 本文件定义了用于在运行时动态加载 libimobiledevice 库的类。
 * libimobiledevice 是一个跨平台的开源库，用于与 iOS 设备进行通信。
 *
 * 通过动态加载的方式，可以：
 * - 避免编译时对 libimobiledevice 库的硬依赖
 * - 在库不存在时优雅降级
 * - 支持不同版本的库
 *
 * @note 目前主要支持 Windows 平台的动态加载
 *
 * @see https://libimobiledevice.org/
 */

#ifndef LIBIMOBILEDEVICE_DYNAMIC_H
#define LIBIMOBILEDEVICE_DYNAMIC_H

/* ============================================================================
 * 平台相关头文件
 * ============================================================================ */

#ifdef _WIN32
#include <windows.h>  // Windows API，用于动态库加载 (LoadLibrary, GetProcAddress 等)
#endif

/* ============================================================================
 * Qt 框架头文件
 * ============================================================================ */

#include <QString>    // Qt 字符串类，用于路径处理
#include <QDebug>     // Qt 调试输出

/* ============================================================================
 * libimobiledevice 官方头文件
 *
 * 直接使用官方头文件以确保类型定义与库完全一致，避免 ABI 不兼容问题。
 * ============================================================================ */

#include <libimobiledevice/libimobiledevice.h>

/* ============================================================================
 * 相关动态库加载器头文件
 *
 * plist 和 lockdownd 相关的函数指针类型定义在独立的头文件中。
 * ============================================================================ */

#include "plist_dynamic.h"
#include "lockdown_dynamic.h"
#include "afc_dynamic.h"

#include "instproxy_dynamic.h"
#include "mobilesync_dynamic.h"
#include "mobilebackup2_dynamic.h"

/* ============================================================================
 * idevice 函数指针类型定义
 *
 * 这些类型定义用于动态加载 libimobiledevice 核心库函数。
 * ============================================================================ */

/* --------------------------------------------------------------------------
 * idevice (设备管理) 函数指针类型
 * -------------------------------------------------------------------------- */

/**
 * Get a list of UDIDs of currently available devices (USBMUX devices only).
 *
 * @param devices List of UDIDs of devices that are currently available.
 *   This list is terminated by a NULL pointer.
 * @param count Number of devices found.
 *
 * @return IDEVICE_E_SUCCESS on success or an error value when an error occurred.
 *
 * @note This function only returns the UDIDs of USBMUX devices. To also include
 *   network devices in the list, use idevice_get_device_list_extended().
 * @see idevice_get_device_list_extended
 *
 * @原型 idevice_error_t idevice_get_device_list(char ***devices, int *count);
 */
typedef idevice_error_t (*idevice_get_device_list_func)(char ***devices, int *count);

/**
 * Free a list of device UDIDs.
 *
 * @param devices List of UDIDs to free.
 *
 * @return Always returnes IDEVICE_E_SUCCESS.
 *
 * @原型 idevice_error_t idevice_device_list_free(char **devices);
 */
typedef idevice_error_t (*idevice_device_list_free_func)(char **devices);

/**
 * Creates an idevice_t structure for the device specified by UDID,
 *  if the device is available (USBMUX devices only).
 *
 * @note The resulting idevice_t structure has to be freed with
 * idevice_free() if it is no longer used.
 * If you need to connect to a device available via network, use
 * idevice_new_with_options() and include IDEVICE_LOOKUP_NETWORK in options.
 *
 * @see idevice_new_with_options
 *
 * @param device Upon calling this function, a pointer to a location of type
 *  idevice_t. On successful return, this location will be populated.
 * @param udid The UDID to match.
 *
 * @return IDEVICE_E_SUCCESS if ok, otherwise an error code.
 *
 * @原型 idevice_error_t idevice_new(idevice_t *device, const char *udid);
 */
typedef idevice_error_t (*idevice_new_func)(idevice_t *device, const char *udid);

/**
 * Cleans up an idevice structure, then frees the structure itself.
 *
 * @param device idevice_t to free.
 *
 * @原型 idevice_error_t idevice_free(idevice_t device);
 */
typedef idevice_error_t (*idevice_free_func)(idevice_t device);

/**
 * Register a callback function that will be called when device add/remove
 * events occur.
 *
 * @param callback Callback function to call.
 * @param user_data Application-specific data passed as parameter
 *   to the registered callback function.
 *
 * @return IDEVICE_E_SUCCESS on success or an error value when an error occurred.
 *
 * @原型 idevice_error_t idevice_event_subscribe(idevice_event_cb_t callback, void *user_data);
 */
typedef idevice_error_t (*idevice_event_subscribe_func)(idevice_event_cb_t callback, void *user_data);

/**
 * Release the event callback function that has been registered with
 *  idevice_event_subscribe().
 *
 * @return IDEVICE_E_SUCCESS on success or an error value when an error occurred.
 *
 * @原型 idevice_error_t idevice_event_unsubscribe(void);
 */
typedef idevice_error_t (*idevice_event_unsubscribe_func)(void);

/* ============================================================================
 * LibimobiledeviceDynamic 类
 *
 * 负责在运行时动态加载 libimobiledevice 和 plist 动态库，
 * 并提供对库函数的访问。使用单例模式确保全局只有一个实例。
 * ============================================================================ */

/**
 * @brief 动态库加载器类
 *
 * 本类负责在 Windows 平台上动态加载 libimobiledevice 和 plist DLL，
 * 并导出库函数供其他模块使用。
 *
 * 使用方法：
 * @code
 * auto& lib = LibimobiledeviceDynamic::instance();
 * if (lib.initialize()) {
 *     // 使用库函数
 *     char** devices = nullptr;
 *     int count = 0;
 *     lib.idevice_get_device_list(&devices, &count);
 *     // ...
 * }
 * @endcode
 *
 * @note
 * - 使用单例模式，通过 instance() 获取唯一实例
 * - 线程安全需要调用者自行保证
 * - 目前仅支持 Windows 平台
 */
class LibimobiledeviceDynamic {
public:
    /* ========================================================================
     * 公共静态方法
     * ======================================================================== */
    
    /**
     * @brief 获取单例实例
     *
     * @return LibimobiledeviceDynamic& 类的唯一实例引用
     *
     * @note 首次调用时会创建实例
     */
    static LibimobiledeviceDynamic& instance();
    
    /* ========================================================================
     * 公共成员方法
     * ======================================================================== */
    
    /**
     * @brief 初始化动态库
     *
     * 加载 libimobiledevice 和 plist 动态库，并解析所有需要的函数地址。
     *
     * @return true 初始化成功，所有函数已加载
     * @return false 初始化失败（库文件不存在或函数解析失败）
     *
     * @note
     * - 可以多次调用，已初始化时直接返回 true
     * - 失败时会输出调试日志
     */
    bool initialize();
    
    /**
     * @brief 清理并卸载动态库
     *
     * 释放已加载的动态库，将所有函数指针置为 nullptr。
     *
     * @note 调用后需要重新 initialize() 才能使用库函数
     */
    void cleanup();
    
    /**
     * @brief 检查是否已初始化
     *
     * @return true 已成功初始化
     * @return false 未初始化或初始化失败
     */
    bool isInitialized() const { return m_initialized; }
    
    /* ========================================================================
     * libimobiledevice 库函数指针
     *
     * 这些成员变量在 initialize() 成功后指向对应的库函数。
     * 使用前请确保 isInitialized() 返回 true。
     * ======================================================================== */
    
    idevice_get_device_list_func idevice_get_device_list;       ///< 获取设备列表
    idevice_device_list_free_func idevice_device_list_free;     ///< 释放设备列表
    idevice_new_func idevice_new;                               ///< 创建设备句柄
    idevice_free_func idevice_free;                             ///< 释放设备句柄
    idevice_event_subscribe_func idevice_event_subscribe;       ///< 订阅设备事件
    idevice_event_unsubscribe_func idevice_event_unsubscribe;   ///< 取消订阅事件
    
    /* ========================================================================
     * lockdownd 服务函数指针
     *
     * lockdownd 是 iOS 设备上的核心服务，用于设备配对和信息查询。
     * ======================================================================== */
    
    lockdownd_client_new_with_handshake_func lockdownd_client_new_with_handshake;  ///< 创建客户端
    lockdownd_client_free_func lockdownd_client_free;                               ///< 释放客户端
    lockdownd_get_value_func lockdownd_get_value;                                   ///< 获取设备属性
    
    /* ========================================================================
     * plist 库函数指针
     *
     * plist 函数用于解析从设备返回的属性列表数据。
     * ======================================================================== */
    
    plist_free_func plist_free;                       ///< 释放 plist 节点
    plist_get_node_type_func plist_get_node_type;     ///< 获取节点类型
    plist_get_string_val_func plist_get_string_val;   ///< 获取字符串值（需手动释放，慎用）
    plist_get_string_ptr_func plist_get_string_ptr;   ///< 获取字符串指针（无需释放，推荐）
    plist_get_bool_val_func plist_get_bool_val;       ///< 获取布尔值
    plist_get_uint_val_func plist_get_uint_val;       ///< 获取整数值
    plist_get_data_val_func plist_get_data_val;       ///< 获取二进制数据
    
    // 新增 plist 创建和操作函数
    plist_new_dict_func plist_new_dict;               ///< 创建字典
    plist_new_string_func plist_new_string;           ///< 创建字符串
    plist_new_bool_func plist_new_bool;               ///< 创建布尔值
    plist_dict_set_item_func plist_dict_set_item;     ///< 设置字典项
    plist_array_get_size_func plist_array_get_size;   ///< 获取数组大小
    plist_array_get_item_func plist_array_get_item;   ///< 获取数组项
    plist_dict_get_item_func plist_dict_get_item;     ///< 获取字典项
    plist_dict_new_iter_func plist_dict_new_iter;     ///< 创建字典迭代器
    plist_dict_next_item_func plist_dict_next_item;   ///< 遍历字典项
    plist_new_array_func plist_new_array;             ///< 创建数组
    plist_array_append_item_func plist_array_append_item; ///< 追加数组项
    plist_new_uint_func plist_new_uint;               ///< 创建无符号整数
    plist_new_date_func plist_new_date;               ///< 创建日期
    plist_to_xml_func plist_to_xml;                   ///< 导出为 XML
    plist_to_xml_free_func plist_to_xml_free;         ///< 释放 XML 缓冲区
    
    /* ========================================================================
     * installation_proxy 服务函数指针
     * ======================================================================== */
    
    instproxy_client_new_func instproxy_client_new;
    instproxy_client_free_func instproxy_client_free;
    instproxy_browse_func instproxy_browse;
    instproxy_install_func instproxy_install;
    instproxy_uninstall_func instproxy_uninstall;
    instproxy_lookup_func instproxy_lookup;
    
    /* ========================================================================
     * AFC (Apple File Conduit) 库函数指针
     *
     * AFC 服务用于访问 iOS 设备的文件系统。
     * ======================================================================== */
    
    afc_client_new_func afc_client_new;                   ///< 创建 AFC 客户端
    afc_client_start_service_func afc_client_start_service; ///< 启动 AFC 服务
    afc_client_free_func afc_client_free;                 ///< 释放 AFC 客户端
    afc_get_device_info_func afc_get_device_info;         ///< 获取设备信息
    afc_read_directory_func afc_read_directory;           ///< 读取目录
    afc_get_file_info_func afc_get_file_info;             ///< 获取文件信息
    afc_file_open_func afc_file_open;                     ///< 打开文件
    afc_file_close_func afc_file_close;                   ///< 关闭文件
    afc_file_read_func afc_file_read;                     ///< 读取文件
    afc_file_write_func afc_file_write;                   ///< 写入文件
    afc_make_directory_func afc_make_directory;           ///< 创建目录
    afc_remove_path_func afc_remove_path;                 ///< 删除路径
    afc_rename_path_func afc_rename_path;                 ///< 重命名路径
    afc_dictionary_free_func afc_dictionary_free;         ///< 释放字典
    
    /* ========================================================================
     * lockdownd 服务启动函数指针
     * ======================================================================== */
    
    lockdownd_start_service_func lockdownd_start_service;                     ///< 启动服务
    lockdownd_service_descriptor_free_func lockdownd_service_descriptor_free; ///< 释放服务描述符
    
    /* ========================================================================
     * mobilesync 服务函数指针
     *
     * mobilesync 服务用于同步 iOS 设备上的数据类（通讯录、日历、书签等）。
     * ======================================================================== */
    
    mobilesync_client_start_service_func mobilesync_client_start_service;         ///< 启动 mobilesync 服务
    mobilesync_client_new_func mobilesync_client_new;                             ///< 创建 mobilesync 客户端
    mobilesync_client_free_func mobilesync_client_free;                           ///< 释放 mobilesync 客户端
    mobilesync_receive_func mobilesync_receive;                                   ///< 接收数据
    mobilesync_send_func mobilesync_send;                                         ///< 发送数据
    mobilesync_start_func mobilesync_start;                                       ///< 开始同步会话
    mobilesync_finish_func mobilesync_finish;                                     ///< 完成同步会话
    mobilesync_get_all_records_from_device_func mobilesync_get_all_records_from_device; ///< 获取所有记录
    mobilesync_receive_changes_func mobilesync_receive_changes;                   ///< 接收变更
    mobilesync_acknowledge_changes_from_device_func mobilesync_acknowledge_changes_from_device; ///< 确认变更
    mobilesync_anchors_new_func mobilesync_anchors_new;                           ///< 创建锚点
    mobilesync_anchors_free_func mobilesync_anchors_free;                         ///< 释放锚点
    mobilesync_cancel_func mobilesync_cancel;                                     ///< 取消同步
    
    /* ========================================================================
     * mobilebackup2 服务函数指针
     * ======================================================================== */
    
    mobilebackup2_client_new_func mobilebackup2_client_new;
    mobilebackup2_client_start_service_func mobilebackup2_client_start_service;
    mobilebackup2_client_free_func mobilebackup2_client_free;
    mobilebackup2_send_message_func mobilebackup2_send_message;
    mobilebackup2_receive_message_func mobilebackup2_receive_message;
    mobilebackup2_send_raw_func mobilebackup2_send_raw;
    mobilebackup2_receive_raw_func mobilebackup2_receive_raw;
    mobilebackup2_version_exchange_func mobilebackup2_version_exchange;
    mobilebackup2_send_request_func mobilebackup2_send_request;
    mobilebackup2_send_status_response_func mobilebackup2_send_status_response;

private:
    /* ========================================================================
     * 私有构造函数和析构函数（单例模式）
     * ======================================================================== */
    
    /**
     * @brief 私有构造函数
     *
     * 初始化所有成员变量为默认值（函数指针为 nullptr）。
     */
    LibimobiledeviceDynamic();
    
    /**
     * @brief 私有析构函数
     *
     * 自动调用 cleanup() 释放资源。
     */
    ~LibimobiledeviceDynamic();
    
    // 禁止拷贝和赋值（单例模式）
    LibimobiledeviceDynamic(const LibimobiledeviceDynamic&) = delete;
    LibimobiledeviceDynamic& operator=(const LibimobiledeviceDynamic&) = delete;
    
    /* ========================================================================
     * 私有辅助方法
     * ======================================================================== */
    
    /**
     * @brief 加载动态库
     *
     * @param path 动态库文件路径
     * @return true 加载成功
     * @return false 加载失败
     */
    bool loadLibrary(const QString& path);
    
    /**
     * @brief 从动态库加载函数
     *
     * @tparam T 函数指针类型
     * @param functionName 函数名称
     * @param functionPtr [out] 函数指针引用
     * @param library 动态库句柄
     * @return true 加载成功
     * @return false 加载失败（函数不存在）
     */
    template<typename T>
    bool loadFunction(const QString& functionName, T& functionPtr, HMODULE library);
    
    /* ========================================================================
     * 私有成员变量
     * ======================================================================== */
    
    bool m_initialized;  ///< 初始化状态标志
    
#ifdef _WIN32
    HMODULE m_imobiledeviceLib;  ///< libimobiledevice 动态库句柄
    HMODULE m_plistLib;          ///< plist 动态库句柄
#endif
};

#endif // LIBIMOBILEDEVICE_DYNAMIC_H