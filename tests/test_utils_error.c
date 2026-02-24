/*
 * ============================================================================
 * 文件名: test_utils_error.c
 * 描述  : Utils 和 Error 模块单元测试驱动程序
 *
 * 测试覆盖范围：
 *  - Error 模块：错误初始化、错误报告、错误计数、错误检测
 *  - Utils 内存管理：malloc/free 基本操作
 *  - Utils 字符串处理：strlen、strcpy、strcmp、strdup
 *  - Utils 哈希表：创建、插入、查找、销毁
 *
 * 编译命令示例（在项目根目录）：
 *   gcc -o tests/test_utils_error tests/test_utils_error.c \
 *       src/error.c src/utils/memory.c src/utils/string.c src/utils/hash.c -I. -Wall -Wextra
 *
 * ============================================================================
 */

#include <stdio.h>
#include "../include/utils.h"
#include "../include/error.h"

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

#define ASSERT_STR_EQ(actual, expected, msg) \
    do { \
        if (util_strcmp((actual), (expected)) != 0) { \
            printf("  [FAIL] %s: expected '%s', got '%s'\n", (msg), (expected), (actual)); \
            test_failed++; \
        } else { \
            printf("  [PASS] %s\n", (msg)); \
            test_passed++; \
        } \
    } while (0)

#define ASSERT_PTR_EQ(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            printf("  [FAIL] %s: pointer mismatch\n", (msg)); \
            test_failed++; \
        } else { \
            printf("  [PASS] %s\n", (msg)); \
            test_passed++; \
        } \
    } while (0)

#define ASSERT_PTR_NEQ(actual, expected, msg) \
    do { \
        if ((actual) == (expected)) { \
            printf("  [FAIL] %s: pointer should not match\n", (msg)); \
            test_failed++; \
        } else { \
            printf("  [PASS] %s\n", (msg)); \
            test_passed++; \
        } \
    } while (0)

static u32 test_passed = 0;
static u32 test_failed = 0;

/* =========================================================================
 * ERROR 模块测试
 * ========================================================================= */

static void test_error_init(void) {
    printf("\n=== Error Module: Initialization ===\n");
    error_init();
    ASSERT_EQ(error_get_count(), 0, "error_get_count() after init");
    ASSERT_EQ(error_has_failed(), FALSE, "error_has_failed() after init");
}

static void test_error_report(void) {
    printf("\n=== Error Module: Error Reporting ===\n");
    error_init();

    printf("  Reporting: [Line 10] E2003: Unknown mnemonic\n");
    error_report(10, ERR_PARSE_UNK_MNEMONIC, NULL_PTR);
    ASSERT_EQ(error_get_count(), 1, "error_get_count() after 1st report");
    ASSERT_EQ(error_has_failed(), TRUE, "error_has_failed() after 1st report");

    printf("  Reporting: [Line 15] E2004: Duplicate label\n");
    error_report(15, ERR_PARSE_DUP_LABEL, "LABEL_A");
    ASSERT_EQ(error_get_count(), 2, "error_get_count() after 2nd report");

    printf("  Reporting: [Line 20] E2005: Undefined symbol\n");
    error_report(20, ERR_PARSE_UNDEFINED_LBL, "UNDEFINED_SYM");
    ASSERT_EQ(error_get_count(), 3, "error_get_count() after 3rd report");
}

static void test_error_types(void) {
    printf("\n=== Error Module: Different Error Types ===\n");
    error_init();

    /* 词法错误 */
    printf("  Lexical errors:\n");
    error_report(5, ERR_LEX_INVALID_CHAR, "'@'");
    error_report(6, ERR_LEX_UNCLOSED_STR, NULL_PTR);
    error_report(7, ERR_LEX_INVALID_NUM, "0x_FF");
    ASSERT_EQ(error_get_count(), 3, "lexical error count");

    error_init();
    printf("  Syntax errors:\n");
    error_report(10, ERR_PARSE_EXPECTED_OP, "after MOV");
    error_report(11, ERR_PARSE_INVALID_REG, "RX");
    ASSERT_EQ(error_get_count(), 2, "syntax error count");

    error_init();
    printf("  System errors:\n");
    error_report(1, ERR_SYS_OUT_OF_MEM, NULL_PTR);
    ASSERT_EQ(error_get_count(), 1, "system error count");
}

/* =========================================================================
 * UTILS 字符串处理测试
 * ========================================================================= */

static void test_strlen(void) {
    printf("\n=== Utils: String Length ===\n");
    ASSERT_EQ(util_strlen(""), 0, "strlen(\"\")");
    ASSERT_EQ(util_strlen("hello"), 5, "strlen(\"hello\")");
    ASSERT_EQ(util_strlen("a"), 1, "strlen(\"a\")");
    ASSERT_EQ(util_strlen("0x12AB"), 6, "strlen(\"0x12AB\")");
}

static void test_strcmp(void) {
    printf("\n=== Utils: String Comparison ===\n");
    ASSERT_EQ(util_strcmp("abc", "abc"), 0, "strcmp(\"abc\", \"abc\") == 0");
    ASSERT_EQ(util_strcmp("abc", "abd") < 0, 1, "strcmp(\"abc\", \"abd\") < 0");
    ASSERT_EQ(util_strcmp("abd", "abc") > 0, 1, "strcmp(\"abd\", \"abc\") > 0");
    ASSERT_EQ(util_strcmp("", "") == 0, 1, "strcmp(\"\", \"\") == 0");
    ASSERT_EQ(util_strcmp("a", "") > 0, 1, "strcmp(\"a\", \"\") > 0");
}

static void test_strcpy(void) {
    printf("\n=== Utils: String Copy ===\n");
    char buf[64];

    util_strcpy(buf, "hello");
    ASSERT_STR_EQ(buf, "hello", "strcpy basic");

    util_strcpy(buf, "");
    ASSERT_STR_EQ(buf, "", "strcpy empty");

    util_strcpy(buf, "0x12AB+3");
    ASSERT_STR_EQ(buf, "0x12AB+3", "strcpy with special chars");
}

static void test_strdup(void) {
    printf("\n=== Utils: String Duplication ===\n");

    char* dup1 = util_strdup("hello");
    ASSERT_PTR_NEQ(dup1, NULL_PTR, "strdup(\"hello\") != NULL");
    ASSERT_STR_EQ(dup1, "hello", "strdup content check");
    util_free(dup1);

    char* dup2 = util_strdup("0x12AB");
    ASSERT_STR_EQ(dup2, "0x12AB", "strdup hex string");
    util_free(dup2);

    char* dup3 = util_strdup("");
    ASSERT_STR_EQ(dup3, "", "strdup empty string");
    util_free(dup3);

    char* null_dup = util_strdup(NULL_PTR);
    ASSERT_PTR_EQ(null_dup, NULL_PTR, "strdup(NULL) returns NULL");
}

/* =========================================================================
 * UTILS 内存管理测试
 * ========================================================================= */

static void test_malloc_free(void) {
    printf("\n=== Utils: Memory Allocation and Deallocation ===\n");

    /* 分配不同大小 */
    u8* buf1 = (u8*)util_malloc(1);
    ASSERT_PTR_NEQ(buf1, NULL_PTR, "malloc(1) success");
    util_free(buf1);

    u32* buf2 = (u32*)util_malloc(sizeof(u32) * 10);
    ASSERT_PTR_NEQ(buf2, NULL_PTR, "malloc(40 bytes) success");
    buf2[0] = 100;
    buf2[9] = 999;
    ASSERT_EQ(buf2[0], 100, "memory write/read after malloc");
    util_free(buf2);

    char* buf3 = (char*)util_malloc(256);
    ASSERT_PTR_NEQ(buf3, NULL_PTR, "malloc(256) success");
    util_strcpy(buf3, "test string");
    ASSERT_STR_EQ(buf3, "test string", "strcpy after malloc");
    util_free(buf3);

    /* 释放 NULL 不应崩溃 */
    util_free(NULL_PTR);
    printf("  [PASS] free(NULL) safe\n");
    test_passed++;
}

/* =========================================================================
 * UTILS 哈希表测试
 * ========================================================================= */

static void test_hashtable_create_destroy(void) {
    printf("\n=== Utils: Hash Table Creation and Destruction ===\n");

    UtilHashTable* ht = util_ht_create(10);
    ASSERT_PTR_NEQ(ht, NULL_PTR, "util_ht_create() success");
    ASSERT_EQ(ht->bucket_count, 10, "bucket count set correctly");
    ASSERT_EQ(ht->element_count, 0, "element count initialized to 0");

    util_ht_destroy(ht);
    printf("  [PASS] hash table destroyed\n");
    test_passed++;
}

static void test_hashtable_insert_lookup(void) {
    printf("\n=== Utils: Hash Table Insert and Lookup ===\n");

    UtilHashTable* ht = util_ht_create(16);

    /* 插入字符串值 */
    util_ht_insert(ht, "MOV", (void*)"move instruction");
    util_ht_insert(ht, "ADD", (void*)"add instruction");
    util_ht_insert(ht, "0xFF", (void*)"hex value");

    /* 查找 */
    char* val1 = (char*)util_ht_lookup(ht, "MOV");
    ASSERT_STR_EQ(val1, "move instruction", "lookup(\"MOV\")");

    char* val2 = (char*)util_ht_lookup(ht, "ADD");
    ASSERT_STR_EQ(val2, "add instruction", "lookup(\"ADD\")");

    char* val3 = (char*)util_ht_lookup(ht, "0xFF");
    ASSERT_STR_EQ(val3, "hex value", "lookup(\"0xFF\")");

    /* 查找不存在的键 */
    void* not_found = util_ht_lookup(ht, "SUB");
    ASSERT_PTR_EQ(not_found, NULL_PTR, "lookup non-existent key returns NULL");

    util_ht_destroy(ht);
}

static void test_hashtable_update(void) {
    printf("\n=== Utils: Hash Table Update Values ===\n");

    UtilHashTable* ht = util_ht_create(8);

    /* 首次插入 */
    util_ht_insert(ht, "LABEL", (void*)"0");
    ASSERT_EQ(ht->element_count, 1, "element count after first insert");

    u32* old_val = (u32*)util_ht_lookup(ht, "LABEL");
    ASSERT_PTR_EQ((void*)"0", old_val, "first value lookup");

    /* 更新现有键 */
    util_ht_insert(ht, "LABEL", (void*)"0x100");
    ASSERT_EQ(ht->element_count, 1, "element count after update (should not increase)");

    u32* new_val = (u32*)util_ht_lookup(ht, "LABEL");
    ASSERT_PTR_EQ((void*)"0x100", new_val, "updated value lookup");

    util_ht_destroy(ht);
}

static void test_hashtable_collision(void) {
    printf("\n=== Utils: Hash Table Collision Handling ===\n");

    /* 创建一个较小的哈希表（增加碰撞概率） */
    UtilHashTable* ht = util_ht_create(4);

    /* 插入多个项 */
    util_ht_insert(ht, "key1", (void*)"value1");
    util_ht_insert(ht, "key2", (void*)"value2");
    util_ht_insert(ht, "key3", (void*)"value3");
    util_ht_insert(ht, "key4", (void*)"value4");
    util_ht_insert(ht, "key5", (void*)"value5");

    ASSERT_EQ(ht->element_count, 5, "element count after 5 inserts");

    /* 验证所有项都能正确查找 */
    ASSERT_STR_EQ((char*)util_ht_lookup(ht, "key1"), "value1", "collision test: key1");
    ASSERT_STR_EQ((char*)util_ht_lookup(ht, "key2"), "value2", "collision test: key2");
    ASSERT_STR_EQ((char*)util_ht_lookup(ht, "key3"), "value3", "collision test: key3");
    ASSERT_STR_EQ((char*)util_ht_lookup(ht, "key4"), "value4", "collision test: key4");
    ASSERT_STR_EQ((char*)util_ht_lookup(ht, "key5"), "value5", "collision test: key5");

    util_ht_destroy(ht);
}

static void test_hashtable_masm_instructions(void) {
    printf("\n=== Utils: Hash Table with MASM Instructions ===\n");

    UtilHashTable* instr_table = util_ht_create(32);

    /* 插入汇编指令 */
    util_ht_insert(instr_table, "MOV", (void*)"move");
    util_ht_insert(instr_table, "ADD", (void*)"add");
    util_ht_insert(instr_table, "SUB", (void*)"subtract");
    util_ht_insert(instr_table, "MUL", (void*)"multiply");
    util_ht_insert(instr_table, "DIV", (void*)"divide");
    util_ht_insert(instr_table, "JMP", (void*)"jump");
    util_ht_insert(instr_table, "CALL", (void*)"call");
    util_ht_insert(instr_table, "RET", (void*)"return");
    util_ht_insert(instr_table, "PUSH", (void*)"push");
    util_ht_insert(instr_table, "POP", (void*)"pop");

    ASSERT_EQ(instr_table->element_count, 10, "instruction table size");

    /* 查找部分指令 */
    ASSERT_PTR_NEQ(util_ht_lookup(instr_table, "MOV"), NULL_PTR, "lookup MOV");
    ASSERT_PTR_NEQ(util_ht_lookup(instr_table, "CALL"), NULL_PTR, "lookup CALL");
    ASSERT_PTR_NEQ(util_ht_lookup(instr_table, "POP"), NULL_PTR, "lookup POP");

    /* 查找不存在的指令 */
    ASSERT_PTR_EQ(util_ht_lookup(instr_table, "NOTEXIST"), NULL_PTR, "lookup non-existent");

    util_ht_destroy(instr_table);
}

/* =========================================================================
 * 主测试入口
 * ========================================================================= */

int main(void) {
    printf("========================================\n");
    printf("  UTILS & ERROR MODULE UNIT TESTS\n");
    printf("========================================\n");

    /* Error 模块测试 */
    test_error_init();
    test_error_report();
    test_error_types();

    /* Utils 字符串测试 */
    test_strlen();
    test_strcmp();
    test_strcpy();
    test_strdup();

    /* Utils 内存测试 */
    test_malloc_free();

    /* Utils 哈希表测试 */
    test_hashtable_create_destroy();
    test_hashtable_insert_lookup();
    test_hashtable_update();
    test_hashtable_collision();
    test_hashtable_masm_instructions();

    printf("\n========================================\n");
    printf("TEST RESULTS SUMMARY\n");
    printf("========================================\n");
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


