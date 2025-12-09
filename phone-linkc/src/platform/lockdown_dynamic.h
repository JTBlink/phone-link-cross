/**
 * @file lockdown_dynamic.h
 * @brief lockdownd 动态库加载器头文件
 *
 * 本文件定义了用于在运行时动态加载 lockdownd 相关函数的函数指针类型。
 * lockdownd 是 iOS 设备上的一个守护进程，负责设备配对、
 * 获取设备信息、启动其他服务等功能。
 *
 * @note 本文件被 libimobiledevice_dynamic.h 包含使用
 *
 * @see https://libimobiledevice.org/
 */

#ifndef LOCKDOWN_DYNAMIC_H
#define LOCKDOWN_DYNAMIC_H

/* ============================================================================
 * lockdownd 官方头文件
 *
 * 直接使用官方头文件以确保类型定义与库完全一致，避免 ABI 不兼容问题。
 * ============================================================================ */

#include <libimobiledevice/lockdown.h>

/* ============================================================================
 * lockdownd 函数指针类型定义
 *
 * 这些类型定义用于动态加载 lockdownd 库函数。
 * ============================================================================ */

/**
 * Creates a new lockdownd client for the device and starts initial handshake.
 * The handshake consists out of query_type, validate_pair, pair and
 * start_session calls. It uses the internal pairing record management.
 *
 * @note The device disconnects automatically if the lockdown connection idles
 *  for more than 10 seconds. Make sure to call lockdownd_client_free() as soon
 *  as the connection is no longer needed.
 *
 * @param device The device to create a lockdownd client for
 * @param client The pointer to the location of the new lockdownd_client
 * @param label The label to use for communication. Usually the program name.
 *  Pass NULL to disable sending the label in requests to lockdownd.
 *
 * @return LOCKDOWN_E_SUCCESS on success, LOCKDOWN_E_INVALID_ARG when client is NULL,
 *  LOCKDOWN_E_INVALID_CONF if configuration data is wrong
 *
 * @原型 lockdownd_error_t lockdownd_client_new_with_handshake(idevice_t device, lockdownd_client_t *client, const char *label);
 */
typedef lockdownd_error_t (*lockdownd_client_new_with_handshake_func)(
    idevice_t device,
    lockdownd_client_t *client,
    const char *label
);

/**
 * Closes the lockdownd client session if one is running and frees up the
 * lockdownd_client struct.
 *
 * @param client The lockdown client
 *
 * @return LOCKDOWN_E_SUCCESS on success, LOCKDOWN_E_INVALID_ARG when client is NULL
 *
 * @原型 lockdownd_error_t lockdownd_client_free(lockdownd_client_t client);
 */
typedef lockdownd_error_t (*lockdownd_client_free_func)(lockdownd_client_t client);

/**
 * Retrieves a preferences plist using an optional domain and/or key name.
 *
 * @param client An initialized lockdownd client.
 * @param domain The domain to query on or NULL for global domain
 * @param key The key name to request or NULL to query for all keys
 * @param value A plist node representing the result value node
 *
 * @return LOCKDOWN_E_SUCCESS on success, LOCKDOWN_E_INVALID_ARG when client is NULL
 *
 * @原型 lockdownd_error_t lockdownd_get_value(lockdownd_client_t client, const char *domain, const char *key, plist_t *value);
 */
typedef lockdownd_error_t (*lockdownd_get_value_func)(
    lockdownd_client_t client,
    const char *domain,
    const char *key,
    plist_t *value
);

/**
 * Requests to start a service and retrieve it's port on success.
 * Sends the escrow bag from the device's pair record.
 *
 * @param client The lockdownd client
 * @param identifier The identifier of the service to start
 * @param service The service descriptor on success or NULL on failure
 *
 * @return LOCKDOWN_E_SUCCESS on success, LOCKDOWN_E_INVALID_ARG if a parameter
 *  is NULL, LOCKDOWN_E_INVALID_SERVICE if the requested service is not known
 *  by the device, LOCKDOWN_E_START_SERVICE_FAILED if the service could not be
 *  started by the device
 *
 * @原型 lockdownd_error_t lockdownd_start_service(lockdownd_client_t client, const char *identifier, lockdownd_service_descriptor_t *service);
 */
typedef lockdownd_error_t (*lockdownd_start_service_func)(
    lockdownd_client_t client,
    const char *identifier,
    lockdownd_service_descriptor_t *service
);

/**
 * Frees memory of a service descriptor as returned by lockdownd_start_service()
 *
 * @param service The service descriptor to free
 *
 * @return LOCKDOWN_E_SUCCESS on success
 *
 * @原型 lockdownd_error_t lockdownd_service_descriptor_free(lockdownd_service_descriptor_t service);
 */
typedef lockdownd_error_t (*lockdownd_service_descriptor_free_func)(
    lockdownd_service_descriptor_t service
);

#endif // LOCKDOWN_DYNAMIC_H