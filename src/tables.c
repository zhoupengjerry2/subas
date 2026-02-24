/*
 * ============================================================================
 * 文件名: tables.c
 * 描述  : 指令表和伪指令表驱动实现
 *
 * 设计说明：
 *  - 所有指令定义存储在常量表中，在编译时即确定
 *  - 通过统一的查询接口隐藏底层表结构，便于日后重构或扩展
 *  - 大小写不敏感地查找指令（转换为大写后比较）
 *  - 采用简单的线性查找，适合指令集规模较小的场景
 *
 * 后续优化：可以替换为哈希表以加速查找（对于大指令集）
 * ============================================================================
 */

#include "../include/tables.h"
#include "../include/utils.h"

/* ============================================================================
 * 指令定义表（常量表驱动）
 * ============================================================================ */

/*
 * 完整的指令和伪指令定义表
 * 未来可扩展以支持更多指令
 */
static const InstructionInfo g_instruction_table[] = {
    /* 一般数据操作指令 */
    {
        .mnemonic = "MOV",
        .type = INSTR_MOV,
        .opcode = 0x88,
        .operand_count = 2,
        .is_pseudo = 0,
        .description = "Move data between registers or memory"
    },
    {
        .mnemonic = "ADD",
        .type = INSTR_ADD,
        .opcode = 0x04,
        .operand_count = 2,
        .is_pseudo = 0,
        .description = "Add two operands"
    },
    {
        .mnemonic = "SUB",
        .type = INSTR_SUB,
        .opcode = 0x2C,
        .operand_count = 2,
        .is_pseudo = 0,
        .description = "Subtract second operand from first"
    },
    {
        .mnemonic = "MUL",
        .type = INSTR_MUL,
        .opcode = 0xF6,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Multiply accumulator by operand"
    },
    {
        .mnemonic = "DIV",
        .type = INSTR_DIV,
        .opcode = 0xF6,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Divide accumulator by operand"
    },
    {
        .mnemonic = "CMP",
        .type = INSTR_CMP,
        .opcode = 0x3C,
        .operand_count = 2,
        .is_pseudo = 0,
        .description = "Compare two operands and set flags"
    },
    
    /* 位操作指令 */
    {
        .mnemonic = "AND",
        .type = INSTR_AND,
        .opcode = 0x24,
        .operand_count = 2,
        .is_pseudo = 0,
        .description = "Bitwise AND"
    },
    {
        .mnemonic = "OR",
        .type = INSTR_OR,
        .opcode = 0x0C,
        .operand_count = 2,
        .is_pseudo = 0,
        .description = "Bitwise OR"
    },
    {
        .mnemonic = "XOR",
        .type = INSTR_XOR,
        .opcode = 0x34,
        .operand_count = 2,
        .is_pseudo = 0,
        .description = "Bitwise XOR"
    },
    {
        .mnemonic = "SHL",
        .type = INSTR_SHL,
        .opcode = 0xD0,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Shift left"
    },
    {
        .mnemonic = "SHR",
        .type = INSTR_SHR,
        .opcode = 0xD0,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Shift right"
    },
    
    /* 跳转指令 */
    {
        .mnemonic = "JMP",
        .type = INSTR_JMP,
        .opcode = 0xEB,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Unconditional jump"
    },
    {
        .mnemonic = "JZ",
        .type = INSTR_JZ,
        .opcode = 0x74,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Jump if zero"
    },
    {
        .mnemonic = "JNZ",
        .type = INSTR_JNZ,
        .opcode = 0x75,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Jump if not zero"
    },
    {
        .mnemonic = "JC",
        .type = INSTR_JC,
        .opcode = 0x72,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Jump if carry"
    },
    {
        .mnemonic = "JNC",
        .type = INSTR_JNC,
        .opcode = 0x73,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Jump if not carry"
    },
    {
        .mnemonic = "LOOP",
        .type = INSTR_LOOP,
        .opcode = 0xE2,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Loop while CX != 0"
    },
    
    /* 栈操作 */
    {
        .mnemonic = "PUSH",
        .type = INSTR_PUSH,
        .opcode = 0x50,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Push operand onto stack"
    },
    {
        .mnemonic = "POP",
        .type = INSTR_POP,
        .opcode = 0x58,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Pop from stack"
    },
    {
        .mnemonic = "CALL",
        .type = INSTR_CALL,
        .opcode = 0xE8,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Call subroutine"
    },
    {
        .mnemonic = "RET",
        .type = INSTR_RET,
        .opcode = 0xC3,
        .operand_count = 0,
        .is_pseudo = 0,
        .description = "Return from subroutine"
    },
    {
        .mnemonic = "NOP",
        .type = INSTR_NOP,
        .opcode = 0x90,
        .operand_count = 0,
        .is_pseudo = 0,
        .description = "No operation"
    },
    
    /* 标志位操作 */
    {
        .mnemonic = "CLC",
        .type = INSTR_CLC,
        .opcode = 0xF8,
        .operand_count = 0,
        .is_pseudo = 0,
        .description = "Clear carry flag"
    },
    {
        .mnemonic = "STC",
        .type = INSTR_STC,
        .opcode = 0xF9,
        .operand_count = 0,
        .is_pseudo = 0,
        .description = "Set carry flag"
    },
    
    /* 中断 */
    {
        .mnemonic = "INT",
        .type = INSTR_INT,
        .opcode = 0xCD,
        .operand_count = 1,
        .is_pseudo = 0,
        .description = "Call interrupt handler"
    },
    
    /* 伪指令 */
    {
        .mnemonic = "SEGMENT",
        .type = PSEUDO_SEGMENT,
        .opcode = 0x00,
        .operand_count = 0,
        .is_pseudo = 1,
        .description = "Define memory segment"
    },
    {
        .mnemonic = "ENDS",
        .type = PSEUDO_ENDS,
        .opcode = 0x00,
        .operand_count = 0,
        .is_pseudo = 1,
        .description = "End segment definition"
    },
    {
        .mnemonic = "ASSUME",
        .type = PSEUDO_ASSUME,
        .opcode = 0x00,
        .operand_count = 1,
        .is_pseudo = 1,
        .description = "Assume register segment association"
    },
    {
        .mnemonic = "ORG",
        .type = PSEUDO_ORG,
        .opcode = 0x00,
        .operand_count = 1,
        .is_pseudo = 1,
        .description = "Set origin address"
    },
    {
        .mnemonic = "DB",
        .type = PSEUDO_DB,
        .opcode = 0x00,
        .operand_count = 1,
        .is_pseudo = 1,
        .description = "Define byte(s)"
    },
    {
        .mnemonic = "PROC",
        .type = PSEUDO_PROC,
        .opcode = 0x00,
        .operand_count = 0,
        .is_pseudo = 1,
        .description = "Define procedure"
    },
    {
        .mnemonic = "ENDP",
        .type = PSEUDO_ENDP,
        .opcode = 0x00,
        .operand_count = 0,
        .is_pseudo = 1,
        .description = "End procedure"
    },
    {
        .mnemonic = "END",
        .type = PSEUDO_END,
        .opcode = 0x00,
        .operand_count = 0,
        .is_pseudo = 1,
        .description = "End assembly"
    },
};

/* 表大小：用于边界检查和遍历 */
static const u32 g_instruction_count = sizeof(g_instruction_table) / sizeof(InstructionInfo);

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/*
 * 将字符转换为大写（ASCII 字母）
 */
static char to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

/*
 * 不区分大小写的字符串比较
 */
static int strcasecmp_simple(const char* s1, const char* s2) {
    while (*s1 != '\0' && *s2 != '\0') {
        char c1 = to_upper(*s1);
        char c2 = to_upper(*s2);
        if (c1 != c2) {
            return (int)c1 - (int)c2;
        }
        s1++;
        s2++;
    }
    return (int)to_upper(*s1) - (int)to_upper(*s2);
}

/* ============================================================================
 * 公共接口实现
 * ============================================================================ */

void tables_init(void) {
    /* 当前为空操作：所有表都在编译时确定 */
}

const InstructionInfo* tables_lookup_instruction(const char* mnemonic) {
    if (mnemonic == NULL_PTR) {
        return NULL_PTR;
    }

    /* 线性查找：比较不区分大小写 */
    for (u32 i = 0; i < g_instruction_count; i++) {
        if (strcasecmp_simple(g_instruction_table[i].mnemonic, mnemonic) == 0) {
            return &g_instruction_table[i];
        }
    }

    return NULL_PTR;
}

int tables_is_pseudo(const char* mnemonic) {
    const InstructionInfo* info = tables_lookup_instruction(mnemonic);
    if (info == NULL_PTR) {
        return -1;  /* 指令不存在 */
    }
    return info->is_pseudo;
}

u32 tables_get_instruction_count(void) {
    return g_instruction_count;
}

const InstructionInfo* tables_get_instruction_by_index(u32 index) {
    if (index >= g_instruction_count) {
        return NULL_PTR;
    }
    return &g_instruction_table[index];
}
