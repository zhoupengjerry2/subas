/**
 * tables.h - 指令表和伪指令表驱动模块头文件
 *
 * 本模块封装汇编器支持的所有指令和伪指令的定义，采用表驱动设计方便扩展。
 *
 * 设计目标：
 *  - 提供统一的指令/伪指令查询接口
 *  - 支持快速定位指令的属性（操作码、操作数个数等）
 *  - 便于后续添加更多指令而无需修改解析逻辑
 *
 * 表结构：
 *  - MASM 指令表：MOV, ADD, SUB, MUL, DIV, CMP 等
 *  - 跳转指令表：JMP, JZ, JNZ, LOOP 等
 *  - 伪指令表：SEGMENT, ENDS, ASSUME, ORG, DB, PROC, ENDP, END 等
 */

#ifndef __TABLES_H__
#define __TABLES_H__

#include "utils.h"

/* ============================================================================
 * 指令/伪指令类型枚举
 * ============================================================================ */

typedef enum {
    /* 一般数据操作指令 */
    INSTR_MOV = 0x01,
    INSTR_ADD = 0x02,
    INSTR_SUB = 0x03,
    INSTR_MUL = 0x04,
    INSTR_DIV = 0x05,
    INSTR_CMP = 0x06,

    /* 位操作指令 */
    INSTR_AND = 0x07,
    INSTR_OR  = 0x08,
    INSTR_XOR = 0x09,
    INSTR_SHL = 0x0A,
    INSTR_SHR = 0x0B,

    /* 跳转指令 */
    INSTR_JMP = 0x10,
    INSTR_JZ  = 0x11,
    INSTR_JNZ = 0x12,
    INSTR_JC  = 0x13,
    INSTR_JNC = 0x14,
    INSTR_LOOP = 0x15,

    /* 栈操作 */
    INSTR_PUSH = 0x20,
    INSTR_POP  = 0x21,
    INSTR_CALL = 0x22,
    INSTR_RET  = 0x23,

    /* 标志位操作 */
    INSTR_CLC = 0x30,
    INSTR_STC = 0x31,

    /* 中断 */
    INSTR_INT = 0x40,

    /* No-op 指令 */
    INSTR_NOP = 0x41,

    /* 伪指令 */
    PSEUDO_SEGMENT = 0x80,
    PSEUDO_ENDS    = 0x81,
    PSEUDO_ASSUME  = 0x82,
    PSEUDO_ORG     = 0x83,
    PSEUDO_DB      = 0x84,
    PSEUDO_PROC    = 0x85,
    PSEUDO_ENDP    = 0x86,
    PSEUDO_END     = 0x87,

    /* 特殊 */
    INSTR_NONE = 0xFF
} InstructionType;

/* ============================================================================
 * 指令信息结构体
 * ============================================================================ */

/*
 * 指令属性结构体：记录一条指令的所有必要属性
 * 用于驱动表-查询指令特性（操作码、操作数个数、是否支持等）
 */
typedef struct {
    const char* mnemonic;       /* 助记符（如 "MOV", "ADD"） */
    InstructionType type;       /* 指令类型枚举 */
    u8 opcode;                  /* 机器码操作码 */
    u8 operand_count;           /* 操作数个数（0-3） */
    int is_pseudo;              /* 是否为伪指令 */
    const char* description;    /* 描述信息 */
} InstructionInfo;

/* ============================================================================
 * 公共接口函数
 * ============================================================================ */

/*
 * 函数: tables_init
 * 描述: 初始化指令表（当前为空操作，因为表是常数）
 */
void tables_init(void);

/*
 * 函数: tables_lookup_instruction
 * 描述: 按助记符查找指令定义。
 * 参数: mnemonic - 要查找的助记符（如 "MOV"），不区分大小写
 * 返回: 指向 InstructionInfo 的指针，未找到返回 NULL_PTR
 */
const InstructionInfo* tables_lookup_instruction(const char* mnemonic);

/*
 * 函数: tables_is_pseudo
 * 描述: 检查某个助记符是否为伪指令
 * 返回: 1 为伪指令，0 为普通指令，-1 表示不存在
 */
int tables_is_pseudo(const char* mnemonic);

/*
 * 函数: tables_get_instruction_count
 * 描述: 获取指令表中指令总数（包括伪指令）
 */
u32 tables_get_instruction_count(void);

/*
 * 函数: tables_get_instruction_by_index
 * 描述: 按索引获取指令定义（用于遍历）
 * 参数: index - 指令索引
 * 返回: 指向 InstructionInfo 的指针，超出范围返回 NULL_PTR
 */
const InstructionInfo* tables_get_instruction_by_index(u32 index);

#endif /* __TABLES_H__ */


