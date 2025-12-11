/**
 * @file mobilebackup2_dynamic.h
 * @brief mobilebackup2 动态库加载器头文件
 */

#ifndef MOBILEBACKUP2_DYNAMIC_H
#define MOBILEBACKUP2_DYNAMIC_H

#include <libimobiledevice/mobilebackup2.h>

typedef mobilebackup2_error_t (*mobilebackup2_client_new_func)(idevice_t device, lockdownd_service_descriptor_t service, mobilebackup2_client_t * client);
typedef mobilebackup2_error_t (*mobilebackup2_client_start_service_func)(idevice_t device, mobilebackup2_client_t* client, const char* label);
typedef mobilebackup2_error_t (*mobilebackup2_client_free_func)(mobilebackup2_client_t client);
typedef mobilebackup2_error_t (*mobilebackup2_send_message_func)(mobilebackup2_client_t client, const char *message, plist_t options);
typedef mobilebackup2_error_t (*mobilebackup2_receive_message_func)(mobilebackup2_client_t client, plist_t *msg_plist, char **dlmessage);
typedef mobilebackup2_error_t (*mobilebackup2_send_raw_func)(mobilebackup2_client_t client, const char *data, uint32_t length, uint32_t *bytes);
typedef mobilebackup2_error_t (*mobilebackup2_receive_raw_func)(mobilebackup2_client_t client, char *data, uint32_t length, uint32_t *bytes);
typedef mobilebackup2_error_t (*mobilebackup2_version_exchange_func)(mobilebackup2_client_t client, double local_versions[], char count, double *remote_version);
typedef mobilebackup2_error_t (*mobilebackup2_send_request_func)(mobilebackup2_client_t client, const char *request, const char *target_identifier, const char *source_identifier, plist_t options);
typedef mobilebackup2_error_t (*mobilebackup2_send_status_response_func)(mobilebackup2_client_t client, int status_code, const char *status1, plist_t status2);

#endif // MOBILEBACKUP2_DYNAMIC_H