/*
 * ============================================================================
 * 文件名: hash.c
 * 描述  : 哈希表实现文件。
 * 遵守 C90 规范，手工实现关键的底层操作，为汇编重写铺垫。
 * ============================================================================
 */

#include "../../include/utils.h"

/*
 * 内部辅助函数: DJB2 字符串哈希算法
 * 描述: 经典且简单的字符串哈希，易于汇编实现 (hash * 33 + c)
 */
static u32 hash_string_djb2(const char* str) {
    u32 hash = 5381;
    s32 c;
    while ((c = *str++) != '\0') {
        hash = ((hash << 5) + hash) + (u32)c; /* hash * 33 + c */
    }
    return hash;
}

UtilHashTable* util_ht_create(u32 bucket_count) {
    UtilHashTable* table;
    u32 i;
    
    if (bucket_count == 0) {
        bucket_count = 1031; /* 提供一个默认的素数大小 */
    }
    
    table = (UtilHashTable*)util_malloc(sizeof(UtilHashTable));
    if (table == NULL_PTR) return NULL_PTR;
    
    table->bucket_count = bucket_count;
    table->element_count = 0;
    
    /* 分配桶数组并初始化为空 */
    table->buckets = (UtilHashNode**)util_malloc(sizeof(UtilHashNode*) * bucket_count);
    for (i = 0; i < bucket_count; i++) {
        table->buckets[i] = NULL_PTR;
    }
    
    return table;
}

void util_ht_insert(UtilHashTable* table, const char* key, void* value) {
    u32 hash_val;
    u32 index;
    UtilHashNode* node;
    
    if (table == NULL_PTR || key == NULL_PTR) return;
    
    hash_val = hash_string_djb2(key);
    index = hash_val % table->bucket_count;
    
    /* 1. 检查键是否已经存在，如果存在则更新 value */
    node = table->buckets[index];
    while (node != NULL_PTR) {
        if (util_strcmp(node->key, key) == 0) {
            node->value = value;
            return;
        }
        node = node->next;
    }
    
    /* 2. 键不存在，创建新节点并采用头插法插入链表 */
    node = (UtilHashNode*)util_malloc(sizeof(UtilHashNode));
    node->key = util_strdup(key); /* 哈希表拥有 key 的所有权 */
    node->value = value;
    node->next = table->buckets[index];
    table->buckets[index] = node;
    
    table->element_count++;
}

void* util_ht_lookup(UtilHashTable* table, const char* key) {
    u32 hash_val;
    u32 index;
    UtilHashNode* node;
    
    if (table == NULL_PTR || key == NULL_PTR) return NULL_PTR;
    
    hash_val = hash_string_djb2(key);
    index = hash_val % table->bucket_count;
    
    node = table->buckets[index];
    while (node != NULL_PTR) {
        if (util_strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }
    
    return NULL_PTR; /* 未找到 */
}

void util_ht_destroy(UtilHashTable* table) {
    u32 i;
    UtilHashNode* current;
    UtilHashNode* next_node;
    
    if (table == NULL_PTR) return;
    
    /* 遍历所有桶，释放节点及其 key 字符串 */
    for (i = 0; i < table->bucket_count; i++) {
        current = table->buckets[i];
        while (current != NULL_PTR) {
            next_node = current->next;
            util_free(current->key); /* 释放 strdup 分配的内存 */
            util_free(current);
            current = next_node;
        }
    }
    
    util_free(table->buckets);
    util_free(table);
}
