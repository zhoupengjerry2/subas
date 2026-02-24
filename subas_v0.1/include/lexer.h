/*
 * ============================================================================
 * 文件名称： lexer.h
 * 描    述： 词法分析模块头文件
 *
 * 本模块提供一个用于汇编器前端的简易词法分析器（Lexer）。
 * 设计目标：
 *  - 遵循 C90 标准（尽量少用标准库）
 *  - 提供清晰、可复用的 API 供后续解析器（parser）调用
 *  - 使用项目工具库（`utils`）和统一错误报告（`error`）
 *  - 遇到词法错误通过返回非 0 值。
 *
 * 词法器将输入源拆分为若干 Token，Token 中包含 Token 类型、词素字符串、行号，对于数字
 * 则包含数字值信息。
 * 词法器不会识别汇编级别的助记符/伪指令语义，这些应在语法分析阶段识别。
 * ============================================================================
 */
#ifndef __LEXER_H__
#define __LEXER_H__

/* 常量宏定义    */
#define MAX_LEXEME  128

/* 词法单元类型（Token Types） */
typedef enum {
    TT_EOF = -1,
    TT_DUMMY,       /* 占位符，无意义  */
    TT_IDENTIFIER,  /* 标签、助记符、伪指令名、符号等 */
    TT_NUMBER,      /* 整数或十六进制数 */
    TT_STRING,      /* 字符串文字 */
    TT_COMMA,       /* , */
    TT_COLON,       /* : */
    TT_LBRACKET,    /* [ */
    TT_RBRACKET,    /* ] */
    TT_LPAREN,      /* ( */
    TT_RPAREN,      /* ) */
    TT_PLUS,        /* + */
    TT_MINUS,       /* - */
    TT_ASTERISK,    /* * */
    TT_SLASH,       /* / */
    TT_OTHER        /* 未识别但作为单字符返回 */
} TokenType;

/* 词法单元结构体  */
typedef struct {
    TokenType   type;
    char        lexeme[MAX_LEXEME];     /* 以 `\0` 结尾的字符串               */
    unsigned    line;                   /* 源文件行号（1 起始）              */
    int         value;                  /* 若为数字，则为其整数值（十六进制）    */
} Token;

/* 词法分析器状态结构体   */
typedef struct Lexer {
    char*       buffer;     /* 缓冲区（存放源文本副本，以便安全访问）  */
    unsigned    buf_len;    /* buffer 的长度（不含隐式终止符）      */
    unsigned    pos;        /* 当前读取位置（0 起始）             */
    unsigned    line;       /* 当前行号（1 起始）                  */
} Lexer;

/* API 函数 */

/*
 * 创建词法分析器：从给定以 \0 结尾的源字符串创建 Lexer 对象并复制源文本。
 * 返回已分配的 Lexer*，失败返回 NULL。
 */
Lexer* lexer_create(const char* src);

/* 释放词法分析器以及内部缓冲区   */
void lexer_destroy(Lexer* lx);

/*
 * 获取下一个 Token 的副本。
 */
Token lexer_next_token(Lexer* lx);


#endif /* __LEXER_H__ */


