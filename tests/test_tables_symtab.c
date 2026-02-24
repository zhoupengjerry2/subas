/*
 * ============================================================================
 * 文件名: test_tables_symtab.c
 * 描述  : Tables 和 Symtab 模块单元测试驱动程序
 *
 * 测试覆盖范围：
 *  - Tables 模块：指令查找、伪指令识别、指令属性查询
 *  - Symtab 模块：符号插入、查找、地址更新、符号类型等
 *
 * 编译命令示例（在项目根目录）：
 *   gcc -o tests/test_tables_symtab tests/test_tables_symtab.c \
 *       src/tables.c src/symtab.c src/utils/memory.c src/utils/string.c \
 *       src/utils/hash.c -I. -Wall -Wextra
 *
 * ============================================================================
 */

#include <stdio.h>
#include "../include/tables.h"
#include "../include/symtab.h"
#include "../include/utils.h"

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
 * TABLES 模块测试
 * ========================================================================= */

static void test_tables_init(void) {
    printf("\n=== Tables: Initialization ===\n");
    tables_init();
    u32 count = tables_get_instruction_count();
    printf("  Total instructions in table: %u\n", count);
    ASSERT_EQ(count > 0, 1, "instruction count > 0");
}

static void test_tables_lookup_regular_instructions(void) {
    printf("\n=== Tables: Lookup Regular Instructions ===\n");
    
    const InstructionInfo* mov = tables_lookup_instruction("MOV");
    ASSERT_PTR_NEQ(mov, NULL_PTR, "lookup MOV");
    ASSERT_STR_EQ(mov->mnemonic, "MOV", "MOV mnemonic");
    ASSERT_EQ(mov->is_pseudo, 0, "MOV is not pseudo");
    ASSERT_EQ(mov->operand_count, 2, "MOV operand count");
    
    const InstructionInfo* add = tables_lookup_instruction("ADD");
    ASSERT_PTR_NEQ(add, NULL_PTR, "lookup ADD");
    ASSERT_EQ(add->operand_count, 2, "ADD operand count");
    
    const InstructionInfo* ret = tables_lookup_instruction("RET");
    ASSERT_PTR_NEQ(ret, NULL_PTR, "lookup RET");
    ASSERT_EQ(ret->operand_count, 0, "RET operand count");
}

static void test_tables_lookup_case_insensitive(void) {
    printf("\n=== Tables: Case-Insensitive Lookup ===\n");
    
    const InstructionInfo* mov1 = tables_lookup_instruction("MOV");
    const InstructionInfo* mov2 = tables_lookup_instruction("mov");
    const InstructionInfo* mov3 = tables_lookup_instruction("Mov");
    
    ASSERT_PTR_EQ(mov1, mov2, "MOV vs mov");
    ASSERT_PTR_EQ(mov1, mov3, "MOV vs Mov");
    ASSERT_EQ(mov1->is_pseudo, 0, "All are same MOV instruction");
}

static void test_tables_lookup_pseudo_instructions(void) {
    printf("\n=== Tables: Lookup Pseudo-Instructions ===\n");
    
    const InstructionInfo* segment = tables_lookup_instruction("SEGMENT");
    ASSERT_PTR_NEQ(segment, NULL_PTR, "lookup SEGMENT");
    ASSERT_EQ(segment->is_pseudo, 1, "SEGMENT is pseudo");
    
    const InstructionInfo* db = tables_lookup_instruction("DB");
    ASSERT_PTR_NEQ(db, NULL_PTR, "lookup DB");
    ASSERT_EQ(db->is_pseudo, 1, "DB is pseudo");
    
    const InstructionInfo* proc = tables_lookup_instruction("PROC");
    ASSERT_PTR_NEQ(proc, NULL_PTR, "lookup PROC");
    ASSERT_EQ(proc->is_pseudo, 1, "PROC is pseudo");
}

static void test_tables_is_pseudo(void) {
    printf("\n=== Tables: Is Pseudo Check ===\n");
    
    ASSERT_EQ(tables_is_pseudo("MOV"), 0, "MOV is not pseudo");
    ASSERT_EQ(tables_is_pseudo("ADD"), 0, "ADD is not pseudo");
    ASSERT_EQ(tables_is_pseudo("SEGMENT"), 1, "SEGMENT is pseudo");
    ASSERT_EQ(tables_is_pseudo("DB"), 1, "DB is pseudo");
    ASSERT_EQ(tables_is_pseudo("UNKNOWN"), -1, "UNKNOWN returns -1");
}

static void test_tables_lookup_not_found(void) {
    printf("\n=== Tables: Lookup Non-existent ===\n");
    
    const InstructionInfo* not_found = tables_lookup_instruction("NOTEXIST");
    ASSERT_PTR_EQ(not_found, NULL_PTR, "non-existent instruction returns NULL");
}

static void test_tables_get_by_index(void) {
    printf("\n=== Tables: Get Instruction by Index ===\n");
    
    u32 count = tables_get_instruction_count();
    const InstructionInfo* first = tables_get_instruction_by_index(0);
    ASSERT_PTR_NEQ(first, NULL_PTR, "index 0 is valid");
    
    const InstructionInfo* last = tables_get_instruction_by_index(count - 1);
    ASSERT_PTR_NEQ(last, NULL_PTR, "last index is valid");
    
    const InstructionInfo* out_of_range = tables_get_instruction_by_index(count + 10);
    ASSERT_PTR_EQ(out_of_range, NULL_PTR, "out of range returns NULL");
}

static void test_tables_jump_instructions(void) {
    printf("\n=== Tables: Jump Instructions ===\n");
    
    const InstructionInfo* jmp = tables_lookup_instruction("JMP");
    ASSERT_PTR_NEQ(jmp, NULL_PTR, "lookup JMP");
    ASSERT_EQ(jmp->operand_count, 1, "JMP operand count");
    
    const InstructionInfo* jz = tables_lookup_instruction("JZ");
    ASSERT_PTR_NEQ(jz, NULL_PTR, "lookup JZ");
    
    const InstructionInfo* loop_instr = tables_lookup_instruction("LOOP");
    ASSERT_PTR_NEQ(loop_instr, NULL_PTR, "lookup LOOP");
}

/* =========================================================================
 * SYMTAB 模块测试
 * ========================================================================= */

static void test_symtab_create_destroy(void) {
    printf("\n=== Symtab: Create and Destroy ===\n");
    
    SymbolTable* symtab = symtab_create(16);
    ASSERT_PTR_NEQ(symtab, NULL_PTR, "symtab_create() success");
    ASSERT_EQ(symtab_get_symbol_count(symtab), 0, "initial symbol count = 0");
    
    symtab_destroy(symtab);
    printf("  [PASS] symtab destroyed\n");
    test_passed++;
}

static void test_symtab_insert_and_lookup(void) {
    printf("\n=== Symtab: Insert and Lookup ===\n");
    
    SymbolTable* symtab = symtab_create(32);
    
    /* 插入第一个标签 */
    int result1 = symtab_insert(symtab, "LABEL_A", SYM_LABEL, 0x100, 10);
    ASSERT_EQ(result1, 0, "insert LABEL_A");
    ASSERT_EQ(symtab_get_symbol_count(symtab), 1, "symbol count after insert");
    
    /* 查找 */
    SymbolInfo* found = symtab_lookup(symtab, "LABEL_A");
    ASSERT_PTR_NEQ(found, NULL_PTR, "lookup LABEL_A");
    ASSERT_EQ(found->address, 0x100, "LABEL_A address");
    ASSERT_EQ(found->line_defined, 10, "LABEL_A line");
    ASSERT_EQ(found->type, SYM_LABEL, "LABEL_A type");
    
    /* 尝试重复插入 */
    int result2 = symtab_insert(symtab, "LABEL_A", SYM_LABEL, 0x200, 20);
    ASSERT_EQ(result2, 1, "duplicate insert returns 1");
    ASSERT_EQ(symtab_get_symbol_count(symtab), 1, "symbol count unchanged");
    
    symtab_destroy(symtab);
}

static void test_symtab_multiple_symbols(void) {
    printf("\n=== Symtab: Multiple Symbols ===\n");
    
    SymbolTable* symtab = symtab_create(64);
    
    /* 插入多个不同类型的符号 */
    symtab_insert(symtab, "START", SYM_LABEL, 0x0000, 1);
    symtab_insert(symtab, "DATA_BUF", SYM_VARIABLE, 0x1000, 5);
    symtab_insert(symtab, "PROC_MAIN", SYM_PROCEDURE, 0x0050, 10);
    symtab_insert(symtab, "LOOP_END", SYM_LABEL, 0x0100, 20);
    
    ASSERT_EQ(symtab_get_symbol_count(symtab), 4, "4 symbols inserted");
    
    /* 查找各个符号 */
    SymbolInfo* start = symtab_lookup(symtab, "START");
    ASSERT_EQ(start->address, 0x0000, "START address");
    ASSERT_EQ(start->type, SYM_LABEL, "START type");
    
    SymbolInfo* data = symtab_lookup(symtab, "DATA_BUF");
    ASSERT_EQ(data->address, 0x1000, "DATA_BUF address");
    ASSERT_EQ(data->type, SYM_VARIABLE, "DATA_BUF type");
    
    SymbolInfo* proc = symtab_lookup(symtab, "PROC_MAIN");
    ASSERT_EQ(proc->address, 0x0050, "PROC_MAIN address");
    ASSERT_EQ(proc->type, SYM_PROCEDURE, "PROC_MAIN type");
    
    symtab_destroy(symtab);
}

static void test_symtab_update_address(void) {
    printf("\n=== Symtab: Update Address ===\n");
    
    SymbolTable* symtab = symtab_create(16);
    
    symtab_insert(symtab, "LABEL", SYM_LABEL, 0x100, 5);
    SymbolInfo* info = symtab_lookup(symtab, "LABEL");
    ASSERT_EQ(info->address, 0x100, "initial address");
    
    /* 更新地址 */
    int result = symtab_update_address(symtab, "LABEL", 0x200);
    ASSERT_EQ(result, 0, "update success");
    
    info = symtab_lookup(symtab, "LABEL");
    ASSERT_EQ(info->address, 0x200, "updated address");
    
    /* 尝试更新不存在的符号 */
    int not_found = symtab_update_address(symtab, "NOTEXIST", 0x300);
    ASSERT_EQ(not_found, -1, "update non-existent returns -1");
    
    symtab_destroy(symtab);
}

static void test_symtab_mark_defined(void) {
    printf("\n=== Symtab: Mark Defined ===\n");
    
    SymbolTable* symtab = symtab_create(16);
    
    symtab_insert(symtab, "LABEL", SYM_LABEL, 0x100, 5);
    SymbolInfo* info = symtab_lookup(symtab, "LABEL");
    ASSERT_EQ(info->is_defined, 1, "initially defined");
    
    /* 标记为已定义（虽然已经是）*/
    int result = symtab_mark_defined(symtab, "LABEL");
    ASSERT_EQ(result, 0, "mark_defined success");
    
    /* 尝试标记不存在的符号 */
    int not_found = symtab_mark_defined(symtab, "NOTEXIST");
    ASSERT_EQ(not_found, -1, "mark_defined non-existent returns -1");
    
    symtab_destroy(symtab);
}

static void test_symtab_lookup_not_found(void) {
    printf("\n=== Symtab: Lookup Non-existent ===\n");
    
    SymbolTable* symtab = symtab_create(16);
    
    symtab_insert(symtab, "LABEL_A", SYM_LABEL, 0x100, 5);
    
    SymbolInfo* found = symtab_lookup(symtab, "NOTEXIST");
    ASSERT_PTR_EQ(found, NULL_PTR, "lookup non-existent returns NULL");
    
    symtab_destroy(symtab);
}

static void test_symtab_assembly_scenario(void) {
    printf("\n=== Symtab: Assembly-like Scenario ===\n");
    
    SymbolTable* symtab = symtab_create(64);
    
    /* 模拟汇编器的 Pass 1：收集所有符号 */
    printf("  Pass 1: Collecting symbols\n");
    symtab_insert(symtab, "SEGMENT", SYM_LABEL, 0x0000, 1);
    symtab_insert(symtab, "MAIN", SYM_PROCEDURE, 0x0000, 5);
    symtab_insert(symtab, "LOOP_START", SYM_LABEL, 0x0010, 10);
    symtab_insert(symtab, "LOOP_END", SYM_LABEL, 0x0020, 15);
    symtab_insert(symtab, "DATA", SYM_VARIABLE, 0x1000, 20);
    
    ASSERT_EQ(symtab_get_symbol_count(symtab), 5, "5 symbols collected");
    
    /* 模拟 Pass 2：解析代码时，需要查找和可能更新地址 */
    printf("  Pass 2: Resolving references\n");
    SymbolInfo* loop_end = symtab_lookup(symtab, "LOOP_END");
    ASSERT_PTR_NEQ(loop_end, NULL_PTR, "LOOP_END found in Pass 2");
    ASSERT_EQ(loop_end->address, 0x0020, "LOOP_END address");
    
    /* 前向引用解决（如果在 Pass 1 中还不知道确切地址，可在 Pass 2 更新） */
    symtab_update_address(symtab, "DATA", 0x1100);
    SymbolInfo* data = symtab_lookup(symtab, "DATA");
    ASSERT_EQ(data->address, 0x1100, "DATA address updated");
    
    symtab_destroy(symtab);
}

/* =========================================================================
 * 主测试入口
 * ========================================================================= */

int main(void) {
    printf("========================================\n");
    printf("  TABLES & SYMTAB MODULE UNIT TESTS\n");
    printf("========================================\n");

    /* Tables 模块测试 */
    test_tables_init();
    test_tables_lookup_regular_instructions();
    test_tables_lookup_case_insensitive();
    test_tables_lookup_pseudo_instructions();
    test_tables_is_pseudo();
    test_tables_lookup_not_found();
    test_tables_get_by_index();
    test_tables_jump_instructions();
    
    /* Symtab 模块测试 */
    test_symtab_create_destroy();
    test_symtab_insert_and_lookup();
    test_symtab_multiple_symbols();
    test_symtab_update_address();
    test_symtab_mark_defined();
    test_symtab_lookup_not_found();
    test_symtab_assembly_scenario();
    
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
