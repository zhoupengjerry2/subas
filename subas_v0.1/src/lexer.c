/*
 * ============================================================================
 * 文件名称： lexer.c
 * 描    述： 词法分析器实现（Lexer）
 *
 * 说    明：
 *  - 实现一个简单且可用于后续语法分析的词法器。
 *  - 支持注释以分号 (';') 开始至行末。
 *  - 支持单引号 '\'' 括起来的字符和字符串文字、十进制数。
 *  - 遇到词法错误通过返回非 0 值。
 *
 * 注释规范：本文件注释均为中文，函数前使用块注释描述接口契约，内部实现处保留简洁的行注释以
 * 解释关键步骤。
 * ============================================================================
 */

#include "../include/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* 内部辅助函数声明 */
static int is_alpha(char c);
static int is_digit(char c);
static int is_alnum(char c);
static char peek_char(Lexer* lx);
static char advance_char(Lexer* lx);
static void skip_whitespace_and_comments(Lexer* lx);


/* 创建词法分析器  */
Lexer* lexer_create(const char* src) {
    Lexer* lx;
    
    /* 判断入口参数合法性    */
    if (src == NULL) {
        return NULL;
    }

    /* 为词法分析器分配内存   */
    lx = (Lexer*)malloc(sizeof(Lexer));
    if (lx == NULL) {
        printf("\nAllocation failed! %s:%s\n", __FILE__, __LINE__);
        return NULL;
    }

    /* 为词法分析器内部缓冲区分配内存  */
    lx->buf_len = strlen(src);
    lx->buffer = (char*)malloc(lx->buf_len + 1);
    if (lx->buffer == NULL) {
        printf("\nAllocation failed! %s:%s\n", __FILE__, __LINE__);
        free(lx);
        return NULL;
    }
    
    /* 复制源文本并确保以 '\0' 结尾 */
    unsigned    i;
    for (i = 0; i < lx->buf_len; i++) {
        lx->buffer[i] = src[i];
    }
    lx->buffer[lx->buf_len] = '\0';

    /* 设置内部指针   */
    lx->pos = 0;
    lx->line = 1;
    
    return lx;
}

/* 销毁词法器，释放内部缓冲区 */
void lexer_destroy(Lexer* lx) {
    /* 判断入口参数合法性    */
    if (lx == NULL) {
        return;
    }
    
    /* 释放内部缓冲区  */
    if (lx->buffer != NULL) {
        free(lx->buffer);
    }
    
    /* 释放词法分析器  */
    free(lx);
}

/* 解析下一个标识符/关键字 */
static Token lex_identifier(Lexer* lx) {
    unsigned start = lx->pos;
    unsigned len = 0;
    int ch = 0;
    Token tok = {TT_DUMMY, "", 0, 0};
    
    /* 判断首个字符是否为字母、数字   */
    ch = peek_char(lx);
    while (is_alnum(ch)) {
        advance_char(lx);
        ch = peek_char(lx);
    }

    len = lx->pos - start;
    char* buf = (char*)malloc(len + 1);
    if (buf == NULL) {
        error_report(lx->line, ERR_SYS_OUT_OF_MEM, NULL_PTR);
        Token t = { TOK_EOF, NULL_PTR, lx->line, 0 };
        return t;
    }
    {
        u32 i;
        for (i = 0; i < len; i++) buf[i] = lx->buffer[start + i];
        buf[len] = '\0';
    }

    Token t;
    t.type = TOK_IDENTIFIER;
    t.lexeme = buf;
    t.line = lx->line;
    t.int_value = 0;
    return t;
}

/* 解析数字：支持 C 风格 0xFF、MASM 风格 0Dh 以及十进制 */
/* 注： 识别顺序：
 *   1. 0x/0X 前缀：C 风格十六进制（0xFF）
 *   2. 数字 + h/H：MASM 风格十六进制（10h, 0FAh）
 *   3. 纯数字：十进制
 */
static Token lex_number(Lexer* lx) {
    u32 start = lx->pos;
    int is_c_hex = 0;

    /* 检查 0x/0X 前缀 */
    if (peek_char(lx) == '0') {
        if (lx->pos + 1 < lx->len) {
            char c1 = lx->buffer[lx->pos + 1];
            if (c1 == 'x' || c1 == 'X') {
                is_c_hex = 1;
            }
        }
    }

    if (is_c_hex) {
        /* C 风格：0xFF */
        advance_char(lx);
        advance_char(lx);
        u32 val = 0;
        u32 cnt = 0;
        while (1) {
            char c = peek_char(lx);
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                advance_char(lx);
                cnt++;
                u32 digit;
                if (c >= '0' && c <= '9') digit = (u32)(c - '0');
                else if (c >= 'a' && c <= 'f') digit = (u32)(c - 'a' + 10);
                else digit = (u32)(c - 'A' + 10);
                val = (val << 4) | digit;
            } else break;
        }
        if (cnt == 0) {
            error_report(lx->line, ERR_LEX_INVALID_NUM, "invalid hex literal");
        }

        u32 len = lx->pos - start;
        char* buf = (char*)util_malloc(len + 1);
        if (buf == NULL_PTR) {
            error_report(lx->line, ERR_SYS_OUT_OF_MEM, NULL_PTR);
            Token t = { TOK_EOF, NULL_PTR, lx->line, 0 };
            return t;
        }
        {
            u32 i;
            for (i = 0; i < len; i++) buf[i] = lx->buffer[start + i];
            buf[len] = '\0';
        }
        Token t = { TOK_NUMBER, buf, lx->line, (s32)val };
        return t;
    }

    /* MASM 或十进制 */
    u32 cnt = 0;
    u32 num_end = lx->pos;

    /* 读取数字和潜在的 A-F 字符 */
    while (1) {
        char c = peek_char(lx);
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            advance_char(lx);
            cnt++;
            num_end = lx->pos;
        } else {
            break;
        }
    }

    if (cnt == 0) {
        error_report(lx->line, ERR_LEX_INVALID_NUM, "invalid decimal literal");
        Token t = { TOK_EOF, NULL_PTR, lx->line, 0 };
        return t;
    }

    /* 检查 MASM hex：数字 + h/H */
    if (peek_char(lx) == 'h' || peek_char(lx) == 'H') {
        advance_char(lx);
        /* 作为十六进制解析 */
        u32 val = 0;
        for (u32 i = start; i < num_end; i++) {
            char c = lx->buffer[i];
            u32 digit = 0;
            if (c >= '0' && c <= '9') digit = (u32)(c - '0');
            else if (c >= 'a' && c <= 'f') digit = (u32)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') digit = (u32)(c - 'A' + 10);
            val = (val << 4) | digit;
        }

        u32 len = lx->pos - start;
        char* buf = (char*)util_malloc(len + 1);
        if (buf == NULL_PTR) {
            error_report(lx->line, ERR_SYS_OUT_OF_MEM, NULL_PTR);
            Token t = { TOK_EOF, NULL_PTR, lx->line, 0 };
            return t;
        }
        {
            u32 i;
            for (i = 0; i < len; i++) buf[i] = lx->buffer[start + i];
            buf[len] = '\0';
        }
        Token t = { TOK_NUMBER, buf, lx->line, (s32)val };
        return t;
    }

    /* 纯十进制 */
    u32 val = 0;
    for (u32 i = start; i < lx->pos; i++) {
        char c = lx->buffer[i];
        if (c >= '0' && c <= '9') {
            val = val * 10 + (u32)(c - '0');
        }
    }

    u32 len = lx->pos - start;
    char* buf = (char*)util_malloc(len + 1);
    if (buf == NULL_PTR) {
        error_report(lx->line, ERR_SYS_OUT_OF_MEM, NULL_PTR);
        Token t = { TOK_EOF, NULL_PTR, lx->line, 0 };
        return t;
    }
    {
        u32 i;
        for (i = 0; i < len; i++) buf[i] = lx->buffer[start + i];
        buf[len] = '\0';
    }
    Token t = { TOK_NUMBER, buf, lx->line, (s32)val };
    return t;
}

/* 解析字符串文字；支持单引号或双引号，遇到 EOF 报错 */
static Token lex_string(Lexer* lx) {
    char quote = advance_char(lx); /* 吃掉起始引号 */
    u32 start = lx->pos;

    while (peek_char(lx) != '\0' && peek_char(lx) != quote) {
        if (peek_char(lx) == '\n') {
            /* 字符串跨行：更新行计数并继续 */
            lx->line++;
        }
        advance_char(lx);
    }

    u32 len = lx->pos - start; /* 计算不含结束引号的长度 */
    
    if (peek_char(lx) != quote) {
        /* 未闭合字符串 */
        error_report(lx->line, ERR_LEX_UNCLOSED_STR, NULL_PTR);
    } else {
        advance_char(lx); /* 吃掉结束引号 */
    }

    /* 复制字符串内容，不包含引号 */
    char* buf = (char*)util_malloc(len + 1);
    if (buf == NULL_PTR) {
        error_report(lx->line, ERR_SYS_OUT_OF_MEM, NULL_PTR);
        Token t = { TOK_EOF, NULL_PTR, lx->line, 0 };
        return t;
    }
    {
        u32 i;
        for (i = 0; i < len; i++) buf[i] = lx->buffer[start + i];
        buf[len] = '\0';
    }

    Token t = { TOK_STRING, buf, lx->line, 0 };
    return t;
}

/* 主接口：返回下一个 token */
Token lexer_next_token(Lexer* lx) {
    Token tok;
    tok.lexeme = NULL_PTR;
    tok.type = TOK_EOF;
    tok.line = lx->line;
    tok.int_value = 0;

    if (lx == NULL_PTR) return tok;

    for (;;) {
        if (lx->pos >= lx->len) {
            tok.type = TOK_EOF;
            tok.lexeme = NULL_PTR;
            tok.line = lx->line;
            return tok;
        }

        /* 先处理空白和注释（不会吞掉换行） */
        skip_whitespace_and_comments(lx);

        if (lx->pos >= lx->len) {
            tok.type = TOK_EOF;
            tok.lexeme = NULL_PTR;
            tok.line = lx->line;
            return tok;
        }

        char c = peek_char(lx);

        /* 处理换行：作为单独 token 返回以便上层语法器按行组织 */
        if (c == '\n') {
            advance_char(lx);
            lx->line++;
            tok.type = TOK_NEWLINE;
            tok.lexeme = util_strdup("\n");
            tok.line = lx->line - 1; /* 返回发生换行前的行号 */
            return tok;
        }

        /* 单字符符号 */
        if (c == ',') { advance_char(lx); tok.type = TOK_COMMA; tok.lexeme = util_strdup(","); tok.line = lx->line; return tok; }
        if (c == ':') { advance_char(lx); tok.type = TOK_COLON; tok.lexeme = util_strdup(":" ); tok.line = lx->line; return tok; }
        if (c == '[') { advance_char(lx); tok.type = TOK_LBRACKET; tok.lexeme = util_strdup("["); tok.line = lx->line; return tok; }
        if (c == ']') { advance_char(lx); tok.type = TOK_RBRACKET; tok.lexeme = util_strdup("]"); tok.line = lx->line; return tok; }
        if (c == '(') { advance_char(lx); tok.type = TOK_LPAREN; tok.lexeme = util_strdup("("); tok.line = lx->line; return tok; }
        if (c == ')') { advance_char(lx); tok.type = TOK_RPAREN; tok.lexeme = util_strdup(")"); tok.line = lx->line; return tok; }
        if (c == '+') { advance_char(lx); tok.type = TOK_PLUS; tok.lexeme = util_strdup("+"); tok.line = lx->line; return tok; }
        if (c == '-') { advance_char(lx); tok.type = TOK_MINUS; tok.lexeme = util_strdup("-"); tok.line = lx->line; return tok; }
        if (c == '*') { advance_char(lx); tok.type = TOK_ASTERISK; tok.lexeme = util_strdup("*"); tok.line = lx->line; return tok; }
        if (c == '/') { advance_char(lx); tok.type = TOK_SLASH; tok.lexeme = util_strdup("/"); tok.line = lx->line; return tok; }

        /* 字符串 */
        if (c == '"' || c == '\'') {
            return lex_string(lx);
        }

        /* 数字 */
        if (is_digit(c)) {
            return lex_number(lx);
        }

        /* 标识符（字母开头） */
        if (is_alpha(c)) {
            return lex_identifier(lx);
        }

        /* 未识别字符：报告错误并跳过该字符 */
        {
            char badch[2];
            badch[0] = c; badch[1] = '\0';
            error_report(lx->line, ERR_LEX_INVALID_CHAR, badch);
            advance_char(lx);
            /* 跳过后继续获取下一个 token */
            continue;
        }
    }
}


/* 判断字母（仅 ASCII） */
static int is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') ||
    (c >= 'a' && c <= 'z') ||
    c == '_' || c == '.' || c == '$' || c == '@';
}

/* 判断数字 */
static int is_digit(char c) {
    return (c >= '0' && c <= '9');
}

/* 判断字母或数字或下划线等 */
static int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

/* 返回当前字符但不前进 */
static char peek_char(Lexer* lx) {
    if (lx->pos >= lx->buf_len) {
        return '\0';
    }
    return lx->buffer[lx->pos];
}

/* 返回当前字符并前进位置 */
static char advance_char(Lexer* lx) {
    char ch = peek_char(lx);
    
    if (ch == '\0') {
        return '\0';
    }
    lx->pos++;
    return ch;
}

/* 跳过空白与注释；注释以 ';' 开始至行尾 */
static void skip_whitespace_and_comments(Lexer* lx) {
    char ch;
    
    for (;;) {
        ch = peek_char(lx);
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            advance_char(lx);
            continue;
        }
        if (ch == ';') {
            /* 注释：跳到行尾或文件结束 */
            while (peek_char(lx) != '\0' && 
                peek_char(lx) != '\n' &&
                peek_char(lx) != '\r') {
                advance_char(lx);
            }
            continue; /* 继续外层循环以处理可能的空白 */
        }
        break;
    }
}
