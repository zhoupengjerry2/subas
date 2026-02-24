/*
 * ============================================================================
 * 文件名: codegen.c
 * 描述  : 代码生成模块实现 - 第二遍扫描
 *
 * 关键算法：
 *  1. 遍历第一遍扫描的指令列表
 *  2. 对每条指令生成机器码
 *  3. 记录标签引用的重定位信息
 *  4. 在代码生成完毕后，利用符号表解决所有标签引用
 *
 * ============================================================================
 */

#include <stdio.h>
#include "../include/codegen.h"

/* ========================================================================= */
/* 内部辅助函数声明 */
/* ========================================================================= */

/*
 * 记录重定位信息
 */
static int record_relocation(
    CodeGen* codegen,
    u32 offset,
    u32 instruction_index,
    u32 operand_index,
    const s8* symbol_name
) {
    if (codegen->relocation_count >= CODEGEN_MAX_RELOCATIONS) {
        error_report(0, ERR_SYS_OUT_OF_MEM, "重定位记录超过限制");
        return -1;
    }
    
    Relocation* rel = &codegen->relocations[codegen->relocation_count];
    rel->offset = offset;
    rel->instruction_index = instruction_index;
    rel->operand_index = operand_index;
    util_strcpy(rel->symbol_name, symbol_name);
    
    codegen->relocation_count++;
    return 0;
}

/* ========================================================================= */
/* API 函数实现 */
/* ========================================================================= */

/*
 * codegen_pass_two: 执行第二遍扫描（代码生成）
 */
CodeGen* codegen_pass_two(const PassOne* pass_one) {
    if (pass_one == NULL) {
        error_report(0, ERR_SYS_OUT_OF_MEM, "Pass One 为 NULL");
        return NULL;
    }
    
    CodeGen* codegen = (CodeGen*)util_malloc(sizeof(CodeGen));
    if (codegen == NULL) {
        error_report(0, ERR_SYS_OUT_OF_MEM, "无法分配 CodeGen 结构");
        return NULL;
    }
    
    codegen->code_buffer = (u8*)util_malloc(CODEGEN_OUTPUT_BUFFER_SIZE);
    if (codegen->code_buffer == NULL) {
        util_free(codegen);
        error_report(0, ERR_SYS_OUT_OF_MEM, "无法分配代码缓冲区");
        return NULL;
    }
    
    codegen->relocations = (Relocation*)util_malloc(
        sizeof(Relocation) * CODEGEN_MAX_RELOCATIONS
    );
    if (codegen->relocations == NULL) {
        util_free(codegen->code_buffer);
        util_free(codegen);
        error_report(0, ERR_SYS_OUT_OF_MEM, "无法分配重定位表");
        return NULL;
    }
    
    codegen->pass_one = pass_one;
    codegen->code_size = 0;
    codegen->relocation_count = 0;
    codegen->has_errors = 0;
    
    /* 遍历第一遍收集的指令，生成代码 */
    for (u32 i = 0; i < pass_one->instruction_count; i++) {
        const InstructionEntry* entry = &pass_one->instructions[i];
        
        if (codegen_emit_instruction(codegen, entry) < 0) {
            codegen->has_errors = 1;
            error_report(entry->line, ERR_PARSE_UNK_MNEMONIC, "未知指令");
        }
    }
    
    /* 解决所有标签引用 */
    if (codegen_resolve_reference(codegen) < 0) {
        codegen->has_errors = 1;
    }
    
    if (codegen->has_errors) {
        codegen_destroy(codegen);
        return NULL;
    }
    
    return codegen;
}

/*
 * codegen_emit_instruction: 生成单条指令的机器码
 */
int codegen_emit_instruction(CodeGen* codegen, const InstructionEntry* entry) {
    if (codegen->code_size + SEMANTIC_MAX_INSTRUCTION_LEN >= CODEGEN_OUTPUT_BUFFER_SIZE) {
        error_report(0, ERR_SYS_OUT_OF_MEM, "代码缓冲区溢出");
        return -1;
    }
    
    const InstructionInfo* instr_info = tables_lookup_instruction((const char*)entry->mnemonic);
    if (instr_info == NULL) {
        error_report(entry->line, ERR_PARSE_UNK_MNEMONIC, "未知指令");
        return -1;
    }
    
    u8* code = codegen->code_buffer + codegen->code_size;
    u32 emitted = 0;
    
    /* 简化代码生成：根据指令类型生成对应的操作码序列 */
    
    if (instr_info->is_pseudo) {
        /* 伪指令处理 */
        if (util_strcmp((const char*)entry->mnemonic, "DB") == 0) {
            /* 数据定义 */
            if (entry->operand_count > 0 && entry->operands[0].type == OPERAND_IMMEDIATE) {
                code[emitted++] = (u8)(entry->operands[0].value & 0xFF);
            }
        }
        /* 其他伪指令在第一遍已处理，此处跳过 */
    } else {
        /* 常规指令处理 */
        code[emitted++] = instr_info->opcode;  /* 操作码 */
        
        /* 处理操作数 */
        for (u32 i = 0; i < entry->operand_count; i++) {
            const Operand* operand = &entry->operands[i];
            
            if (operand->type == OPERAND_IMMEDIATE) {
                /* 立即数寻址 */
                if (operand->value <= 0xFF) {
                    code[emitted++] = (u8)(operand->value & 0xFF);
                } else {
                    code[emitted++] = (u8)(operand->value & 0xFF);
                    code[emitted++] = (u8)((operand->value >> 8) & 0xFF);
                }
            } else if (operand->type == OPERAND_REGISTER) {
                /* 寄存器寻址 - 简化为寄存器编号 */
                code[emitted++] = 0xC0 | (u8)(i & 0x3F);
            } else if (operand->type == OPERAND_LABEL || operand->type == OPERAND_MEMORY) {
                /* 标签引用 - 记录重定位信息，暂时填充 0 */
                if (operand->type == OPERAND_LABEL && util_strlen((const char*)operand->name) > 0) {
                    if (record_relocation(codegen, codegen->code_size + emitted, 
                                        entry - codegen->pass_one->instructions, i,
                                        operand->name) < 0) {
                        return -1;
                    }
                }
                code[emitted++] = 0x00;
                code[emitted++] = 0x00;
            }
        }
    }
    
    codegen->code_size += emitted;
    return 0;
}

/*
 * codegen_resolve_reference: 解决所有标签引用
 */
int codegen_resolve_reference(CodeGen* codegen) {
    for (u32 i = 0; i < codegen->relocation_count; i++) {
        const Relocation* rel = &codegen->relocations[i];
        
        /* 从符号表查找符号地址 */
        SymbolInfo* symbol = symtab_lookup(codegen->pass_one->symtab, (const char*)rel->symbol_name);
        if (symbol == NULL) {
            error_report(0, ERR_PARSE_UNDEFINED_LBL, "未定义的标签或符号");
            return -1;
        }
        
        if (!symbol->is_defined) {
            error_report(0, ERR_PARSE_UNDEFINED_LBL, "标签未定义");
            return -1;
        }
        
        /* 填充地址 */
        u32 address = symbol->address;
        codegen->code_buffer[rel->offset] = (u8)(address & 0xFF);
        codegen->code_buffer[rel->offset + 1] = (u8)((address >> 8) & 0xFF);
    }
    
    return 0;
}

/*
 * codegen_get_code_buffer: 获取生成的代码缓冲区
 */
u8* codegen_get_code_buffer(const CodeGen* codegen, u32* out_size) {
    if (codegen == NULL) {
        *out_size = 0;
        return NULL;
    }
    
    *out_size = codegen->code_size;
    return codegen->code_buffer;
}

/*
 * codegen_get_relocation_info: 获取重定位信息
 */
Relocation* codegen_get_relocation_info(const CodeGen* codegen, u32* out_count) {
    if (codegen == NULL) {
        *out_count = 0;
        return NULL;
    }
    
    *out_count = codegen->relocation_count;
    return codegen->relocations;
}

/*
 * codegen_destroy: 销毁代码生成上下文
 */
void codegen_destroy(CodeGen* codegen) {
    if (codegen == NULL) return;
    
    if (codegen->code_buffer != NULL) {
        util_free(codegen->code_buffer);
    }
    
    if (codegen->relocations != NULL) {
        util_free(codegen->relocations);
    }
    
    util_free(codegen);
}
