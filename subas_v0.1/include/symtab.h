/**
 * symtab.h - 符号表模块头文件
 *
 * 本模块实现汇编器的符号表，用于维护标签、变量、过程等符号的定义和地址。
 *
 * 设计目标：
 *  - 提供符号的快速查询和插入（基于哈希表）
 *  - 记录符号的地址、类型、定义行号等属性
 *  - 支持两遍扫描：Pass 1 建立完整符号表，Pass 2 解决前向引用
 *  - 便于后续报告"未定义符号"等错误
 *
 * 符号表结构：
 *  - 标签（Label）：普通标签，指向某个地址
 *  - 变量（Variable）：数据符号（DB 定义）
 *  - 过程（Procedure）：PROC 定义的过程
 */

#ifndef __SYMTAB_H__
#define __SYMTAB_H__

#include "utils.h"

/* ============================================================================
 * 符号类型和属性定义
 * ============================================================================ */

typedef enum {
    SYM_LABEL = 0,
    SYM_VARIABLE = 1,
    SYM_PROCEDURE = 2
} SymbolType;

/*
 * 符号信息结构体：记录一个符号（标签、变量等）的所有属性
 */
typedef struct {
    char* name;                 /* 符号名称 */
    SymbolType type;            /* 符号类型 */
    u32 address;                /* 符号对应的地址（字节偏移） */
    u32 line_defined;           /* 符号定义时的源代码行号 */
    int is_defined;             /* 是否已定义（1）还是仅引用（0） */
    void* extra_info;           /* 扩展信息指针（可选，用于存储其他属性） */
} SymbolInfo;

/* ============================================================================
 * 符号表结构体
 * ============================================================================ */

/*
 * 前向声明：符号表的实现细节对外部隐藏
 * 外部仅通过 SymbolTable* 指针操作符号表
 */
typedef struct SymbolTable {
    UtilHashTable* symbols;     /* 哈希表：key = 符号名，value = SymbolInfo* */
    u32 total_symbols;          /* 符号总数 */
    u32 next_address;           /* 下一个可用地址（用于自动分配） */
} SymbolTable;

/* ============================================================================
 * 公共接口函数
 * ============================================================================ */

/*
 * 函数: symtab_create
 * 描述: 创建一个新的符号表
 * 参数: initial_capacity - 哈希表容量建议（会向上取整到适当大小）
 * 返回: 已分配的 SymbolTable*，失败返回 NULL_PTR
 */
SymbolTable* symtab_create(u32 initial_capacity);

/*
 * 函数: symtab_destroy
 * 描述: 销毁符号表并释放所有相关内存
 * 参数: symtab - 符号表指针
 */
void symtab_destroy(SymbolTable* symtab);

/*
 * 函数: symtab_insert
 * 描述: 向符号表中插入或更新一个符号
 * 参数: symtab - 符号表指针
 *       name   - 符号名称（将被复制）
 *       type   - 符号类型
 *       address - 符号地址
 *       line   - 定义行号
 * 返回: 成功返回 0，表已存在返回 1，失败返回 -1
 */
int symtab_insert(SymbolTable* symtab, const char* name, SymbolType type, u32 address, u32 line);

/*
 * 函数: symtab_lookup
 * 描述: 查找符号
 * 参数: symtab - 符号表指针
 *       name   - 要查找的符号名
 * 返回: 指向 SymbolInfo 的指针，未找到返回 NULL_PTR
 */
SymbolInfo* symtab_lookup(SymbolTable* symtab, const char* name);

/*
 * 函数: symtab_update_address
 * 描述: 更新符号的地址（用于前向引用解决）
 * 返回: 成功返回 0，符号不存在返回 -1
 */
int symtab_update_address(SymbolTable* symtab, const char* name, u32 new_address);

/*
 * 函数: symtab_mark_defined
 * 描述: 标记符号为已定义（区分声明和定义）
 * 返回: 成功返回 0，符号不存在返回 -1
 */
int symtab_mark_defined(SymbolTable* symtab, const char* name);

/*
 * 函数: symtab_get_symbol_count
 * 描述: 获取符号表中的符号总数
 */
u32 symtab_get_symbol_count(SymbolTable* symtab);

/*
 * 函数: symtab_clear
 * 描述: 清空符号表（重置地址计数但保留表结构）
 */
void symtab_clear(SymbolTable* symtab);

#endif /* __SYMTAB_H__ */


