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
 * Get a pointer to the buffer of a #PLIST_STRING node.
 *
 * @note DO NOT MODIFY the buffer. Mind that the buffer is only available
 *   until the plist node gets freed. Make a copy if needed.
 *
 * @param node The node
 * @param length If non-NULL, will be set to the length of the string
 *
 * @return Pointer to the NULL-terminated buffer.
 *
 * @原型 const char* plist_get_string_ptr(plist_t node, uint64_t* length);
 */
typedef const char* (*plist_get_string_ptr_func)(plist_t node, uint64_t* length);

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

/**
 * Get the value of a #PLIST_DATA node.
 * This function does nothing if node is not of type #PLIST_DATA
 *
 * @param node the node
 * @param val a pointer to a char* buffer. This function allocates the memory,
 *            caller is responsible for freeing it.
 * @param length a pointer to a uint64_t variable.
 *
 * @原型 void plist_get_data_val(plist_t node, char **val, uint64_t *length);
 */
typedef void (*plist_get_data_val_func)(plist_t node, char **val, uint64_t *length);

/**
 * Create a new root plist_t type #PLIST_DICT
 *
 * @return the created plist
 *
 * @原型 plist_t plist_new_dict(void);
 */
typedef plist_t (*plist_new_dict_func)(void);

/**
 * Create a new plist_t type #PLIST_STRING
 *
 * @param val the sting value, encoded in UTF8.
 * @return the created item
 *
 * @原型 plist_t plist_new_string(const char *val);
 */
typedef plist_t (*plist_new_string_func)(const char *val);

/**
 * Create a new plist_t type #PLIST_BOOLEAN
 *
 * @param val the boolean value, 0 is false, other values are true.
 * @return the created item
 *
 * @原型 plist_t plist_new_bool(uint8_t val);
 */
typedef plist_t (*plist_new_bool_func)(uint8_t val);

/**
 * Set item identified by key in a #PLIST_DICT node.
 * The previous item identified by key will be freed using #plist_free.
 * If there is no item for the given key a new item will be inserted.
 *
 * @param node the node of type #PLIST_DICT
 * @param key the identifier of the item to set.
 * @param item the new item associated to key
 *
 * @原型 void plist_dict_set_item(plist_t node, const char* key, plist_t item);
 */
typedef void (*plist_dict_set_item_func)(plist_t node, const char* key, plist_t item);

/**
 * Get size of a #PLIST_ARRAY node.
 *
 * @param node the node of type #PLIST_ARRAY
 * @return size of the #PLIST_ARRAY node
 *
 * @原型 uint32_t plist_array_get_size(plist_t node);
 */
typedef uint32_t (*plist_array_get_size_func)(plist_t node);

/**
 * Get the nth item in a #PLIST_ARRAY node.
 *
 * @param node the node of type #PLIST_ARRAY
 * @param n the index of the item to get. Range is [0, array_size[
 * @return the nth item or NULL if node is not of type #PLIST_ARRAY
 *
 * @原型 plist_t plist_array_get_item(plist_t node, uint32_t n);
 */
typedef plist_t (*plist_array_get_item_func)(plist_t node, uint32_t n);

/**
 * Get the item in a #PLIST_DICT node.
 *
 * @param node the node of type #PLIST_DICT
 * @param key the identifier of the item to get.
 * @return the item or NULL if node is not of type #PLIST_DICT. The caller should not free
 *		the returned node.
 *
 * @原型 plist_t plist_dict_get_item(plist_t node, const char* key);
 */
typedef plist_t (*plist_dict_get_item_func)(plist_t node, const char* key);

/**
 * Create an iterator of a #PLIST_DICT node.
 * The allocated iterator should be freed with the standard free function.
 *
 * @param node The node of type #PLIST_DICT.
 * @param iter Location to store the iterator for the dictionary.
 *
 * @原型 void plist_dict_new_iter(plist_t node, plist_dict_iter *iter);
 */
typedef void (*plist_dict_new_iter_func)(plist_t node, plist_dict_iter *iter);

/**
 * Increment iterator of a #PLIST_DICT node.
 *
 * @param node The node of type #PLIST_DICT
 * @param iter Iterator of the dictionary
 * @param key Location to store the key, or NULL. The caller is responsible
 *		for freeing the the returned string.
 * @param val Location to store the value, or NULL. The caller must *not*
 *		free the returned value. Will be set to NULL when no more
 *		key/value pairs are left to iterate.
 *
 * @原型 void plist_dict_next_item(plist_t node, plist_dict_iter iter, char **key, plist_t *val);
 */
typedef void (*plist_dict_next_item_func)(plist_t node, plist_dict_iter iter, char **key, plist_t *val);

/**
 * Create a new root plist_t type #PLIST_ARRAY
 *
 * @return the created plist
 *
 * @原型 plist_t plist_new_array(void);
 */
typedef plist_t (*plist_new_array_func)(void);

/**
 * Append a new item at the end of a #PLIST_ARRAY node.
 *
 * @param node the node of type #PLIST_ARRAY
 * @param item the new item
 *
 * @原型 void plist_array_append_item(plist_t node, plist_t item);
 */
typedef void (*plist_array_append_item_func)(plist_t node, plist_t item);

#endif // PLIST_DYNAMIC_H