/**
 * @file mobilesync_dynamic.h
 * @brief mobilesync 服务动态加载函数指针类型定义
 *
 * 本文件定义了用于动态加载 libimobiledevice 中 mobilesync 服务相关函数的类型。
 * mobilesync 服务用于同步 iOS 设备上的数据类，如通讯录、日历、书签等。
 */

#ifndef MOBILESYNC_DYNAMIC_H
#define MOBILESYNC_DYNAMIC_H

#include <libimobiledevice/mobilesync.h>

/* ============================================================================
 * mobilesync 服务函数指针类型定义
 * ============================================================================ */

/**
 * 启动 mobilesync 服务并创建客户端
 * @原型 mobilesync_error_t mobilesync_client_start_service(idevice_t device, mobilesync_client_t* client, const char* label);
 */
typedef mobilesync_error_t (*mobilesync_client_start_service_func)(idevice_t device, mobilesync_client_t* client, const char* label);

/**
 * 创建 mobilesync 客户端
 * @原型 mobilesync_error_t mobilesync_client_new(idevice_t device, lockdownd_service_descriptor_t service, mobilesync_client_t * client);
 */
typedef mobilesync_error_t (*mobilesync_client_new_func)(idevice_t device, lockdownd_service_descriptor_t service, mobilesync_client_t * client);

/**
 * 释放 mobilesync 客户端
 * @原型 mobilesync_error_t mobilesync_client_free(mobilesync_client_t client);
 */
typedef mobilesync_error_t (*mobilesync_client_free_func)(mobilesync_client_t client);

/**
 * 接收来自设备的数据
 * @原型 mobilesync_error_t mobilesync_receive(mobilesync_client_t client, plist_t *plist);
 */
typedef mobilesync_error_t (*mobilesync_receive_func)(mobilesync_client_t client, plist_t *plist);

/**
 * 发送数据到设备
 * @原型 mobilesync_error_t mobilesync_send(mobilesync_client_t client, plist_t plist);
 */
typedef mobilesync_error_t (*mobilesync_send_func)(mobilesync_client_t client, plist_t plist);

/**
 * 开始同步会话
 * @原型 mobilesync_error_t mobilesync_start(mobilesync_client_t client, const char *data_class, mobilesync_anchors_t anchors, uint64_t computer_data_class_version, mobilesync_sync_type_t *sync_type, uint64_t *device_data_class_version, char** error_description);
 */
typedef mobilesync_error_t (*mobilesync_start_func)(mobilesync_client_t client, const char *data_class, mobilesync_anchors_t anchors, uint64_t computer_data_class_version, mobilesync_sync_type_t *sync_type, uint64_t *device_data_class_version, char** error_description);

/**
 * 完成同步会话
 * @原型 mobilesync_error_t mobilesync_finish(mobilesync_client_t client);
 */
typedef mobilesync_error_t (*mobilesync_finish_func)(mobilesync_client_t client);

/**
 * 请求从设备获取所有记录
 * @原型 mobilesync_error_t mobilesync_get_all_records_from_device(mobilesync_client_t client);
 */
typedef mobilesync_error_t (*mobilesync_get_all_records_from_device_func)(mobilesync_client_t client);

/**
 * 接收设备的变更记录
 * @原型 mobilesync_error_t mobilesync_receive_changes(mobilesync_client_t client, plist_t *entities, uint8_t *is_last_record, plist_t *actions);
 */
typedef mobilesync_error_t (*mobilesync_receive_changes_func)(mobilesync_client_t client, plist_t *entities, uint8_t *is_last_record, plist_t *actions);

/**
 * 确认已处理从设备接收的变更
 * @原型 mobilesync_error_t mobilesync_acknowledge_changes_from_device(mobilesync_client_t client);
 */
typedef mobilesync_error_t (*mobilesync_acknowledge_changes_from_device_func)(mobilesync_client_t client);

/**
 * 创建锚点结构
 * @原型 mobilesync_error_t mobilesync_anchors_new(const char *device_anchor, const char *computer_anchor, mobilesync_anchors_t *anchor);
 */
typedef mobilesync_error_t (*mobilesync_anchors_new_func)(const char *device_anchor, const char *computer_anchor, mobilesync_anchors_t *anchor);

/**
 * 释放锚点结构
 * @原型 mobilesync_error_t mobilesync_anchors_free(mobilesync_anchors_t anchors);
 */
typedef mobilesync_error_t (*mobilesync_anchors_free_func)(mobilesync_anchors_t anchors);

/**
 * 取消同步会话
 * @原型 mobilesync_error_t mobilesync_cancel(mobilesync_client_t client, const char* reason);
 */
typedef mobilesync_error_t (*mobilesync_cancel_func)(mobilesync_client_t client, const char* reason);

#endif // MOBILESYNC_DYNAMIC_H