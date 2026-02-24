/*
 * ============================================================================
 * 文件名: semantic.h
 * 描述  : 语义分析模块 - 第一遍扫描
 *
 * 功能：
 *  - 词法分析得到的 Token 流进行语法分析和语义检查
 *  - 建立符号表，收集所有标签、变量、过程等信息
 *  - 计算指令长度，分配地址
 *  - 检测语义错误（重复定义、不支持的操作数等）
 *
 * 设计：
 *  - PassOne: 第一遍扫描上下文
 *  - semantic_pass_one: 执行第一遍扫描，返回符号表和汇编信息
 *  - semantic_analyze: 对单条指令进行语义分析
 *
 * ============================================================================
 */

#ifndef __SEMANTIC_H__
#define __SEMANTIC_H__

#include "utils.h"
#include "error.h"
#include "tables.h"
#include "symtab.h"
#include "lexer.h"

/* ========================================================================= */
/* 常量定义 */
/* ========================================================================= */

#define SEMANTIC_MAX_OPERANDS    3
#define SEMANTIC_MAX_INSTRUCTION_LEN  15
#define SEMANTIC_CODE_SECTION_SIZE    0x10000

/* ========================================================================= */
/* 数据结构定义 */
/* ========================================================================= */

/*
 * 操作数类型分类
 */
typedef enum {
    OPERAND_NONE = 0,           /* 无操作数 */
    OPERAND_REGISTER,           /* 寄存器 */
    OPERAND_IMMEDIATE,          /* 立即数 */
    OPERAND_MEMORY,             /* 内存地址 */
    OPERAND_LABEL,              /* 标签/符号 */
    OPERAND_INVALID             /* 无效操作数 */
} OperandType;

/*
 * 操作数结构
 */
typedef struct {
    OperandType type;
    u32 value;                  /* 寄存器号、立即数、地址等 */
    s8 name[128];               /* 符号名，如于标签或变量 */
} Operand;

/*
 * 指令信息（第一遍扫描阶段收集的指令信息）
 */
typedef struct {
    u32 address;                /* 指令在代码段中的地址 */
    u32 length;                 /* 指令长度（字节） */
    u32 line;                   /* 指令对应的源代码行号 */
    s8 mnemonic[32];            /* 助记符 */
    Operand operands[SEMANTIC_MAX_OPERANDS];
    u32 operand_count;          /* 实际操作数数量 */
    u32 has_label;              /* 是否具有标签前缀 */
    s8 label[128];              /* 标签名 */
} InstructionEntry;

/*
 * 第一遍扫描上下文
 */
typedef struct {
    SymbolTable* symtab;        /* 符号表 */
    InstructionEntry* instructions;  /* 指令列表 */
    u32 instruction_count;      /* 指令总数 */
    u32 max_instructions;       /* 指令列表容量 */
    u32 current_address;        /* 当前代码地址（第一遍结束时为代码长度） */
    u32 current_line;           /* 当前行号 */
    u32 has_errors;             /* 是否发生错误 */
} PassOne;

/* ========================================================================= */
/* API 函数声明 */
/* ========================================================================= */

/*
 * semantic_pass_one
 *
 * 功能：执行第一遍扫描（语法与语义收集）
 *
 * 参数：
 *   - tokens: Token 数组
 *   - token_count: Token 总数
 *
 * 返回值：
 *   - PassOne* : 第一遍扫描上下文，包含符号表和指令列表
 *   - NULL: 发生错误
 *
 * 描述：
 *   遍历 Token 流，识别指令和伪指令，建立符号表，
 *   计算每条指令的地址和长度。
 */
PassOne* semantic_pass_one(const Token* tokens, u32 token_count);

/*
 * semantic_analyze_instruction
 *
 * 功能：分析单条指令，提取助记符、操作数、标签等信息
 *
 * 参数：
 *   - pass_one: 第一遍扫描上下文
 *   - tokens: Token 数组
 *   - token_index: 当前 Token 索引
 *   - out_entry: 输出指令信息
 *
 * 返回值：
 *   - >=0: 成功处理的 Token 数
 *   - -1: 发生错误
 *
 * 描述：
 *   从指定 Token 位置开始，解析一条指令或伪指令，
 *   填充 out_entry 结构。
 */
int semantic_analyze_instruction(
    PassOne* pass_one,
    const Token* tokens,
    u32 token_index,
    InstructionEntry* out_entry
);

/*
 * semantic_get_instruction_length
 *
 * 功能：获取指令编码后的字节长度
 *
 * 参数：
 *   - mnemonic: 助记符
 *   - operand_types: 操作数类型数组
 *   - operand_count: 操作数个数
 *
 * 返回值：
 *   - u32: 指令长度（字节）
 *
 * 描述：
 *   根据指令类型和操作数计算机器码长度。
 *   如：MOV reg, immediate = 2 + 2 或 3 字节等
 */
u32 semantic_get_instruction_length(
    const s8* mnemonic,
    const OperandType* operand_types,
    u32 operand_count
);

/*
 * semantic_validate_operand
 *
 * 功能：验证操作数的合法性
 *
 * 参数：
 *   - mnemonic: 助记符
 *   - position: 操作数位置（0=第一个，1=第二个等）
 *   - operand: 操作数
 *
 * 返回值：
 *   - 0: 合法
 *   - -1: 非法
 *
 * 描述：
 *   检查操作数是否符合指令要求
 *   如：MOV 的第一个操作数不能是立即数等
 */
int semantic_validate_operand(
    const s8* mnemonic,
    u32 position,
    const Operand* operand
);

/*
 * semantic_pass_one_destroy
 *
 * 功能：销毁第一遍扫描上下文
 *
 * 参数：
 *   - pass_one: 上下文指针
 *
 * 返回值：无
 */
void semantic_pass_one_destroy(PassOne* pass_one);

#endif /* __SEMANTIC_H__ */


