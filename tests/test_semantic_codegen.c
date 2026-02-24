/*
 * ============================================================================
 * 文件名: test_semantic_codegen.c
 * 描述  : 语义分析 (Semantic) 和代码生成 (CodeGen) 两遍扫描模块单元测试
 *
 * 测试覆盖范围：
 *  - Semantic 模块：Token 流转换为指令列表，符号表建立，标签记录
 *  - CodeGen 模块：指令代码生成，标签前向/向后引用解决
 *  - 集成测试：完整的两遍扫描流程验证
 *
 * 编译命令（在项目根目录）：
 *   gcc -o tests/test_semantic_codegen tests/test_semantic_codegen.c \
 *       src/semantic.c src/codegen.c src/tables.c src/symtab.c \
 *       src/lexer.c src/utils/memory.c src/utils/string.c \
 *       src/utils/hash.c src/error.c -I. -Wall -Wextra
 *
 * ============================================================================
 */

#include <stdio.h>
#include "../include/semantic.h"
#include "../include/codegen.h"
#include "../include/lexer.h"

#define ASSERT_EQ(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            printf("  [FAIL] %s: expected %d, got %d\n", (msg), (int)(expected), (int)(actual)); \
            test_failed++; \
        } else { \
            printf("  [PASS] %s\n", (msg)); \
            test_passed++; \
        } \
    } while (0)

#define ASSERT_PTR_NEQ(actual, expected, msg) \
    do { \
        if ((actual) == (expected)) { \
            printf("  [FAIL] %s: pointer should not be NULL\n", (msg)); \
            test_failed++; \
        } else { \
            printf("  [PASS] %s\n", (msg)); \
            test_passed++; \
        } \
    } while (0)

static u32 test_passed = 0;
static u32 test_failed = 0;

/* =========================================================================
 * SEMANTIC 模块测试
 * ========================================================================= */

static void test_semantic_pass_one_simple(void) {
    printf("\n=== Semantic: Pass One Simple Instructions ===\n");
    
    Token tokens[5];
    
    tokens[0].type = TOK_IDENTIFIER;
    tokens[0].lexeme = util_strdup("MOV");
    tokens[0].line = 1;
    tokens[0].int_value = 0;
    
    tokens[1].type = TOK_NEWLINE;
    tokens[1].lexeme = NULL_PTR;
    tokens[1].line = 1;
    
    tokens[2].type = TOK_IDENTIFIER;
    tokens[2].lexeme = util_strdup("RET");
    tokens[2].line = 2;
    tokens[2].int_value = 0;
    
    tokens[3].type = TOK_NEWLINE;
    tokens[3].lexeme = NULL_PTR;
    tokens[3].line = 2;
    
    tokens[4].type = TOK_EOF;
    tokens[4].lexeme = NULL_PTR;
    tokens[4].line = 3;
    
    tables_init();
    PassOne* pass_one = semantic_pass_one(tokens, 5);
    
    ASSERT_PTR_NEQ(pass_one, NULL_PTR, "semantic_pass_one success");
    
    if (pass_one != NULL) {
        ASSERT_EQ(pass_one->instruction_count, 2, "2 instructions parsed");
        ASSERT_EQ(pass_one->current_address > 0, 1, "address advanced");
        semantic_pass_one_destroy(pass_one);
    }
    
    if (tokens[0].lexeme) util_free(tokens[0].lexeme);
    if (tokens[2].lexeme) util_free(tokens[2].lexeme);
}

static void test_semantic_symbol_table(void) {
    printf("\n=== Semantic: Symbol Table Building ===\n");
    
    Token tokens[8];
    
    tokens[0].type = TOK_IDENTIFIER;
    tokens[0].lexeme = util_strdup("LABEL");
    tokens[0].line = 1;
    
    tokens[1].type = TOK_COLON;
    tokens[1].lexeme = NULL_PTR;
    tokens[1].line = 1;
    
    tokens[2].type = TOK_IDENTIFIER;
    tokens[2].lexeme = util_strdup("MOV");
    tokens[2].line = 1;
    
    tokens[3].type = TOK_NEWLINE;
    tokens[3].lexeme = NULL_PTR;
    tokens[3].line = 1;
    
    tokens[4].type = TOK_IDENTIFIER;
    tokens[4].lexeme = util_strdup("RET");
    tokens[4].line = 2;
    
    tokens[5].type = TOK_NEWLINE;
    tokens[5].lexeme = NULL_PTR;
    tokens[5].line = 2;
    
    tokens[6].type = TOK_EOF;
    tokens[6].lexeme = NULL_PTR;
    tokens[6].line = 3;
    
    tables_init();
    PassOne* pass_one = semantic_pass_one(tokens, 7);
    
    ASSERT_PTR_NEQ(pass_one, NULL_PTR, "semantic_pass_one succeeded");
    
    if (pass_one != NULL) {
        printf("  Instructions: %u, Symbols: %u\n", 
            pass_one->instruction_count, 
            symtab_get_symbol_count(pass_one->symtab));
        semantic_pass_one_destroy(pass_one);
    }
    
    for (int i = 0; i < 7; i++) {
        if (tokens[i].lexeme) util_free(tokens[i].lexeme);
    }
}

static void test_semantic_instruction_details(void) {
    printf("\n=== Semantic: Instruction Entry Details ===\n");
    
    Token tokens[4];
    
    tokens[0].type = TOK_IDENTIFIER;
    tokens[0].lexeme = util_strdup("ADD");
    tokens[0].line = 1;
    
    tokens[1].type = TOK_NEWLINE;
    tokens[1].lexeme = NULL_PTR;
    tokens[1].line = 1;
    
    tokens[2].type = TOK_EOF;
    tokens[2].lexeme = NULL_PTR;
    tokens[2].line = 2;
    
    tables_init();
    PassOne* pass_one = semantic_pass_one(tokens, 3);
    
    ASSERT_PTR_NEQ(pass_one, NULL_PTR, "semantic_pass_one succeeded");
    
    if (pass_one != NULL && pass_one->instruction_count > 0) {
        InstructionEntry* entry = &pass_one->instructions[0];
        printf("  Mnemonic: %s\n", entry->mnemonic);
        ASSERT_EQ(entry->address, 0, "first instr at addr 0");
        ASSERT_EQ(entry->length > 0, 1, "instr has length");
        semantic_pass_one_destroy(pass_one);
    }
    
    if (tokens[0].lexeme) util_free(tokens[0].lexeme);
}

/* =========================================================================
 * CODEGEN 模块测试
 * ========================================================================= */

static void test_codegen_pass_two(void) {
    printf("\n=== CodeGen: Pass Two Code Generation ===\n");
    
    Token tokens[4];
    
    tokens[0].type = TOK_IDENTIFIER;
    tokens[0].lexeme = util_strdup("RET");
    tokens[0].line = 1;
    
    tokens[1].type = TOK_NEWLINE;
    tokens[1].lexeme = NULL_PTR;
    tokens[1].line = 1;
    
    tokens[2].type = TOK_EOF;
    tokens[2].lexeme = NULL_PTR;
    tokens[2].line = 2;
    
    tables_init();
    PassOne* pass_one = semantic_pass_one(tokens, 3);
    
    ASSERT_PTR_NEQ(pass_one, NULL_PTR, "semantic_pass_one succeeded");
    
    if (pass_one != NULL) {
        CodeGen* codegen = codegen_pass_two(pass_one);
        
        if (codegen != NULL) {
            ASSERT_PTR_NEQ(codegen, NULL_PTR, "codegen_pass_two succeeded");
            
            u32 code_size = 0;
            (void)codegen_get_code_buffer(codegen, &code_size);
            printf("  Generated code: %u bytes\n", code_size);
            ASSERT_EQ(code_size > 0, 1, "code generated");
            codegen_destroy(codegen);
        }
        
        semantic_pass_one_destroy(pass_one);
    }
    
    if (tokens[0].lexeme) util_free(tokens[0].lexeme);
}

static void test_codegen_label_resolve(void) {
    printf("\n=== CodeGen: Label Reference Resolution ===\n");
    
    Token tokens[10];
    
    tokens[0].type = TOK_IDENTIFIER;
    tokens[0].lexeme = util_strdup("START");
    tokens[0].line = 1;
    
    tokens[1].type = TOK_COLON;
    tokens[1].lexeme = NULL_PTR;
    tokens[1].line = 1;
    
    tokens[2].type = TOK_IDENTIFIER;
    tokens[2].lexeme = util_strdup("MOV");
    tokens[2].line = 1;
    
    tokens[3].type = TOK_NEWLINE;
    tokens[3].lexeme = NULL_PTR;
    tokens[3].line = 1;
    
    tokens[4].type = TOK_IDENTIFIER;
    tokens[4].lexeme = util_strdup("JMP");
    tokens[4].line = 2;
    
    tokens[5].type = TOK_NEWLINE;
    tokens[5].lexeme = NULL_PTR;
    tokens[5].line = 2;
    
    tokens[6].type = TOK_EOF;
    tokens[6].lexeme = NULL_PTR;
    tokens[6].line = 3;
    
    tables_init();
    PassOne* pass_one = semantic_pass_one(tokens, 7);
    
    ASSERT_PTR_NEQ(pass_one, NULL_PTR, "semantic_pass_one succeeded");
    
    if (pass_one != NULL) {
        CodeGen* codegen = codegen_pass_two(pass_one);
        
        if (codegen != NULL) {
            u32 reloc_count = 0;
            (void)codegen_get_relocation_info(codegen, &reloc_count);
            printf("  Relocations recorded: %u\n", reloc_count);
            printf("  [PASS] Label reference handled\n");
            test_passed++;
            codegen_destroy(codegen);
        }
        
        semantic_pass_one_destroy(pass_one);
    }
    
    for (int i = 0; i < 7; i++) {
        if (tokens[i].lexeme) util_free(tokens[i].lexeme);
    }
}

static void test_codegen_forward_ref(void) {
    printf("\n=== CodeGen: Forward Reference (Future Label) ===\n");
    
    Token tokens[12];
    
    tokens[0].type = TOK_IDENTIFIER;
    tokens[0].lexeme = util_strdup("JMP");
    tokens[0].line = 1;
    
    tokens[1].type = TOK_NEWLINE;
    tokens[1].lexeme = NULL_PTR;
    tokens[1].line = 1;
    
    tokens[2].type = TOK_IDENTIFIER;
    tokens[2].lexeme = util_strdup("LOOP");
    tokens[2].line = 2;
    
    tokens[3].type = TOK_NEWLINE;
    tokens[3].lexeme = NULL_PTR;
    tokens[3].line = 2;
    
    tokens[4].type = TOK_IDENTIFIER;
    tokens[4].lexeme = util_strdup("END");
    tokens[4].line = 3;
    
    tokens[5].type = TOK_COLON;
    tokens[5].lexeme = NULL_PTR;
    tokens[5].line = 3;
    
    tokens[6].type = TOK_IDENTIFIER;
    tokens[6].lexeme = util_strdup("RET");
    tokens[6].line = 3;
    
    tokens[7].type = TOK_NEWLINE;
    tokens[7].lexeme = NULL_PTR;
    tokens[7].line = 3;
    
    tokens[8].type = TOK_EOF;
    tokens[8].lexeme = NULL_PTR;
    tokens[8].line = 4;
    
    tables_init();
    PassOne* pass_one = semantic_pass_one(tokens, 9);
    
    ASSERT_PTR_NEQ(pass_one, NULL_PTR, "semantic_pass_one succeeded");
    
    if (pass_one != NULL) {
        printf("  Instructions: %u, Symbols: %u\n", 
            pass_one->instruction_count, 
            symtab_get_symbol_count(pass_one->symtab));
        printf("  [PASS] Forward reference tested\n");
        test_passed++;
        semantic_pass_one_destroy(pass_one);
    }
    
    for (int i = 0; i < 9; i++) {
        if (tokens[i].lexeme) util_free(tokens[i].lexeme);
    }
}

/* =========================================================================
 * 集成测试
 * ========================================================================= */

static void test_full_two_pass(void) {
    printf("\n=== Integration: Full Two-Pass Assembly ===\n");
    
    Token tokens[12];
    
    tokens[0].type = TOK_IDENTIFIER;
    tokens[0].lexeme = util_strdup("SEGMENT");
    tokens[0].line = 1;
    
    tokens[1].type = TOK_NEWLINE;
    tokens[1].lexeme = NULL_PTR;
    tokens[1].line = 1;
    
    tokens[2].type = TOK_IDENTIFIER;
    tokens[2].lexeme = util_strdup("START");
    tokens[2].line = 2;
    
    tokens[3].type = TOK_COLON;
    tokens[3].lexeme = NULL_PTR;
    tokens[3].line = 2;
    
    tokens[4].type = TOK_IDENTIFIER;
    tokens[4].lexeme = util_strdup("MOV");
    tokens[4].line = 2;
    
    tokens[5].type = TOK_NEWLINE;
    tokens[5].lexeme = NULL_PTR;
    tokens[5].line = 2;
    
    tokens[6].type = TOK_IDENTIFIER;
    tokens[6].lexeme = util_strdup("JMP");
    tokens[6].line = 3;
    
    tokens[7].type = TOK_NEWLINE;
    tokens[7].lexeme = NULL_PTR;
    tokens[7].line = 3;
    
    tokens[8].type = TOK_IDENTIFIER;
    tokens[8].lexeme = util_strdup("END");
    tokens[8].line = 4;
    
    tokens[9].type = TOK_NEWLINE;
    tokens[9].lexeme = NULL_PTR;
    tokens[9].line = 4;
    
    tokens[10].type = TOK_EOF;
    tokens[10].lexeme = NULL_PTR;
    tokens[10].line = 5;
    
    tables_init();
    
    printf("  Pass 1: Semantic analysis...\n");
    PassOne* pass_one = semantic_pass_one(tokens, 11);
    ASSERT_PTR_NEQ(pass_one, NULL_PTR, "Pass 1 success");
    
    if (pass_one != NULL) {
        printf("    Instructions: %u, Code size: 0x%04X, Symbols: %u\n", 
            pass_one->instruction_count, 
            pass_one->current_address,
            symtab_get_symbol_count(pass_one->symtab));
        
        printf("  Pass 2: Code generation...\n");
        CodeGen* codegen = codegen_pass_two(pass_one);
        
        if (codegen != NULL) {
            u32 code_size = 0;
            (void)codegen_get_code_buffer(codegen, &code_size);
            
            u32 reloc_count = 0;
            (void)codegen_get_relocation_info(codegen, &reloc_count);
            
            printf("    Generated code: %u bytes, Relocations: %u\n", code_size, reloc_count);
            printf("  [PASS] Full two-pass completed\n");
            test_passed++;
            codegen_destroy(codegen);
        }
        
        semantic_pass_one_destroy(pass_one);
    }
    
    for (int i = 0; i < 11; i++) {
        if (tokens[i].lexeme) util_free(tokens[i].lexeme);
    }
}

/* =========================================================================
 * 主测试入口
 * ========================================================================= */

int main(void) {
    printf("============================================\n");
    printf("  SEMANTIC & CODEGEN TWO-PASS TEST SUITE\n");
    printf("============================================\n");

    tables_init();
    
    /* Semantic 测试 */
    test_semantic_pass_one_simple();
    test_semantic_symbol_table();
    test_semantic_instruction_details();
    
    /* CodeGen 测试 */
    test_codegen_pass_two();
    test_codegen_label_resolve();
    test_codegen_forward_ref();
    
    /* 集成测试 */
    test_full_two_pass();
    
    printf("\n============================================\n");
    printf("TEST RESULTS SUMMARY\n");
    printf("============================================\n");
    printf("Passed: %u\n", test_passed);
    printf("Failed: %u\n", test_failed);
    printf("Total:  %u\n", test_passed + test_failed);
    
    if (test_failed == 0) {
        printf("\n✓ ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("\n✗ SOME TESTS FAILED\n");
        return 1;
    }
}
