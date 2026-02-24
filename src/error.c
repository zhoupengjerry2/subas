/*
 * ============================================================================
 * 文件名: error.c
 * 描述  : 统一错误处理与报告模块的实现。
 * 内部维护错误计数器，并根据错误码映射对应的标准提示信息。
 * ============================================================================
 */

#include "../include/error.h"
#include <stdio.h> /* 仅用于向 stderr 输出信息 */

/* --------------------------------------------------------------------------
 * 1. 静态内部状态
 * -------------------------------------------------------------------------- */
static u32 g_error_count = 0;

/* * 错误码与提示信息的映射结构体 
 */
typedef struct {
    ErrorCode code;
    const char* message;
} ErrorMapping;

/* * 错误信息表驱动设计
 * 便于后续扩展错误类型，无需修改逻辑代码。
 */
static const ErrorMapping g_error_table[] = {
    { ERR_LEX_INVALID_CHAR,    "Lexical Error: Invalid character encountered" },
    { ERR_LEX_UNCLOSED_STR,    "Lexical Error: Unclosed string literal" },
    { ERR_LEX_INVALID_NUM,     "Lexical Error: Invalid numeric constant" },
    
    { ERR_PARSE_EXPECTED_OP,   "Syntax Error: Expected operand missing" },
    { ERR_PARSE_INVALID_REG,   "Syntax Error: Invalid register name" },
    { ERR_PARSE_UNK_MNEMONIC,  "Syntax Error: Unknown instruction mnemonic" },
    { ERR_PARSE_DUP_LABEL,     "Symbol Error: Duplicate label definition" },
    { ERR_PARSE_UNDEFINED_LBL, "Symbol Error: Undefined reference to label" },
    
    { ERR_SYS_OUT_OF_MEM,      "System Error: Memory allocation failed" },
    { ERR_SYS_FILE_IO,         "System Error: File I/O operation failed" },
    
    { ERR_NONE,                NULL_PTR } /* 结束标记 */
};

/* --------------------------------------------------------------------------
 * 2. 内部辅助函数
 * -------------------------------------------------------------------------- */

/*
 * 函数: find_error_msg
 * 描述: 根据错误码在表中检索对应的字符串提示。
 */
static const char* find_error_msg(ErrorCode code) {
    u32 i = 0;
    while (g_error_table[i].message != NULL_PTR) {
        if (g_error_table[i].code == code) {
            return g_error_table[i].message;
        }
        i++;
    }
    return "Unknown Error Occurred";
}

/* --------------------------------------------------------------------------
 * 3. 公共接口实现
 * -------------------------------------------------------------------------- */

void error_init(void) {
    g_error_count = 0;
}

void error_report(u32 line_num, ErrorCode code, const char* detail) {
    const char* base_msg;
    
    base_msg = find_error_msg(code);
    g_error_count++;

    /* 统一错误格式输出: [Line XXX] Error E1001: Message (Detail) */
    fprintf(stderr, "[Line %u] Error E%d: %s", (unsigned int)line_num, (int)code, base_msg);
    
    if (detail != NULL_PTR) {
        fprintf(stderr, " -> %s", detail);
    }
    
    fprintf(stderr, "\n");
}

u32 error_get_count(void) {
    return g_error_count;
}

bool_t error_has_failed(void) {
    return (g_error_count > 0) ? TRUE : FALSE;
}
