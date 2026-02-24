/**
 * @brief error.h - 错误处理模块头文件。
 * 定义了错误代码枚举和错误报告接口，用于在汇编过程中统一处理各种错误情况，
 * 如语法错误、未知指令、未定义标签等。
 * 设计目标：提供清晰、统一的错误报告机制，便于调试和维护，同时保持与汇编
 * 语言的兼容性。
 * @author Jerry Zhou
 * @date 2026/02/23
 * @version 0.1
 *
 */
#ifndef __ERROR_H__
#define __ERROR_H__

#include "utils.h"

/* --------------------------------------------------------------------------
 * 1. 错误码定义 (Error Codes)
 * 采用四位错误码：1xxx 为词法错误，2xxx 为语法/语义错误，3xxx 为系统/内存错误
 * -------------------------------------------------------------------------- */
typedef enum {
    ERR_NONE                = 0,

    /* 词法错误 (Lexical Errors) */
    ERR_LEX_INVALID_CHAR    = 1001,  /* 非法字符 */
    ERR_LEX_UNCLOSED_STR    = 1002,  /* 字符串未闭合 */
    ERR_LEX_INVALID_NUM     = 1003,  /* 非法数字格式 */

    /* 语法/解析错误 (Syntax/Parsing Errors) */
    ERR_PARSE_EXPECTED_OP   = 2001,  /* 缺少操作数 */
    ERR_PARSE_INVALID_REG   = 2002,  /* 非法寄存器名 */
    ERR_PARSE_UNK_MNEMONIC  = 2003,  /* 未知指令助记符 */
    ERR_PARSE_DUP_LABEL     = 2004,  /* 标签重复定义 */
    ERR_PARSE_UNDEFINED_LBL = 2005,  /* 符号未定义 (通常在 Pass 2 报错) */

    /* 系统/资源错误 (System Errors) */
    ERR_SYS_OUT_OF_MEM      = 3001,  /* 内存溢出 */
    ERR_SYS_FILE_IO         = 3002   /* 文件读取/写入失败 */
} ErrorCode;

/* --------------------------------------------------------------------------
 * 2. 公共接口函数
 * -------------------------------------------------------------------------- */

/*
 * 函数: error_init
 * 描述: 初始化错误处理模块，重置错误计数。
 */
void error_init(void);

/*
 * 函数: error_report
 * 描述: 向标准错误流报告一个错误。
 * 参数: line_num - 发生错误的源代码行号
 * code     - 错误码 (ErrorCode 枚举)
 * detail   - 额外的详细描述信息 (可选，可为 NULL_PTR)
 */
void error_report(u32 line_num, ErrorCode code, const char* detail);

/*
 * 函数: error_get_count
 * 描述: 获取当前已报告的错误总数。
 * 返回: 错误数量
 */
u32 error_get_count(void);

/*
 * 函数: error_has_failed
 * 描述: 检查编译是否失败（错误数 > 0）。
 * 返回: TRUE 表示失败，FALSE 表示暂无错误
 */
bool_t error_has_failed(void);


#endif /* __ERROR_H__ */


