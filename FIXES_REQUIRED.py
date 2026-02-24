#!/usr/bin/env python3
"""
SUBAS v0.1 Header/Implementation Mismatch Fixes
===============================================

This script documents all compilation-blocking issues and required fixes.
The .c implementations were written for a DIFFERENT interface than what
the actual headers define.

Generated: 2026-02-24
"""

# ==============================================================================
# ISSUE 1: semantic.c uses wrong TokenType enum constants
# ==============================================================================
# File: subas_v0.1/src/semantic.c
# Problem: Uses TOKEN_NEWLINE, TOKEN_EOF, TOKEN_IDENTIFIER, TOKEN_COLON, etc.
#          but lexer.h defines them as TOK_NEWLINE, TOK_EOF, etc.
# Fix Type: Global string replacement across file

ISSUE_1_FIXES = [
    {
        "file": "subas_v0.1/src/semantic.c",
        "type": "token_enum_constants",
        "replacements": [
            ("TOKEN_NEWLINE", "TOK_NEWLINE"),
            ("TOKEN_EOF", "TOK_EOF"),
            ("TOKEN_IDENTIFIER", "TOK_IDENTIFIER"),
            ("TOKEN_COLON", "TOK_COLON"),
            ("TOKEN_COMMA", "TOK_COMMA"),
            ("TOKEN_LBRACKET", "TOK_LBRACKET"),
            ("TOKEN_RBRACKET", "TOK_RBRACKET"),
        ],
        "description": "Replace all TokenType enum const references to match lexer.h",
        "matches_found": 20,  # Approximate count from grep
    }
]

# ==============================================================================
# ISSUE 2: symtab.c uses completely wrong data structure than header
# ==============================================================================
# File: subas_v0.1/src/symtab.c
# Problem: Implementation treats buckets[] as index array into nodes[] array,
#          but header defines buckets[] as SymbolNode* pointers (true hash table)
# Fix Type: Complete rewrite of symtab.c to use proper pointer-based hash table
# Impact: CRITICAL - symtab.c cannot compile/function as-is

ISSUE_2_DESCRIPTION = """
CRITICAL MISMATCH: symtab.c vs symtab.h

Header Definition (symtab.h):
    typedef struct {
        SymbolNode *buckets[MAX_SYMBOLS];  // <-- Pointers to SymbolNode
        unsigned int symbol_count;        // <-- NOT num_symbols
        unsigned int error_count;
    } SymbolTable;

Implementation Issues in symtab.c:
    1. Line 76: symtab->num_symbols = 0;
       Should be: symtab->symbol_count = 0;
       
    2. Line 80: symtab->buckets[i] = -1;
       Should be: symtab->buckets[i] = NULL;
       (buckets are SymbolNode* pointers, not int indices)
       
    3. Lines 83-89: References symtab->nodes[i]
       No such field exists in header!
       Need to dynamically allocate SymbolNode or use static array
       
    4. Line 128: current = symtab->buckets[hash_idx];
       If buckets[i] is -1, this is treating it as index (WRONG)
       Should be: current = symtab->buckets[hash_idx];
       And current should be SymbolNode* not int
       
    5. Line 130: symtab->nodes[current].name
       Should work with pointer-based linked list instead

Solution: Rewrite symtab.c to use proper hash table with pointer-based linked lists.
The buckets array should point directly to SymbolNode chains, not to array indices.
"""

ISSUE_2_FIXES = [
    {
        "file": "subas_v0.1/src/symtab.c",
        "type": "complete_rewrite",
        "critical": True,
        "changes": [
            {
                "location": "symtab_init() function",
                "old": """
void symtab_init(SymbolTable *symtab) {
    int i;
    
    if (!symtab) {
        return;
    }
    
    symtab->num_symbols = 0;
    
    for (i = 0; i < MAX_HASH_BUCKETS; i++) {
        symtab->buckets[i] = -1;
    }
    
    for (i = 0; i < MAX_SYMBOLS; i++) {
        symtab->nodes[i].name[0] = '\\0';
        symtab->nodes[i].value = 0;
        symtab->nodes[i].type = SYMBOL_LABEL;
        symtab->nodes[i].next = -1;
    }
}
""",
                "new": """
void symtab_init(SymbolTable *symtab) {
    int i;
    
    if (!symtab) {
        return;
    }
    
    symtab->symbol_count = 0;    // Changed: num_symbols -> symbol_count
    symtab->error_count = 0;
    
    for (i = 0; i < MAX_SYMBOLS; i++) {
        symtab->buckets[i] = NULL;  // Changed: -1 -> NULL (pointer-based)
    }
}
""",
            },
            {
                "location": "symtab_insert() function",
                "old": """
int symtab_insert(SymbolTable *symtab, const char *name, int value, int type) {
    int hash_idx;
    int node_idx;
    int current;
    
    if (!symtab || !name) {
        return 0;
    }
    
    current = symtab->buckets[symtab_hash(name)];
    while (current >= 0) {
        if (strcmp(symtab->nodes[current].name, name) == 0) {
            printf("Error: Duplicate symbol definition '%s'\\n", name);
            return 0;
        }
        current = symtab->nodes[current].next;
    }
    
    if (symtab->num_symbols >= MAX_SYMBOLS) {
        printf("Error: Symbol table full (max %d symbols)\\n", MAX_SYMBOLS);
        return 0;
    }
    
    hash_idx = symtab_hash(name);
    node_idx = symtab->num_symbols;
    
    strncpy(symtab->nodes[node_idx].name, name, MAX_SYMBOL_NAME - 1);
    symtab->nodes[node_idx].name[MAX_SYMBOL_NAME - 1] = '\\0';
    symtab->nodes[node_idx].value = value;
    symtab->nodes[node_idx].type = type;
    
    symtab->nodes[node_idx].next = symtab->buckets[hash_idx];
    symtab->buckets[hash_idx] = node_idx;
    
    symtab->num_symbols++;
    
    return 1;
}
""",
                "new": """
int symtab_insert(SymbolTable *symtab, const char *name, SymbolType type,
                  unsigned int address, int line) {
    SymbolNode *new_node;
    unsigned int hash_idx;
    SymbolNode *current;
    
    if (!symtab || !name) {
        return 0;
    }
    
    hash_idx = symtab_hash(name);
    current = symtab->buckets[hash_idx];
    
    // Check for duplicate
    while (current) {
        if (strcmp(current->name, name) == 0) {
            printf("Error: Duplicate symbol '%s' at line %d\\n", name, line);
            symtab->error_count++;
            return -1;  // Duplicate definition
        }
        current = current->next;
    }
    
    // Allocate new node
    new_node = (SymbolNode *)malloc(sizeof(SymbolNode));
    if (!new_node) {
        printf("Error: Memory allocation failed for symbol\\n");
        return 0;
    }
    
    // Fill in node
    strncpy(new_node->name, name, MAX_SYMBOL_NAME - 1);
    new_node->name[MAX_SYMBOL_NAME - 1] = '\\0';
    new_node->type = type;
    new_node->address = address;
    new_node->is_defined = 1;
    new_node->line = line;
    
    // Insert at chain head
    new_node->next = symtab->buckets[hash_idx];
    symtab->buckets[hash_idx] = new_node;
    
    symtab->symbol_count++;
    
    return 0;  // Success
}
""",
            },
            {
                "location": "symtab_lookup() function",
                "old": """
SymbolNode* symtab_lookup(SymbolTable *symtab, const char *name) {
    int hash_idx;
    int current;
    
    if (!symtab || !name) {
        return NULL;
    }
    
    hash_idx = symtab_hash(name);
    current = symtab->buckets[hash_idx];
    
    while (current >= 0) {
        if (strcmp(symtab->nodes[current].name, name) == 0) {
            return &symtab->nodes[current];
        }
        current = symtab->nodes[current].next;
    }
    
    return NULL;
}
""",
                "new": """
SymbolNode* symtab_lookup(SymbolTable *symtab, const char *name) {
    unsigned int hash_idx;
    SymbolNode *current;
    
    if (!symtab || !name) {
        return NULL;
    }
    
    hash_idx = symtab_hash(name);
    current = symtab->buckets[hash_idx];
    
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}
""",
            },
            {
                "location": "symtab_destroy() function",
                "old": """
void symtab_destroy(SymbolTable *symtab) {
    if (!symtab) {
        return;
    }
    
    symtab_init(symtab);
}
""",
                "new": """
void symtab_destroy(SymbolTable *symtab) {
    int i;
    SymbolNode *current, *next;
    
    if (!symtab) {
        return;
    }
    
    // Free all linked list nodes
    for (i = 0; i < MAX_SYMBOLS; i++) {
        current = symtab->buckets[i];
        while (current) {
            next = current->next;
            free(current);
            current = next;
        }
        symtab->buckets[i] = NULL;
    }
    
    symtab->symbol_count = 0;
    symtab->error_count = 0;
}
""",
            }
        ]
    }
]

# ==============================================================================
# ISSUE 3: main.c uses wrong field names for lexer and symtab
# ==============================================================================
# File: subas_v0.1/src/main.c
# Problem: Uses lexer.num_tokens instead of lexer.token_count
#          Uses symtab.num_symbols instead of symtab.symbol_count

ISSUE_3_FIXES = [
    {
        "file": "subas_v0.1/src/main.c",
        "type": "field_name_replacements",
        "replacements": [
            ("lexer.num_tokens", "lexer.token_count"),
            ("pass_one.symtab.num_symbols", "pass_one.symtab.symbol_count"),
        ],
        "description": "Fix field name mismatches",
        "lines_affected": [160, 161, 163, 180],
    }
]

# ==============================================================================
# ISSUE 4: codegen.h has unnecessary forward declaration
# ==============================================================================
# File: subas_v0.1/include/codegen.h
# Problem: Forward declares InstructionEntry but semantic.h fully defines it
# Fix Type: Remove forward declaration, add include

ISSUE_4_FIXES = [
    {
        "file": "subas_v0.1/include/codegen.h",
        "type": "remove_forward_decl_add_include",
        "current_code": """
#include "config.h"
#include "symtab.h"

typedef struct {
    ...
} CodeGen;

/* 前向声明，避免循环包含 */
typedef struct InstructionEntry InstructionEntry;
""",
        "new_code": """
#include "config.h"
#include "symtab.h"
#include "semantic.h"

typedef struct {
    ...
} CodeGen;

// InstructionEntry is defined in semantic.h
""",
        "description": "Remove InstructionEntry forward declaration and include semantic.h",
        "line": "After symtab.h include",
    }
]

# ==============================================================================
# SUMMARY
# ==============================================================================

SUMMARY = """
COMPILATION-BLOCKING FIXES REQUIRED
===================================

Priority 1 (CRITICAL - Prevents Compilation):
    1. symtab.c: Complete rewrite to use pointer-based hash table
       - File: subas_v0.1/src/symtab.c
       - Lines affected: 50-230 (entire hash table implementation)
       - Reason: Structure mismatch (nodes[] array vs buckets[] pointers)

    2. semantic.c: Replace TokenType enum constants
       - File: subas_v0.1/src/semantic.c
       - Replacements: TOKEN_* -> TOK_*
       - Lines affected: ~20 lines
       - Reason: lexer.h defines TOK_NEWLINE, not TOKEN_NEWLINE

    3. main.c: Fix field name references
       - File: subas_v0.1/src/main.c
       - Changes: lexer.num_tokens -> lexer.token_count
                 symtab.num_symbols -> symtab.symbol_count
       - Reason: Headers have different field names

Priority 2 (Recommended):
    4. codegen.h: Remove InstructionEntry forward declaration
       - File: subas_v0.1/include/codegen.h
       - Change: Add #include "semantic.h", remove typedef struct
       - Reason: Avoid duplicate definitions

Total Files to Modify: 4
- symtab.c (complete rewrite)
- semantic.c (token type fixes)
- main.c (field name fixes)
- codegen.h (include fix)

Expected Compilation Status After Fixes:
    BEFORE: Link-time and runtime errors
    AFTER: Should compile successfully
"""

if __name__ == "__main__":
    print(SUMMARY)
    print("\n" + "=" * 80)
    print("DETAILED CHANGES BY FILE")
    print("=" * 80)
    
    print("\n[1] subas_v0.1/src/semantic.c")
    print("-" * 80)
    for fix in ISSUE_1_FIXES:
        for old, new in fix["replacements"]:
            print(f"    OLD: {old}")
            print(f"    NEW: {new}")
            print()
    
    print("\n[2] subas_v0.1/src/symtab.c")
    print("-" * 80)
    print(ISSUE_2_DESCRIPTION)
    
    print("\n[3] subas_v0.1/src/main.c")
    print("-" * 80)
    for fix in ISSUE_3_FIXES:
        for old, new in fix["replacements"]:
            print(f"    OLD: {old}")
            print(f"    NEW: {new}")
            print()
    
    print("\n[4] subas_v0.1/include/codegen.h")
    print("-" * 80)
    print("    REMOVE: typedef struct InstructionEntry InstructionEntry;")
    print("    ADD: #include \"semantic.h\" (after #include \"symtab.h\")")
