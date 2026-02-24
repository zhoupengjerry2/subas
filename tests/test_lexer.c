/*
 * ============================================================================
 * 文件名: test_lexer.c
 * 描述  : 词法分析器单元测试驱动程序
 *
 * 测试覆盖范围：
 *  - 基础 token 识别（标识符、数字、字符串、符号）
 *  - 注释处理
 *  - 十进制与十六进制数字
 *  - 错误报告
 *  - 行号跟踪
 *
 * 编译命令示例（在项目根目录）：
 *   gcc -o test_lexer test_lexer.c src/lexer.c src/error.c src/utils/memory.c \
 *       src/utils/string.c src/utils/hash.c -I.
 *
 * ============================================================================
 */

#include <stdio.h>
#include "../include/lexer.h"
#include "../include/error.h"

/* 辅助宏：便于打印 token 类型名称 */
static const char* token_type_name(TokenType type) {
    switch (type) {
        case TOK_EOF: return "EOF";
        case TOK_NEWLINE: return "NEWLINE";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_COMMA: return "COMMA";
        case TOK_COLON: return "COLON";
        case TOK_LBRACKET: return "LBRACKET";
        case TOK_RBRACKET: return "RBRACKET";
        case TOK_LPAREN: return "LPAREN";
        case TOK_RPAREN: return "RPAREN";
        case TOK_PLUS: return "PLUS";
        case TOK_MINUS: return "MINUS";
        case TOK_ASTERISK: return "ASTERISK";
        case TOK_SLASH: return "SLASH";
        case TOK_OTHER: return "OTHER";
        default: return "UNKNOWN";
    }
}

/* 测试用例结构体 */
typedef struct {
    const char* name;
    const char* input;
    int expected_tokens;
} TestCase;

/* 测试 1：基础标识符与符号 */
static void test_basic_tokens(void) {
    const char* src = "MOV AX, BX";
    Lexer* lx;
    Token tok;
    int count = 0;

    printf("=== Test 1: Basic Tokens (MOV AX, BX) ===\n");
    error_init();
    lx = lexer_create_from_string(src);
    if (lx == NULL_PTR) {
        printf("FAIL: lexer_create_from_string returned NULL\n");
        return;
    }

    while (1) {
        tok = lexer_next_token(lx);
        if (tok.type == TOK_EOF) break;
        if (tok.type == TOK_NEWLINE) continue; /* 跳过换行 */

        printf("Token %d: type=%s, lexeme='%s', line=%u",
               count++, token_type_name(tok.type), tok.lexeme, tok.line);
        if (tok.type == TOK_NUMBER) printf(", int_value=%d", tok.int_value);
        printf("\n");
        token_dispose(&tok);
    }

    printf("Expected 3 tokens (MOV, AX, BX), got %d tokens\n", count);
    printf("Error count: %u\n\n", error_get_count());
    lexer_destroy(lx);
}

/* 测试 2：十进制与十六进制数字 */
static void test_numbers(void) {
    const char* src = "DB 100, 0xFF, 0x00AB, 0";
    Lexer* lx;
    Token tok;
    int count = 0;

    printf("=== Test 2: Numbers (Decimal and Hex) ===\n");
    error_init();
    lx = lexer_create_from_string(src);
    if (lx == NULL_PTR) {
        printf("FAIL: lexer_create_from_string returned NULL\n");
        return;
    }

    while (1) {
        tok = lexer_next_token(lx);
        if (tok.type == TOK_EOF) break;
        if (tok.type == TOK_NEWLINE) continue;

        if (tok.type == TOK_NUMBER || tok.type == TOK_IDENTIFIER) {
            printf("Token: type=%s, lexeme='%s', int_value=%d, line=%u\n",
                   token_type_name(tok.type), tok.lexeme, tok.int_value, tok.line);
            count++;
        }
        token_dispose(&tok);
    }

    printf("Expected 5 tokens (DB + 4 numbers), got %d\n", count + 1);
    printf("Error count: %u\n\n", error_get_count());
    lexer_destroy(lx);
}

/* 测试 3：字符串 */
static void test_strings(void) {
    const char* src = "DB \"Hello World\", 'test'";
    Lexer* lx;
    Token tok;
    int count = 0;

    printf("=== Test 3: String Literals ===\n");
    error_init();
    lx = lexer_create_from_string(src);
    if (lx == NULL_PTR) {
        printf("FAIL: lexer_create_from_string returned NULL\n");
        return;
    }

    while (1) {
        tok = lexer_next_token(lx);
        if (tok.type == TOK_EOF) break;
        if (tok.type == TOK_NEWLINE) continue;

        printf("Token: type=%s, lexeme='%s', line=%u\n",
               token_type_name(tok.type), tok.lexeme, tok.line);
        count++;
        token_dispose(&tok);
    }

    printf("Expected 5 tokens (DB + string + comma + string), got %d\n", count);
    printf("Error count: %u\n\n", error_get_count());
    lexer_destroy(lx);
}

/* 测试 4：注释处理 */
static void test_comments(void) {
    const char* src = "MOV AX, 1 ; This is a comment\nADD BX, 2 ; Another";
    Lexer* lx;
    Token tok;
    int count = 0;

    printf("=== Test 4: Comments (';' to end of line) ===\n");
    error_init();
    lx = lexer_create_from_string(src);
    if (lx == NULL_PTR) {
        printf("FAIL: lexer_create_from_string returned NULL\n");
        return;
    }

    while (1) {
        tok = lexer_next_token(lx);
        if (tok.type == TOK_EOF) break;

        printf("Token: type=%s, lexeme='%s', line=%u",
               token_type_name(tok.type), tok.lexeme, tok.line);
        if (tok.type == TOK_NUMBER) printf(", int_value=%d", tok.int_value);
        printf("\n");
        count++;
        token_dispose(&tok);
    }

    printf("Token count (including comments): %d\n", count);
    printf("Error count: %u\n\n", error_get_count());
    lexer_destroy(lx);
}

/* 测试 5：特殊符号 */
static void test_special_chars(void) {
    const char* src = "LABEL: [BP+2] (AX) * / - +";
    Lexer* lx;
    Token tok;
    int count = 0;

    printf("=== Test 5: Special Characters ===\n");
    error_init();
    lx = lexer_create_from_string(src);
    if (lx == NULL_PTR) {
        printf("FAIL: lexer_create_from_string returned NULL\n");
        return;
    }

    while (1) {
        tok = lexer_next_token(lx);
        if (tok.type == TOK_EOF) break;
        if (tok.type == TOK_NEWLINE) continue;

        printf("Token: type=%s, lexeme='%s'\n", token_type_name(tok.type), tok.lexeme);
        count++;
        token_dispose(&tok);
    }

    printf("Token count: %d\n", count);
    printf("Error count: %u\n\n", error_get_count());
    lexer_destroy(lx);
}

/* 测试 6：行号跟踪 */
static void test_line_tracking(void) {
    const char* src = "Line1\nLine2\n\nLine4";
    Lexer* lx;
    Token tok;

    printf("=== Test 6: Line Number Tracking ===\n");
    error_init();
    lx = lexer_create_from_string(src);
    if (lx == NULL_PTR) {
        printf("FAIL: lexer_create_from_string returned NULL\n");
        return;
    }

    while (1) {
        tok = lexer_next_token(lx);
        if (tok.type == TOK_EOF) {
            printf("EOF at line %u\n", tok.line);
            break;
        }

        printf("Line %u: type=%s, lexeme='%s'\n", tok.line, token_type_name(tok.type), tok.lexeme);
        token_dispose(&tok);
    }

    printf("Error count: %u\n\n", error_get_count());
    lexer_destroy(lx);
}

/* 测试 7：错误恢复 */
static void test_error_handling(void) {
    const char* src = "MOV @# BX \"unclosed";
    Lexer* lx;
    Token tok;

    printf("=== Test 7: Error Handling (Invalid Chars & Unclosed String) ===\n");
    error_init();
    lx = lexer_create_from_string(src);
    if (lx == NULL_PTR) {
        printf("FAIL: lexer_create_from_string returned NULL\n");
        return;
    }

    while (1) {
        tok = lexer_next_token(lx);
        if (tok.type == TOK_EOF) break;
        if (tok.type == TOK_NEWLINE) continue;

        printf("Token: type=%s, lexeme='%s', line=%u\n",
               token_type_name(tok.type), tok.lexeme, tok.line);
        token_dispose(&tok);
    }

    printf("Total errors reported: %u (expected 2: invalid chars + unclosed string)\n", error_get_count());
    printf("Error count: %u\n\n", error_get_count());
    lexer_destroy(lx);
}

/* 测试 8：汇编伪指令链 */
static void test_masm_pseudo(void) {
    const char* src = "SEGMENT PROC ENDP ASSUME ORG END";
    Lexer* lx;
    Token tok;
    int count = 0;

    printf("=== Test 8: MASM Pseudo-Instructions ===\n");
    error_init();
    lx = lexer_create_from_string(src);
    if (lx == NULL_PTR) {
        printf("FAIL: lexer_create_from_string returned NULL\n");
        return;
    }

    while (1) {
        tok = lexer_next_token(lx);
        if (tok.type == TOK_EOF) break;
        if (tok.type == TOK_NEWLINE) continue;

        printf("Token: type=%s, lexeme='%s'\n", token_type_name(tok.type), tok.lexeme);
        count++;
        token_dispose(&tok);
    }

    printf("Token count: %d (expected 6 keywords)\n", count);
    printf("Error count: %u\n\n", error_get_count());
    lexer_destroy(lx);
}

/* 测试 9：MASM 风格十六进制数字 */
static void test_masm_hex_numbers(void) {
    const char* src = "DB 0Dh, 0FFh, 0x00ABh, 0x123, 10h, 1Ah, 0FAh, 0FAH, 0x0AB, 255";
    Lexer* lx;
    Token tok;
    int count = 0;

    printf("=== Test 9: MASM Hex Format (0Dh, 0FaH) ===\n");
    error_init();
    lx = lexer_create_from_string(src);
    if (lx == NULL_PTR) {
        printf("FAIL: lexer_create_from_string returned NULL\n");
        return;
    }

    while (1) {
        tok = lexer_next_token(lx);
        if (tok.type == TOK_EOF) break;
        if (tok.type == TOK_NEWLINE) continue;

        if (tok.type == TOK_NUMBER) {
            printf("Token: type=%s, lexeme='%s', int_value=%d (hex=0x%x)\n",
                   token_type_name(tok.type), tok.lexeme, tok.int_value, (u32)tok.int_value);
        } else if (tok.type == TOK_IDENTIFIER) {
            printf("Token: type=%s, lexeme='%s'\n", token_type_name(tok.type), tok.lexeme);
        }
        count++;
        token_dispose(&tok);
    }

    printf("Total tokens: %d\n", count);
    printf("Error count: %u (should be 0 for valid numbers)\n\n", error_get_count());
    lexer_destroy(lx);
}

/* 主测试入口 */
int main(void) {
    printf("========================================\n");
    printf("   LEXER MODULE UNIT TESTS\n");
    printf("========================================\n\n");

    test_basic_tokens();
    test_numbers();
    test_strings();
    test_comments();
    test_special_chars();
    test_line_tracking();
    test_error_handling();
    test_masm_pseudo();
    test_masm_hex_numbers();

    printf("========================================\n");
    printf("   ALL TESTS COMPLETED\n");
    printf("========================================\n");

    return 0;
}


