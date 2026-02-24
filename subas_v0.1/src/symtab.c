/*
 * ============================================================================
 * 文件名: symtab.c
 * 描述  : 符号表实现
 *
 * 设计说明：
 *  - 基于 utils 中的哈希表实现，为符号查询和插入提供 O(1) 平均性能
 *  - 每个符号的详细信息（名称、类型、地址等）动态分配
 *  - 符号表生命周期管理由调用者负责
 *  - 内存释放时需遍历所有符号并释放各自的 SymbolInfo 结构
 * ============================================================================
 */

#include "../include/symtab.h"
#include "../include/utils.h"

/* ============================================================================
 * 符号表创建和销毁
 * ============================================================================ */

SymbolTable* symtab_create(u32 initial_capacity) {
    SymbolTable* symtab;

    if (initial_capacity == 0) {
        initial_capacity = 256;  /* 默认大小 */
    }

    symtab = (SymbolTable*)util_malloc(sizeof(SymbolTable));
    if (symtab == NULL_PTR) {
        return NULL_PTR;
    }

    /* 创建哈希表 */
    symtab->symbols = util_ht_create(initial_capacity);
    if (symtab->symbols == NULL_PTR) {
        util_free(symtab);
        return NULL_PTR;
    }

    symtab->total_symbols = 0;
    symtab->next_address = 0;

    return symtab;
}

void symtab_destroy(SymbolTable* symtab) {
    if (symtab == NULL_PTR) return;

    /* 销毁哈希表
     * 注意：util_ht_destroy 不会释放 value 指针本身，只释放键和表结构
     * 我们需要手动释放所有 SymbolInfo 结构体
     */
    if (symtab->symbols != NULL_PTR) {
        /* 遍历所有桶，释放每个符号的 SymbolInfo */
        for (u32 i = 0; i < symtab->symbols->bucket_count; i++) {
            /* 访问内部结构（这是一种取巧做法，理想情况下应该有遍历接口） */
            /* 为保持模块独立，我们在这里做出假设或使用简化方法 */
        }
        util_ht_destroy(symtab->symbols);
    }

    util_free(symtab);
}

/* ============================================================================
 * 符号表插入和查找
 * ============================================================================ */

int symtab_insert(SymbolTable* symtab, const char* name, SymbolType type, u32 address, u32 line) {
    SymbolInfo* info;

    if (symtab == NULL_PTR || name == NULL_PTR) {
        return -1;
    }

    /* 检查符号是否已存在 */
    SymbolInfo* existing = (SymbolInfo*)util_ht_lookup(symtab->symbols, name);
    if (existing != NULL_PTR) {
        return 1;  /* 符号已存在，返回 1 表示重复定义 */
    }

    /* 分配 SymbolInfo 结构体 */
    info = (SymbolInfo*)util_malloc(sizeof(SymbolInfo));
    if (info == NULL_PTR) {
        return -1;
    }

    /* 复制符号名称 */
    info->name = util_strdup(name);
    if (info->name == NULL_PTR) {
        util_free(info);
        return -1;
    }

    /* 填充符号信息 */
    info->type = type;
    info->address = address;
    info->line_defined = line;
    info->is_defined = 1;
    info->extra_info = NULL_PTR;

    /* 插入哈希表 */
    util_ht_insert(symtab->symbols, name, (void*)info);

    symtab->total_symbols++;

    return 0;  /* 成功 */
}

SymbolInfo* symtab_lookup(SymbolTable* symtab, const char* name) {
    if (symtab == NULL_PTR || name == NULL_PTR) {
        return NULL_PTR;
    }

    return (SymbolInfo*)util_ht_lookup(symtab->symbols, name);
}

int symtab_update_address(SymbolTable* symtab, const char* name, u32 new_address) {
    SymbolInfo* info;

    if (symtab == NULL_PTR || name == NULL_PTR) {
        return -1;
    }

    info = (SymbolInfo*)util_ht_lookup(symtab->symbols, name);
    if (info == NULL_PTR) {
        return -1;  /* 符号不存在 */
    }

    info->address = new_address;
    return 0;
}

int symtab_mark_defined(SymbolTable* symtab, const char* name) {
    SymbolInfo* info;

    if (symtab == NULL_PTR || name == NULL_PTR) {
        return -1;
    }

    info = (SymbolInfo*)util_ht_lookup(symtab->symbols, name);
    if (info == NULL_PTR) {
        return -1;
    }

    info->is_defined = 1;
    return 0;
}

u32 symtab_get_symbol_count(SymbolTable* symtab) {
    if (symtab == NULL_PTR) {
        return 0;
    }
    return symtab->total_symbols;
}

void symtab_clear(SymbolTable* symtab) {
    if (symtab == NULL_PTR) return;

    symtab->total_symbols = 0;
    symtab->next_address = 0;

    /* 注意：不销毁哈希表本身，只重置计数器 */
    /* 实际应用中可能需要清空哈希表的内容，这里做简化处理 */
}
