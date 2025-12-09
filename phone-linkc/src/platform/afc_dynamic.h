/**
 * @file afc_dynamic.h
 * @brief AFC (Apple File Conduit) 动态库函数指针类型定义
 *
 * 本文件定义了用于动态加载 AFC 服务相关函数的类型。
 * AFC 服务用于访问 iOS 设备的文件系统。
 */

#ifndef AFC_DYNAMIC_H
#define AFC_DYNAMIC_H

#include <libimobiledevice/afc.h>
#include <libimobiledevice/lockdown.h>

/* ============================================================================
 * AFC 函数指针类型定义
 * ============================================================================ */

/**
 * 创建 AFC 客户端
 * @原型 afc_error_t afc_client_new(idevice_t device, lockdownd_service_descriptor_t service, afc_client_t *client);
 */
typedef afc_error_t (*afc_client_new_func)(idevice_t device, lockdownd_service_descriptor_t service, afc_client_t *client);

/**
 * 启动 AFC 服务并创建客户端
 * @原型 afc_error_t afc_client_start_service(idevice_t device, afc_client_t* client, const char* label);
 */
typedef afc_error_t (*afc_client_start_service_func)(idevice_t device, afc_client_t* client, const char* label);

/**
 * 释放 AFC 客户端
 * @原型 afc_error_t afc_client_free(afc_client_t client);
 */
typedef afc_error_t (*afc_client_free_func)(afc_client_t client);

/**
 * 获取设备信息
 * @原型 afc_error_t afc_get_device_info(afc_client_t client, char ***device_information);
 */
typedef afc_error_t (*afc_get_device_info_func)(afc_client_t client, char ***device_information);

/**
 * 读取目录内容
 * @原型 afc_error_t afc_read_directory(afc_client_t client, const char *path, char ***directory_information);
 */
typedef afc_error_t (*afc_read_directory_func)(afc_client_t client, const char *path, char ***directory_information);

/**
 * 获取文件信息
 * @原型 afc_error_t afc_get_file_info(afc_client_t client, const char *path, char ***file_information);
 */
typedef afc_error_t (*afc_get_file_info_func)(afc_client_t client, const char *path, char ***file_information);

/**
 * 打开文件
 * @原型 afc_error_t afc_file_open(afc_client_t client, const char *filename, afc_file_mode_t file_mode, uint64_t *handle);
 */
typedef afc_error_t (*afc_file_open_func)(afc_client_t client, const char *filename, afc_file_mode_t file_mode, uint64_t *handle);

/**
 * 关闭文件
 * @原型 afc_error_t afc_file_close(afc_client_t client, uint64_t handle);
 */
typedef afc_error_t (*afc_file_close_func)(afc_client_t client, uint64_t handle);

/**
 * 读取文件内容
 * @原型 afc_error_t afc_file_read(afc_client_t client, uint64_t handle, char *data, uint32_t length, uint32_t *bytes_read);
 */
typedef afc_error_t (*afc_file_read_func)(afc_client_t client, uint64_t handle, char *data, uint32_t length, uint32_t *bytes_read);

/**
 * 写入文件内容
 * @原型 afc_error_t afc_file_write(afc_client_t client, uint64_t handle, const char *data, uint32_t length, uint32_t *bytes_written);
 */
typedef afc_error_t (*afc_file_write_func)(afc_client_t client, uint64_t handle, const char *data, uint32_t length, uint32_t *bytes_written);

/**
 * 创建目录
 * @原型 afc_error_t afc_make_directory(afc_client_t client, const char *path);
 */
typedef afc_error_t (*afc_make_directory_func)(afc_client_t client, const char *path);

/**
 * 删除文件或目录
 * @原型 afc_error_t afc_remove_path(afc_client_t client, const char *path);
 */
typedef afc_error_t (*afc_remove_path_func)(afc_client_t client, const char *path);

/**
 * 重命名路径
 * @原型 afc_error_t afc_rename_path(afc_client_t client, const char *from, const char *to);
 */
typedef afc_error_t (*afc_rename_path_func)(afc_client_t client, const char *from, const char *to);

/**
 * 释放字典
 * @原型 afc_error_t afc_dictionary_free(char **dictionary);
 */
typedef afc_error_t (*afc_dictionary_free_func)(char **dictionary);

#endif // AFC_DYNAMIC_H