#ifndef LIBIMOBILEDEVICE_DYNAMIC_H
#define LIBIMOBILEDEVICE_DYNAMIC_H

#ifdef _WIN32
#include <windows.h>
#endif

#include <QString>
#include <QDebug>

// 前向声明libimobiledevice结构体
typedef struct idevice_private idevice_private;
typedef idevice_private* idevice_t;
typedef struct lockdownd_client_private lockdownd_client_private;
typedef lockdownd_client_private* lockdownd_client_t;
typedef struct plist_node* plist_t;

// 错误码枚举
typedef enum {
    IDEVICE_E_SUCCESS = 0,
    IDEVICE_E_INVALID_ARG = -1,
    IDEVICE_E_UNKNOWN_ERROR = -2,
    IDEVICE_E_NO_DEVICE = -3,
    IDEVICE_E_NOT_ENOUGH_DATA = -4,
    IDEVICE_E_BAD_HEADER = -5,
    IDEVICE_E_SSL_ERROR = -6,
    IDEVICE_E_TIMEOUT = -7
} idevice_error_t;

typedef enum {
    LOCKDOWN_E_SUCCESS = 0,
    LOCKDOWN_E_INVALID_ARG = -1,
    LOCKDOWN_E_INVALID_CONF = -2,
    LOCKDOWN_E_PLIST_ERROR = -3,
    LOCKDOWN_E_PAIRING_FAILED = -4,
    LOCKDOWN_E_SSL_ERROR = -5,
    LOCKDOWN_E_DICT_ERROR = -6,
    LOCKDOWN_E_NOT_ENOUGH_DATA = -7,
    LOCKDOWN_E_MUX_ERROR = -8
} lockdownd_error_t;

// 事件类型枚举
typedef enum {
    IDEVICE_DEVICE_ADD = 1,
    IDEVICE_DEVICE_REMOVE = 2,
    IDEVICE_DEVICE_PAIRED = 3
} idevice_event_type;

typedef struct {
    idevice_event_type event;
    char* udid;
    int conn_type;
} idevice_event_t;

// plist节点类型
typedef enum {
    PLIST_BOOLEAN,
    PLIST_UINT,
    PLIST_REAL,
    PLIST_STRING,
    PLIST_ARRAY,
    PLIST_DICT,
    PLIST_DATE,
    PLIST_DATA,
    PLIST_KEY,
    PLIST_UID,
    PLIST_NONE
} plist_type;

// 函数指针类型定义
typedef idevice_error_t (*idevice_get_device_list_func)(char ***devices, int *count);
typedef idevice_error_t (*idevice_device_list_free_func)(char **devices);
typedef idevice_error_t (*idevice_new_func)(idevice_t *device, const char *udid);
typedef idevice_error_t (*idevice_free_func)(idevice_t device);
typedef idevice_error_t (*idevice_event_subscribe_func)(void (*callback)(const idevice_event_t *event, void *user_data), void *user_data);
typedef idevice_error_t (*idevice_event_unsubscribe_func)(void);

typedef lockdownd_error_t (*lockdownd_client_new_with_handshake_func)(idevice_t device, lockdownd_client_t *client, const char *label);
typedef lockdownd_error_t (*lockdownd_client_free_func)(lockdownd_client_t client);
typedef lockdownd_error_t (*lockdownd_get_value_func)(lockdownd_client_t client, const char *domain, const char *key, plist_t *value);

typedef void (*plist_free_func)(plist_t plist);
typedef plist_type (*plist_get_node_type_func)(plist_t node);
typedef void (*plist_get_string_val_func)(plist_t node, char **val);
typedef void (*plist_get_bool_val_func)(plist_t node, uint8_t *val);
typedef void (*plist_get_uint_val_func)(plist_t node, uint64_t *val);

/**
 * 动态库加载器类
 * 负责在Windows上动态加载libimobiledevice和plist DLL
 */
class LibimobiledeviceDynamic {
public:
    static LibimobiledeviceDynamic& instance();
    
    bool initialize();
    void cleanup();
    bool isInitialized() const { return m_initialized; }
    
    // libimobiledevice 函数
    idevice_get_device_list_func idevice_get_device_list;
    idevice_device_list_free_func idevice_device_list_free;
    idevice_new_func idevice_new;
    idevice_free_func idevice_free;
    idevice_event_subscribe_func idevice_event_subscribe;
    idevice_event_unsubscribe_func idevice_event_unsubscribe;
    
    // lockdownd 函数
    lockdownd_client_new_with_handshake_func lockdownd_client_new_with_handshake;
    lockdownd_client_free_func lockdownd_client_free;
    lockdownd_get_value_func lockdownd_get_value;
    
    // plist 函数
    plist_free_func plist_free;
    plist_get_node_type_func plist_get_node_type;
    plist_get_string_val_func plist_get_string_val;
    plist_get_bool_val_func plist_get_bool_val;
    plist_get_uint_val_func plist_get_uint_val;
    
private:
    LibimobiledeviceDynamic();
    ~LibimobiledeviceDynamic();
    LibimobiledeviceDynamic(const LibimobiledeviceDynamic&) = delete;
    LibimobiledeviceDynamic& operator=(const LibimobiledeviceDynamic&) = delete;
    
    bool loadLibrary(const QString& path);
    template<typename T>
    bool loadFunction(const QString& functionName, T& functionPtr, HMODULE library);
    
    bool m_initialized;
    
#ifdef _WIN32
    HMODULE m_imobiledeviceLib;
    HMODULE m_plistLib;
#endif
};

#endif // LIBIMOBILEDEVICE_DYNAMIC_H