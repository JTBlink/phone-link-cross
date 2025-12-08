/**
 * @file plist_dynamic.h
 * @brief plist 动态库加载器头文件
 *
 * 本文件定义了用于在运行时动态加载 plist 库的函数指针类型。
 * plist (Property List) 是 Apple 使用的数据序列化格式，
 * iOS 设备通信中大量使用 plist 格式传输数据。
 *
 * @note 本文件被 libimobiledevice_dynamic.h 包含使用
 *
 * @see https://libimobiledevice.org/
 * @see https://github.com/libimobiledevice/libplist
 */

#ifndef PLIST_DYNAMIC_H
#define PLIST_DYNAMIC_H

/* ============================================================================
 * plist 官方头文件
 *
 * 直接使用官方头文件以确保类型定义与库完全一致，避免 ABI 不兼容问题。
 * ============================================================================ */

#include <plist/plist.h>

/* ============================================================================
 * plist 函数指针类型定义
 *
 * 这些类型定义用于动态加载 plist 库函数。
 * ============================================================================ */

/**
 * Destruct a plist_t node and all its children recursively
 *
 * @param plist the plist to free
 *
 * @原型 void plist_free(plist_t plist);
 */
typedef void (*plist_free_func)(plist_t plist);

/**
 * Get the #plist_type of a node.
 *
 * @param node the node
 * @return the type of the node
 *
 * @原型 plist_type plist_get_node_type(plist_t node);
 */
typedef plist_type (*plist_get_node_type_func)(plist_t node);

/**
 * Get the value of a #PLIST_STRING node.
 * This function does nothing if node is not of type #PLIST_STRING
 *
 * @param node the node
 * @param val a pointer to a C-string. This function allocates the memory,
 *            caller is responsible for freeing it. Data is UTF-8 encoded.
 *
 * @原型 void plist_get_string_val(plist_t node, char **val);
 */
typedef void (*plist_get_string_val_func)(plist_t node, char **val);

/**
 * Get the value of a #PLIST_BOOLEAN node.
 * This function does nothing if node is not of type #PLIST_BOOLEAN
 *
 * @param node the node
 * @param val a pointer to a uint8_t variable.
 *
 * @原型 void plist_get_bool_val(plist_t node, uint8_t *val);
 */
typedef void (*plist_get_bool_val_func)(plist_t node, uint8_t *val);

/**
 * Get the value of a #PLIST_UINT node.
 * This function does nothing if node is not of type #PLIST_UINT
 *
 * @param node the node
 * @param val a pointer to a uint64_t variable.
 *
 * @原型 void plist_get_uint_val(plist_t node, uint64_t *val);
 */
typedef void (*plist_get_uint_val_func)(plist_t node, uint64_t *val);

#endif // PLIST_DYNAMIC_H