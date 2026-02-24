/**
 * lexer.h - 词法分析模块头文件
 *
 * 本模块提供一个用于汇编器前端的简易词法分析器（Lexer）。
 * 设计目标：
 *  - 遵循 C90 标准（尽量少用标准库）
 *  - 提供清晰、可复用的 API 供后续解析器（parser）调用
 *  - 使用项目工具库（`utils`）和统一错误报告（`error`）
 *
 * 词法器将输入源拆分为若干 Token，Token 中包含词素字符串、副本所有权、
 * 行号以及（当适用）数字值信息。词法器不会识别汇编级别的助记符/伪指令
 * 语义，这些应在语法分析阶段识别。
 *
 */
#ifndef __LEXER_H__
#define __LEXER_H__

#include "utils.h"

/* 词法单元类型（Token Types） */
typedef enum {
    TOK_EOF = 0,
    TOK_NEWLINE,
    TOK_IDENTIFIER, /* 标签、助记符、伪指令名、符号等 */
    TOK_NUMBER,     /* 整数或十六进制数 */
    TOK_STRING,     /* 字符串文字 */
    TOK_COMMA,      /* , */
    TOK_COLON,      /* : */
    TOK_LBRACKET,   /* [ */
    TOK_RBRACKET,   /* ] */
    TOK_LPAREN,     /* ( */
    TOK_RPAREN,     /* ) */
    TOK_PLUS,       /* + */
    TOK_MINUS,      /* - */
    TOK_ASTERISK,   /* * */
    TOK_SLASH,      /* / */
    TOK_OTHER       /* 未识别但作为单字符返回 */
} TokenType;

/* 词法单元结构体：携带字符串副本以便上层解析器使用并在处理完毕后释放 */
typedef struct {
    TokenType type;
    char* lexeme;   /* 指向已分配的以\0结尾的字符串（由 token_dispose 释放） */
    u32 line;       /* 源文件行号（1 起始） */
    s32 int_value;  /* 若为数字，可填充其整数值（十进制/十六进制） */
} Token;

/* 词法器状态结构体 */
typedef struct Lexer {
    char* buffer;   /* 源文本副本（以便安全访问） */
    u32 pos;        /* 当前读取位置（0 起始） */
    u32 len;        /* buffer 的长度（不含隐式终止符） */
    u32 line;       /* 当前行号（1 起始） */
} Lexer;

/* API 函数 */

/*
 * 创建词法器：从给定以 \0 结尾的源字符串创建 Lexer 对象并复制源文本。
 * 返回已分配的 Lexer*，失败返回 NULL_PTR。
 */
Lexer* lexer_create_from_string(const char* src);

/* 释放词法器以及内部缓冲区 */
void lexer_destroy(Lexer* lx);

/*
 * 获取下一个 Token（按需分配 lexeme）的副本。
 * 使用者在不再需要时必须调用 `token_dispose` 释放 token.lexeme。
 */
Token lexer_next_token(Lexer* lx);

/* 释放单个 Token 占用的内存（主要是 lexeme） */
void token_dispose(Token* tok);

#endif /* __LEXER_H__ */


