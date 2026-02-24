/*
 * ============================================================================
 * 文件名: codegen.h
 * 描述  : 代码生成模块 - 第二遍扫描
 *
 * 功能：
 *  - 利用第一遍扫描建立的符号表解决向前和向后的标签引用
 *  - 将指令翻译成机器码（定义操作码、操作码扩展等）
 *  - 处理立即数寻址、内存寻址等各种寻址方式
 *  - 生成最终的二进制代码
 *
 * 设计：
 *  - CodeGen: 第二遍扫描上下文
 *  - codegen_pass_two: 执行第二遍扫描，生成机器码
 *  - codegen_emit_instruction: 对单条指令生成机器码
 *  - codegen_resolve_reference: 解决标签引用
 *
 * ============================================================================
 */

#ifndef __CODEGEN_H__
#define __CODEGEN_H__

#include "utils.h"
#include "error.h"
#include "tables.h"
#include "symtab.h"
#include "semantic.h"

/* ========================================================================= */
/* 常量定义 */
/* ========================================================================= */

#define CODEGEN_OUTPUT_BUFFER_SIZE  0x10000
#define CODEGEN_MAX_RELOCATIONS     1000

/* ========================================================================= */
/* 数据结构定义 */
/* ========================================================================= */

/*
 * 重定位记录（处理前向和向后标签引用）
 * 用于在生成代码时记录需要后续修复的引用
 */
typedef struct {
    u32 offset;                 /* 代码中需要修复的位置偏移 */
    u32 instruction_index;      /* 对应的指令索引 */
    u32 operand_index;          /* 对应的操作数索引 */
    s8 symbol_name[128];        /* 符号名 */
} Relocation;

/*
 * 第二遍扫描上下文
 */
typedef struct {
    const PassOne* pass_one;    /* 第一遍扫描结果 */
    u8* code_buffer;            /* 代码输出缓冲区 */
    u32 code_size;              /* 当前代码大小（字节） */
    Relocation* relocations;    /* 重定位记录数组 */
    u32 relocation_count;       /* 重定位记录数 */
    u32 has_errors;             /* 是否发生错误 */
} CodeGen;

/* ========================================================================= */
/* API 函数声明 */
/* ========================================================================= */

/*
 * codegen_pass_two
 *
 * 功能：执行第二遍扫描（代码生成）
 *
 * 参数：
 *   - pass_one: 第一遍扫描结果
 *
 * 返回值：
 *   - CodeGen* : 第二遍扫描上下文，包含生成的代码
 *   - NULL: 发生错误
 *
 * 描述：
 *   遍历第一遍扫描收集的指令列表，依次生成机器码。
 *   如遇到标签引用，先记录重定位信息；当遍历完后，
 *   利用完整的符号表填充所有标签引用。
 */
CodeGen* codegen_pass_two(const PassOne* pass_one);

/*
 * codegen_emit_instruction
 *
 * 功能：生成单条指令的机器码
 *
 * 参数：
 *   - codegen: 代码生成上下文
 *   - entry: 指令信息
 *
 * 返回值：
 *   - 0: 成功
 *   - -1: 失败
 *
 * 描述：
 *   根据指令类型和操作数，将指令编码为机器码。
 *   如果操作数为标签引用，记录重定位信息。
 */
int codegen_emit_instruction(CodeGen* codegen, const InstructionEntry* entry);

/*
 * codegen_resolve_reference
 *
 * 功能：解决所有标签引用（向前和向后）
 *
 * 参数：
 *   - codegen: 代码生成上下文
 *
 * 返回值：
 *   - 0: 成功
 *   - -1: 存在未定义符号或其他错误
 *
 * 描述：
 *   遍历重定位记录表，对每条记录查找符号表中的地址，
 *   填充到代码缓冲区相应位置，完成标签引用解决。
 */
int codegen_resolve_reference(CodeGen* codegen);

/*
 * codegen_get_code_buffer
 *
 * 功能：获取生成的代码缓冲区
 *
 * 参数：
 *   - codegen: 代码生成上下文
 *   - out_size: 输出参数，返回代码大小
 *
 * 返回值：
 *   - u8* : 代码缓冲区指针
 *   - NULL: 空缓冲区
 */
u8* codegen_get_code_buffer(const CodeGen* codegen, u32* out_size);

/*
 * codegen_get_relocation_info
 *
 * 功能：获取重定位信息
 *
 * 参数：
 *   - codegen: 代码生成上下文
 *   - out_count: 输出参数，返回重定位记录数
 *
 * 返回值：
 *   - Relocation* : 重定位记录数组
 *   - NULL: 无重定位
 */
Relocation* codegen_get_relocation_info(const CodeGen* codegen, u32* out_count);

/*
 * codegen_destroy
 *
 * 功能：销毁代码生成上下文
 *
 * 参数：
 *   - codegen: 上下文指针
 *
 * 返回值：无
 */
void codegen_destroy(CodeGen* codegen);

#endif /* __CODEGEN_H__ */
