/**
 * @brief utils.h - 核心基础设施与工具库头文件。
 * 为上层模块（如词法分析、符号表、表驱动解析）提供基础数据类型、
 * 安全的内存管理、底层字符串操作以及通用的哈希表数据结构。
 * 设计目标：极简、高内聚、零C标准库依赖泄漏、便于向汇编语言移植。
 * @author Jerry Zhou
 * @date 2026/02/23
 * @version 0.1
 * 
 */
#ifndef __UTILS_H__
#define __UTILS_H__


/* --------------------------------------------------------------------------
 * 1. 基础数据类型定义 (替代 <stdint.h> 和 <stdbool.h>，C90兼容)
 * -------------------------------------------------------------------------- */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;

typedef int                bool_t;
#define TRUE               1
#define FALSE              0
#define NULL_PTR           ((void*)0)

/* --------------------------------------------------------------------------
 * 2. 内存管理接口
 * 未来移植汇编时，可将 util_malloc 替换为自定义的堆分配器或系统调用
 * -------------------------------------------------------------------------- */

/*
 * 函数: util_malloc
 * 描述: 分配指定大小的内存，如果失败将触发系统级严重错误。
 * 参数: size - 需要分配的字节数
 * 返回: 指向已分配内存的指针
 */
void* util_malloc(u32 size);

/*
 * 函数: util_free
 * 描述: 释放由 util_malloc 分配的内存。
 * 参数: ptr - 指向需释放内存的指针
 */
void util_free(void* ptr);


/* --------------------------------------------------------------------------
 * 3. 基础字符串处理接口 (替代 <string.h>)
 * 全部采用纯指针操作实现，极易转换为汇编指令。
 * -------------------------------------------------------------------------- */

/*
 * 函数: util_strlen
 * 描述: 计算以空字符结尾的字符串的长度。
 */
u32 util_strlen(const char* str);

/*
 * 函数: util_strcpy
 * 描述: 将源字符串复制到目标地址。调用者需保证目标缓冲区足够大。
 */
char* util_strcpy(char* dest, const char* src);

/*
 * 函数: util_strcmp
 * 描述: 比较两个字符串。相等返回0，s1<s2返回负数，s1>s2返回正数。
 */
s32 util_strcmp(const char* s1, const char* s2);

/*
 * 函数: util_strdup
 * 描述: 复制字符串并为其分配新的内存空间。
 */
char* util_strdup(const char* str);

/*
 * 函数: util_memset
 * 描述: 将内存区域设置为指定字节值。
 * 参数: ptr - 内存地址，size - 字节数，value - 填充值
 */
void* util_memset(void* ptr, int value, u32 size);


/* --------------------------------------------------------------------------
 * 4. 通用哈希表数据结构 (用于符号表和指令表驱动)
 * 采用拉链法（Separate Chaining）解决哈希冲突。
 * -------------------------------------------------------------------------- */

/* 哈希表节点结构体 */
typedef struct UtilHashNode {
    char* key;                  /* 键 (字符串) */
    void* value;                /* 值 (泛型指针，可指向符号属性或指令定义) */
    struct UtilHashNode* next;  /* 链表下一个节点 (解决冲突) */
} UtilHashNode;

/* 哈希表结构体 */
typedef struct UtilHashTable {
    UtilHashNode** buckets;     /* 桶数组 */
    u32 bucket_count;           /* 桶的数量 */
    u32 element_count;          /* 当前表内元素总数 */
} UtilHashTable;

/*
 * 函数: util_ht_create
 * 描述: 创建并初始化一个哈希表。
 * 参数: bucket_count - 桶的数量（建议为素数以减少冲突）
 * 返回: 哈希表指针
 */
UtilHashTable* util_ht_create(u32 bucket_count);

/*
 * 函数: util_ht_insert
 * 描述: 向哈希表中插入键值对。如果键已存在，则更新其对应的值。
 * 参数: table - 哈希表指针
 * key   - 键（内部会通过 util_strdup 复制一份）
 * value - 泛型数据指针
 */
void util_ht_insert(UtilHashTable* table, const char* key, void* value);

/*
 * 函数: util_ht_lookup
 * 描述: 在哈希表中查找键对应的值。
 * 参数: table - 哈希表指针
 * key   - 要查找的键
 * 返回: 成功返回对应的值指针，失败返回 NULL_PTR
 */
void* util_ht_lookup(UtilHashTable* table, const char* key);

/*
 * 函数: util_ht_destroy
 * 描述: 销毁哈希表并释放所有相关内存。注意：此函数不释放 value 指针指向的内存。
 * 参数: table - 哈希表指针
 */
void util_ht_destroy(UtilHashTable* table);


#endif /* __UTILS_H__ */
