/*
 * ============================================================================
 * 文件名: semantic.c
 * 描述  : 语义分析模块实现 - 第一遍扫描
 *
 * 关键算法：
 *  1. 遍历 Token 流，识别助记符和伪指令
 *  2. 对每条指令计算其地址和长度
 *  3. 对标签和符号进行符号表登记
 *  4. 检测语义错误（重复定义等）
 *
 * ============================================================================
 */

#include <stdio.h>
#include "../include/semantic.h"

/* ========================================================================= */
/* 内部辅助函数声明 */
/* ========================================================================= */

/*
 * 检查 Token 是否为寄存器名
 */
static int is_register(const char* name) {
    const char* regs[] = {"AX", "BX", "CX", "DX", "AH", "AL", "BH", "BL",
                        "CH", "CL", "DH", "DL", "SI", "DI", "BP", "SP", NULL};
    for (int i = 0; regs[i] != NULL; i++) {
        if (util_strcmp(name, regs[i]) == 0) return 1;
    }
    return 0;
}

/*
 * 获取默认的指令长度估计（用于 Pass 1）
 * 实际长度在代码生成时才精确计算
 */
static u32 estimate_instruction_length(const char* mnemonic, u32 operand_count) {
    (void)operand_count;
    /* 简化估计：大多数指令 2-3 字节 */
    if (util_strcmp(mnemonic, "DB") == 0) return 1;
    if (util_strcmp(mnemonic, "ORG") == 0) return 0;  /* 伪指令 */
    if (util_strcmp(mnemonic, "SEGMENT") == 0) return 0;
    if (util_strcmp(mnemonic, "ENDS") == 0) return 0;
    if (util_strcmp(mnemonic, "PROC") == 0) return 0;
    if (util_strcmp(mnemonic, "ENDP") == 0) return 0;
    if (util_strcmp(mnemonic, "END") == 0) return 0;
    return 3;
}

/* ========================================================================= */
/* API 函数实现 */
/* ========================================================================= */

/*
 * semantic_pass_one: 执行第一遍扫描
 */
PassOne* semantic_pass_one(const Token* tokens, u32 token_count) {
    PassOne* pass_one = (PassOne*)util_malloc(sizeof(PassOne));
    if (pass_one == NULL) {
        error_report(0, ERR_SYS_OUT_OF_MEM, "无法分配 PassOne 结构");
        return NULL;
    }

    pass_one->symtab = symtab_create(256);
    if (pass_one->symtab == NULL) {
        util_free(pass_one);
        error_report(0, ERR_SYS_OUT_OF_MEM, "无法创建符号表");
        return NULL;
    }

    pass_one->max_instructions = 512;
    pass_one->instructions = (InstructionEntry*)util_malloc(
        sizeof(InstructionEntry) * pass_one->max_instructions
    );
    if (pass_one->instructions == NULL) {
        symtab_destroy(pass_one->symtab);
        util_free(pass_one);
        error_report(0, ERR_SYS_OUT_OF_MEM, "无法分配指令列表");
        return NULL;
    }

    pass_one->instruction_count = 0;
    pass_one->current_address = 0;
    pass_one->current_line = 1;
    pass_one->has_errors = 0;

    /* 遍历 Token 流，提取指令 */
    u32 i = 0;
    while (i < token_count) {
        if (tokens[i].type == TOK_NEWLINE || tokens[i].type == TOK_EOF) {
            i++;
            pass_one->current_line++;
            continue;
        }

        /* 尝试解析一条指令 */
        if (pass_one->instruction_count >= pass_one->max_instructions) {
            pass_one->has_errors = 1;
            error_report(tokens[i].line, ERR_PARSE_EXPECTED_OP, "指令数超过限制");
            break;
        }

        InstructionEntry* entry = &pass_one->instructions[pass_one->instruction_count];
        int tokens_consumed = semantic_analyze_instruction(pass_one, tokens, i, entry);

        if (tokens_consumed < 0) {
            pass_one->has_errors = 1;
            /* 报告无法解析的 token，以便调试 */
            if (i < token_count && tokens[i].lexeme != NULL) {
                error_report(tokens[i].line, ERR_PARSE_EXPECTED_OP, (const char*)tokens[i].lexeme);
            } else {
                error_report(pass_one->current_line, ERR_PARSE_EXPECTED_OP, "无法解析的指令或伪指令");
            }
            i++;
            continue;
        }

        entry->address = pass_one->current_address;
        entry->line = pass_one->current_line;

        /* 预估指令长度 */
        entry->length = estimate_instruction_length(entry->mnemonic, entry->operand_count);
        pass_one->current_address += entry->length;

        /* 如果指令有标签，登记到符号表 */
        if (entry->has_label) {
            int result = symtab_insert(
                pass_one->symtab,
                entry->label,
                SYM_LABEL,
                entry->address,
                entry->line
            );
            if (result != 0) {
                pass_one->has_errors = 1;
                error_report(entry->line, ERR_PARSE_DUP_LABEL,
                    "标签重复定义");
            }
        }

        pass_one->instruction_count++;
        i += tokens_consumed;
    }

    if (pass_one->has_errors) {
        semantic_pass_one_destroy(pass_one);
        return NULL;
    }

    return pass_one;
}

/*
 * semantic_analyze_instruction: 分析单条指令
 */
int semantic_analyze_instruction(
    PassOne* pass_one,
    const Token* tokens,
    u32 token_index,
    InstructionEntry* out_entry
) {
    u32 i = token_index;
    u32 tokens_consumed = 0;

    out_entry->operand_count = 0;
    out_entry->has_label = 0;
    util_memset(out_entry->mnemonic, 0, sizeof(out_entry->mnemonic));
    util_memset(out_entry->label, 0, sizeof(out_entry->label));

    /* 检查是否有标签前缀 (标签: 指令) */
    if (tokens[i].type == TOK_IDENTIFIER && i + 1 < 65535 && tokens[i+1].type == TOK_COLON) {
        out_entry->has_label = 1;
        util_strcpy(out_entry->label, (const char*)tokens[i].lexeme);
        i += 2;
        tokens_consumed = 2;

        /* 标签后面可能直接是 NEWLINE，这种情况下只有标签，没有指令 */
        if (i >= 65535 || tokens[i].type == TOK_NEWLINE || tokens[i].type == TOK_EOF) {
            /* 创建一个虚拟"NOP"指令来保持标签地址 */
            util_strcpy(out_entry->mnemonic, "NOP");
            out_entry->operand_count = 0;
            return tokens_consumed;
        }
    }

    /* 读取助记符 */
    if (i >= 65535 || tokens[i].type != TOK_IDENTIFIER) {
        return -1;
    }

    /* 支持格式：label PROC  或 label ENDP （标签后直接跟助记符而非冒号）
       如果遇到 IDENT IDENT 且第二个 IDENT 是已知助记符，则第一个为标签 */
    if (i + 1 < 65535 && tokens[i+1].type == TOK_IDENTIFIER) {
        const InstructionInfo* info = tables_lookup_instruction((const char*)tokens[i+1].lexeme);
        /* 如果第二个标识符是已知伪指令：
           - 若为 PROC：第一个为标签定义（label PROC）
           - 若为 ENDP/END：将第一个作为操作数，第二个为助记符（如 "main ENDP"）
        */
        if (info != NULL) {
            if (info->type == PSEUDO_PROC) {
                out_entry->has_label = 1;
                util_strcpy(out_entry->label, (const char*)tokens[i].lexeme);
                util_strcpy(out_entry->mnemonic, (const char*)tokens[i+1].lexeme);
                i += 2;
                tokens_consumed += 2;
            } else if (info->type == PSEUDO_DB) {
                /* 形如: label DB ... —— 将前置标识符视为标签定义 */
                out_entry->has_label = 1;
                util_strcpy(out_entry->label, (const char*)tokens[i].lexeme);
                util_strcpy(out_entry->mnemonic, (const char*)tokens[i+1].lexeme);
                i += 2;
                tokens_consumed += 2;
            } else {
                /* 将第一个标识符作为操作数（标签名），第二个为助记符 */
                util_strcpy(out_entry->mnemonic, (const char*)tokens[i+1].lexeme);
                /* 填充一个标签型操作数 */
                out_entry->operands[0].type = OPERAND_LABEL;
                util_memset(out_entry->operands[0].name, 0, sizeof(out_entry->operands[0].name));
                util_strcpy(out_entry->operands[0].name, (const char*)tokens[i].lexeme);
                out_entry->operand_count = 1;
                i += 2;
                tokens_consumed += 2;
            }
        } else {
            util_strcpy(out_entry->mnemonic, (const char*)tokens[i].lexeme);
            i++;
            tokens_consumed++;
        }
    } else {
        util_strcpy(out_entry->mnemonic, (const char*)tokens[i].lexeme);
        i++;
        tokens_consumed++;
    }

    /* 解析操作数 */
    while (i < 65535 && tokens[i].type != TOK_NEWLINE && tokens[i].type != TOK_EOF
           && out_entry->operand_count < SEMANTIC_MAX_OPERANDS) {

        Operand* operand = &out_entry->operands[out_entry->operand_count];
        operand->type = OPERAND_NONE;
        operand->value = 0;
        util_memset(operand->name, 0, sizeof(operand->name));

        /* 按 Token 类型确定操作数类型 */
        if (tokens[i].type == TOK_IDENTIFIER) {
            if (is_register((const char*)tokens[i].lexeme)) {
                operand->type = OPERAND_REGISTER;
            } else {
                operand->type = OPERAND_LABEL;
                util_strcpy(operand->name, (const char*)tokens[i].lexeme);
            }
        } else if (tokens[i].type == TOK_NUMBER) {
            operand->type = OPERAND_IMMEDIATE;
            operand->value = tokens[i].int_value;
        } else if (tokens[i].type == TOK_LBRACKET) {
            /* 内存寻址模式 [address] */
            operand->type = OPERAND_MEMORY;
            i++;
            tokens_consumed++;
            if (tokens[i].type == TOK_NUMBER) {
                operand->value = tokens[i].int_value;
            } else if (tokens[i].type == TOK_IDENTIFIER) {
                util_strcpy(operand->name, (const char*)tokens[i].lexeme);
            }
            i++;
            tokens_consumed++;
            if (tokens[i].type == TOK_RBRACKET) {
                i++;
                tokens_consumed++;
            }
            out_entry->operand_count++;
            if (i < 65535 && tokens[i].type == TOK_COMMA) {
                i++;
                tokens_consumed++;
            }
            continue;
        } else {
            /* 非操作数 Token，结束操作数解析 */
            break;
        }

        out_entry->operand_count++;
        i++;
        tokens_consumed++;

        /* 处理类似 CS:CODE 的语法（伪指令 ASSUME 使用） */
        if (i < 65535 && tokens[i].type == TOK_COLON && i + 1 < 65535 && tokens[i+1].type == TOK_IDENTIFIER) {
            /* 将冒号和后续标识符并入当前操作数名称，例如 "CS:CODE" */
            char tmp[128];
            util_memset(tmp, 0, sizeof(tmp));
            util_strcpy(tmp, operand->name);
            /* 追加 ':' */
            tmp[util_strlen(tmp)] = ':';
            tmp[util_strlen(tmp) + 1] = '\0';
            util_strcpy(tmp + util_strlen(tmp), (const char*)tokens[i+1].lexeme);
            util_strcpy(operand->name, tmp);

            /* 消耗 ':' 和后续标识符 */
            i += 2;
            tokens_consumed += 2;
        }

        /* 检查是否有逗号分隔的下一个操作数 */
        if (i < 65535 && tokens[i].type == TOK_COMMA) {
            i++;
            tokens_consumed++;
        } else {
            break;
        }
    }

    return tokens_consumed;
}

/*
 * semantic_get_instruction_length: 计算指令长度
 */
u32 semantic_get_instruction_length(
    const s8* mnemonic,
    const OperandType* operand_types,
    u32 operand_count
) {
    (void)operand_types;  /* 当前简化实现 */
    (void)operand_count;

    /* 根据 MASM 指令编码规则预估长度 */
    if (util_strcmp((const char*)mnemonic, "DB") == 0) return 1;
    if (util_strcmp((const char*)mnemonic, "ORG") == 0) return 0;
    if (util_strcmp((const char*)mnemonic, "SEGMENT") == 0) return 0;
    if (util_strcmp((const char*)mnemonic, "ENDS") == 0) return 0;

    return 3;  /* 默认 3 字节 */
}

/*
 * semantic_validate_operand: 验证操作数合法性
 */
int semantic_validate_operand(
    const s8* mnemonic,
    u32 position,
    const Operand* operand
) {
    /* 简化验证：主要检查立即数不能作为第一操作数等 */
    if (util_strcmp((const char*)mnemonic, "MOV") == 0 && position == 0) {
        /* MOV 的目标不能是立即数 */
        if (operand->type == OPERAND_IMMEDIATE) {
            return -1;
        }
    }

    return 0;
}

/*
 * semantic_pass_one_destroy: 销毁第一遍扫描上下文
 */
void semantic_pass_one_destroy(PassOne* pass_one) {
    if (pass_one == NULL) return;

    if (pass_one->symtab != NULL) {
        symtab_destroy(pass_one->symtab);
    }

    if (pass_one->instructions != NULL) {
        util_free(pass_one->instructions);
    }

    util_free(pass_one);
}


