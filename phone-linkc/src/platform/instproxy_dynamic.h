/**
 * @file instproxy_dynamic.h
 * @brief installation_proxy 动态库函数指针类型定义
 *
 * 本文件定义了用于动态加载 installation_proxy 服务相关函数的类型。
 * installation_proxy 服务用于管理 iOS 设备上的应用（安装、卸载、列出应用等）。
 */

#ifndef INSTPROXY_DYNAMIC_H
#define INSTPROXY_DYNAMIC_H

#include <libimobiledevice/installation_proxy.h>
#include <libimobiledevice/lockdown.h>

/* ============================================================================
 * installation_proxy 函数指针类型定义
 * ============================================================================ */

/**
 * 创建 installation_proxy 客户端
 * @原型 instproxy_error_t instproxy_client_new(idevice_t device, lockdownd_service_descriptor_t service, instproxy_client_t *client);
 */
typedef instproxy_error_t (*instproxy_client_new_func)(idevice_t device, lockdownd_service_descriptor_t service, instproxy_client_t *client);

/**
 * 释放 installation_proxy 客户端
 * @原型 instproxy_error_t instproxy_client_free(instproxy_client_t client);
 */
typedef instproxy_error_t (*instproxy_client_free_func)(instproxy_client_t client);

/**
 * 列出已安装的应用
 * @原型 instproxy_error_t instproxy_browse(instproxy_client_t client, plist_t client_options, plist_t *result);
 */
typedef instproxy_error_t (*instproxy_browse_func)(instproxy_client_t client, plist_t client_options, plist_t *result);

/**
 * 安装应用
 * @原型 instproxy_error_t instproxy_install(instproxy_client_t client, const char *pkg_path, plist_t client_options, instproxy_status_cb_t status_cb, void *user_data);
 */
typedef instproxy_error_t (*instproxy_install_func)(instproxy_client_t client, const char *pkg_path, plist_t client_options, instproxy_status_cb_t status_cb, void *user_data);

/**
 * 卸载应用
 * @原型 instproxy_error_t instproxy_uninstall(instproxy_client_t client, const char *appid, plist_t client_options, instproxy_status_cb_t status_cb, void *user_data);
 */
typedef instproxy_error_t (*instproxy_uninstall_func)(instproxy_client_t client, const char *appid, plist_t client_options, instproxy_status_cb_t status_cb, void *user_data);

/**
 * 查询应用信息
 * @原型 instproxy_error_t instproxy_lookup(instproxy_client_t client, const char** appids, plist_t client_options, plist_t *result);
 */
typedef instproxy_error_t (*instproxy_lookup_func)(instproxy_client_t client, const char** appids, plist_t client_options, plist_t *result);

#endif // INSTPROXY_DYNAMIC_H