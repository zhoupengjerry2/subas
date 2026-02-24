/*
 * ============================================================================
 * 文件名: main.c
 * 描述  : SUBAS 汇编器主程序
 *
 * 功能：
 *  - 解析命令行参数
 *  - 读取源文件
 *  - 初始化各个模块
 *  - 按顺序调用 lexer → semantic → codegen
 *  - 生成输出文件或二进制代码
 *  - 清理资源并报告编译结果
 *
 * 使用方法：
 *   subas [-o OUTPUT] [-v] INPUT_FILE
 *
 * 参数：
 *   INPUT_FILE   : 源代码文件（.asm）
 *   -o OUTPUT    : 输出文件路径（默认为 input.com）
 *   -v          : 详细模式，打印中间结果
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "../include/lexer.h"
#include "../include/semantic.h"
#include "../include/codegen.h"
#include "../include/tables.h"
#include "../include/error.h"
#include "../include/utils.h"

/* ========================================================================= */
/* 常量定义 */
/* ========================================================================= */

#define MAX_SOURCE_SIZE     (64 * 1024)    /* 最大源文件大小：64KB */
#define MAX_TOKENS          (4096)         /* 最大 Token 数 */
#define SUBAS_VERSION       "0.1.0"
#define DEFAULT_OUTPUT_EXT  ".com"

/* ========================================================================= */
/* 类型定义 */
/* ========================================================================= */

/*
 * 命令行参数结构
 */
typedef struct {
    char* input_file;           /* 输入源文件路径 */
    char* output_file;          /* 输出文件路径 */
    int verbose;                /* 详细模式标志 */
    int help;                   /* 显示帮助标志 */
} CommandLine;

/* ========================================================================= */
/* 内部函数声明 */
/* ========================================================================= */

/*
 * 打印使用说明
 */
static void print_usage(const char* program_name);

/*
 * 打印版本信息
 */
static void print_version(void);

/*
 * 解析命令行参数
 */
static int parse_command_line(int argc, char* argv[], CommandLine* cmd);

/*
 * 读取源文件内容
 */
static char* read_source_file(const char* filename);

/*
 * 生成输出文件名
 */
static char* generate_output_filename(const char* input_file);

/*
 * 写入二进制文件
 */
static int write_output_file(const char* filename, const u8* code, u32 size);

/*
 * 打印编译统计信息
 */
static void print_statistics(
    const PassOne* pass_one,
    const CodeGen* codegen,
    int times_ms
);

/* ========================================================================= */
/* 实现 */
/* ========================================================================= */

static void print_usage(const char* program_name) {
    printf("SUBAS v%s - 16-bit MASM 3.0 Subset Assembler\n\n", SUBAS_VERSION);
    printf("Usage: %s [options] INPUT_FILE\n\n", program_name);
    printf("Options:\n");
    printf("  -o FILE     Output file path (default: input.com)\n");
    printf("  -v          Verbose mode (print intermediate results)\n");
    printf("  -h, --help  Show this help message\n");
    printf("  --version   Show version information\n");
    printf("\nExample:\n");
    printf("  %s program.asm              (Generate program.com)\n", program_name);
    printf("  %s -o out.bin program.asm   (Generate out.bin)\n", program_name);
}

static void print_version(void) {
    printf("SUBAS v%s\n", SUBAS_VERSION);
    printf("16-bit MASM 3.0 Subset Assembler\n");
    printf("Built for x86 real mode (16-bit) assembly\n");
}

static int parse_command_line(int argc, char* argv[], CommandLine* cmd) {
    int i;

    /* 初始化命令行结构 */
    cmd->input_file = NULL_PTR;
    cmd->output_file = NULL_PTR;
    cmd->verbose = 0;
    cmd->help = 0;

    /* 查找选项和输入文件 */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            /* 选项处理 */
            if (util_strcmp(argv[i], "-o") == 0) {
                /* -o 输出文件 */
                if (i + 1 >= argc) {
                    printf("Error: -o requires an argument\n");
                    return -1;
                }
                cmd->output_file = argv[++i];
            } else if (util_strcmp(argv[i], "-v") == 0) {
                /* 详细模式 */
                cmd->verbose = 1;
            } else if (util_strcmp(argv[i], "-h") == 0 ||
                       util_strcmp(argv[i], "--help") == 0) {
                cmd->help = 1;
            } else if (util_strcmp(argv[i], "--version") == 0) {
                print_version();
                return 0;
            } else {
                printf("Error: Unknown option '%s'\n", argv[i]);
                return -1;
            }
        } else {
            /* 非选项参数视为输入文件 */
            if (cmd->input_file != NULL_PTR) {
                printf("Error: Multiple input files specified\n");
                return -1;
            }
            cmd->input_file = argv[i];
        }
    }

    return 0;
}

static char* read_source_file(const char* filename) {
    FILE* fp;
    char* buffer;
    u32 size;
    u32 bytes_read;

    if (filename == NULL_PTR) {
        error_report(0, ERR_SYS_FILE_IO, "No input file specified");
        return NULL_PTR;
    }

    /* 打开文件 */
    fp = fopen(filename, "rb");
    if (fp == NULL_PTR) {
        error_report(0, ERR_SYS_FILE_IO, "Cannot open input file");
        return NULL_PTR;
    }

    /* 获取文件大小 */
    fseek(fp, 0, SEEK_END);
    size = (u32)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size > MAX_SOURCE_SIZE) {
        error_report(0, ERR_SYS_FILE_IO, "Input file too large");
        fclose(fp);
        return NULL_PTR;
    }

    /* 分配缓冲区 */
    buffer = (char*)util_malloc(size + 1);
    if (buffer == NULL_PTR) {
        error_report(0, ERR_SYS_OUT_OF_MEM, "Cannot allocate buffer");
        fclose(fp);
        return NULL_PTR;
    }

    /* 读取文件 */
    bytes_read = (u32)fread(buffer, 1, size, fp);
    if (bytes_read != size) {
        error_report(0, ERR_SYS_FILE_IO, "File read error");
        util_free(buffer);
        fclose(fp);
        return NULL_PTR;
    }

    buffer[size] = '\0';
    fclose(fp);

    return buffer;
}

static char* generate_output_filename(const char* input_file) {
    char* output;
    u32 len;
    u32 i;

    if (input_file == NULL_PTR) {
        return util_strdup("output.com");
    }

    len = util_strlen(input_file);
    output = (char*)util_malloc(len + 10);
    if (output == NULL_PTR) {
        return util_strdup("output.com");
    }

    util_strcpy(output, input_file);

    /* 替换扩展名为 .com */
    for (i = len; i > 0; i--) {
        if (output[i - 1] == '.') {
            output[i] = 'c';
            output[i + 1] = 'o';
            output[i + 2] = 'm';
            output[i + 3] = '\0';
            return output;
        }
    }

    /* 没有扩展名，直接附加 */
    output[len] = '.';
    output[len + 1] = 'c';
    output[len + 2] = 'o';
    output[len + 3] = 'm';
    output[len + 4] = '\0';

    return output;
}

static int write_output_file(const char* filename, const u8* code, u32 size) {
    FILE* fp;
    u32 bytes_written;

    if (filename == NULL_PTR || code == NULL_PTR || size == 0) {
        error_report(0, ERR_SYS_FILE_IO, "Invalid output parameters");
        return -1;
    }

    fp = fopen(filename, "wb");
    if (fp == NULL_PTR) {
        error_report(0, ERR_SYS_FILE_IO, "Cannot create output file");
        return -1;
    }

    bytes_written = (u32)fwrite(code, 1, size, fp);
    if (bytes_written != size) {
        error_report(0, ERR_SYS_FILE_IO, "File write error");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

static void print_statistics(
    const PassOne* pass_one,
    const CodeGen* codegen,
    int times_ms
) {
    u32 code_size = 0;
    u32 reloc_count = 0;

    if (pass_one != NULL_PTR) {
        printf("  Instructions: %u\n", pass_one->instruction_count);
        printf("  Symbol count: %u\n", symtab_get_symbol_count(pass_one->symtab));
    }

    if (codegen != NULL_PTR) {
        (void)codegen_get_code_buffer(codegen, &code_size);
        (void)codegen_get_relocation_info(codegen, &reloc_count);
        printf("  Code size: %u bytes\n", code_size);
        printf("  Relocations: %u\n", reloc_count);
    }

    printf("  Compilation time: %d ms\n", times_ms);
}

/* ========================================================================= */
/* 主程序入口 */
/* ========================================================================= */

int main(int argc, char* argv[]) {
    CommandLine cmdline;
    char* source;
    Lexer* lexer;
    Token* tokens;
    u32 token_count;
    u32 token_capacity;
    int has_eof;
    PassOne* pass_one;
    CodeGen* codegen;
    char* output_file;
    u8* code_buffer;
    u32 code_size;
    int error_count;

    printf("========================================\n");
    printf("  SUBAS v%s - Assembler\n", SUBAS_VERSION);
    printf("========================================\n\n");

    /* 解析命令行参数 */
    if (parse_command_line(argc, argv, &cmdline) != 0) {
        print_usage(argv[0]);
        return 1;
    }

    if (cmdline.help) {
        print_usage(argv[0]);
        return 0;
    }

    if (cmdline.input_file == NULL_PTR) {
        printf("Error: No input file specified\n\n");
        print_usage(argv[0]);
        return 1;
    }

    /* 初始化错误系统 */
    error_init();

    if (cmdline.verbose) {
        printf("Configuration:\n");
        printf("  Input file: %s\n", cmdline.input_file);
        printf("  Output file: %s\n", cmdline.output_file != NULL_PTR ?
               cmdline.output_file : "(auto-generated)");
        printf("  Verbose mode: ON\n\n");
    }

    /* ===== 第 0 步：读取源文件 ===== */
    printf("Step 0: Reading source file...\n");
    source = read_source_file(cmdline.input_file);
    if (source == NULL_PTR) {
        printf("Compilation failed!\n");
        return 1;
    }

    if (cmdline.verbose) {
        printf("  Source file size: %u bytes\n\n", (u32)util_strlen(source));
    }

    /* ===== 第 1 步：初始化表驱动系统 ===== */
    printf("Step 1: Initializing tables...\n");
    tables_init();
    if (cmdline.verbose) {
        printf("  Instructions loaded: %u\n\n", tables_get_instruction_count());
    }

    /* ===== 第 2 步：词法分析 (Lexing) ===== */
    printf("Step 2: Lexical analysis (Lexing)...\n");
    lexer = lexer_create_from_string(source);
    if (lexer == NULL_PTR) {
        printf("ERROR: Cannot create lexer\n");
        printf("Compilation failed!\n");
        util_free(source);
        return 1;
    }

    /* 收集所有 Token */
    token_capacity = MAX_TOKENS;
    tokens = (Token*)util_malloc(sizeof(Token) * token_capacity);
    if (tokens == NULL_PTR) {
        error_report(0, ERR_SYS_OUT_OF_MEM, "Cannot allocate token buffer");
        printf("Compilation failed!\n");
        lexer_destroy(lexer);
        util_free(source);
        return 1;
    }

    token_count = 0;
    has_eof = 0;
    while (!has_eof) {
        Token tok = lexer_next_token(lexer);

        if (token_count >= token_capacity) {
            printf("ERROR: Too many tokens\n");
            printf("Compilation failed!\n");
            util_free(tokens);
            lexer_destroy(lexer);
            util_free(source);
            return 1;
        }

        tokens[token_count] = tok;

        if (tok.type == TOK_EOF) {
            has_eof = 1;
        }

        token_count++;
    }

    printf("  Tokens: %u\n", token_count);
    error_count = error_get_count();
    if (error_count > 0) {
        printf("Lexical errors detected! (%d)\n", error_count);
        printf("Compilation failed!\n");
        util_free(tokens);
        util_free(source);
        return 1;
    }
    lexer_destroy(lexer);

    /* ===== 第 3 步：语义分析 (Pass 1) ===== */
    printf("Step 3: Semantic analysis (Pass 1)...\n");
    pass_one = semantic_pass_one(tokens, token_count);

    if (pass_one == NULL_PTR) {
        printf("ERROR: Semantic analysis failed (pass_one is NULL)\n");
        printf("Compilation failed!\n");
        util_free(tokens);
        util_free(source);
        return 1;
    }

    printf("  Instructions: %u\n", pass_one->instruction_count);
    printf("  Code size: 0x%04X\n", pass_one->current_address);
    printf("  Symbols: %u\n", symtab_get_symbol_count(pass_one->symtab));

    error_count = error_get_count();
    if (error_count > 0) {
        printf("Semantic errors detected! (%d)\n", error_count);
        printf("Compilation failed!\n");
        semantic_pass_one_destroy(pass_one);
        util_free(tokens);
        util_free(source);
        return 1;
    }

    if (cmdline.verbose) {
        printf("\n");
    }

    /* ===== 第 4 步：代码生成 (Pass 2) ===== */
    printf("Step 4: Code generation (Pass 2)...\n");
    codegen = codegen_pass_two(pass_one);
    if (codegen == NULL_PTR) {
        printf("ERROR: Code generation failed\n");
        printf("Compilation failed!\n");
        semantic_pass_one_destroy(pass_one);
        util_free(tokens);
        util_free(source);
        return 1;
    }

    code_buffer = codegen_get_code_buffer(codegen, &code_size);
    printf("  Generated code size: %u bytes\n", code_size);

    error_count = error_get_count();
    if (error_count > 0) {
        printf("Code generation errors detected! (%d)\n", error_count);
        printf("Compilation failed!\n");
        codegen_destroy(codegen);
        semantic_pass_one_destroy(pass_one);
        util_free(tokens);
        util_free(source);
        return 1;
    }

    if (cmdline.verbose) {
        printf("\n");
    }

    /* ===== 第 5 步：输出文件生成 ===== */
    printf("Step 5: Output file generation...\n");

    if (cmdline.output_file == NULL_PTR) {
        output_file = generate_output_filename(cmdline.input_file);
    } else {
        output_file = cmdline.output_file;
    }

    if (write_output_file(output_file, code_buffer, code_size) != 0) {
        printf("ERROR: Cannot write output file\n");
        printf("Compilation failed!\n");
        if (cmdline.output_file == NULL_PTR) {
            util_free(output_file);
        }
        codegen_destroy(codegen);
        semantic_pass_one_destroy(pass_one);
        util_free(tokens);
        util_free(source);
        return 1;
    }

    printf("  Output file: %s (%u bytes)\n", output_file, code_size);

    /* ===== 清理资源 ===== */
    printf("\nStep 6: Cleanup...\n");
    codegen_destroy(codegen);
    semantic_pass_one_destroy(pass_one);

    /* 释放 Token 的 lexeme */
    for (u32 i = 0; i < token_count; i++) {
        if (tokens[i].lexeme != NULL_PTR) {
            util_free(tokens[i].lexeme);
        }
    }
    util_free(tokens);
    util_free(source);

    if (cmdline.output_file == NULL_PTR) {
        util_free(output_file);
    }

    /* ===== 编译完成 ===== */
    error_count = error_get_count();

    printf("\n========================================\n");
    printf("COMPILATION COMPLETE\n");
    printf("========================================\n");
    printf("Errors: %d\n", error_count);

    if (error_count == 0) {
        printf("Status: SUCCESS ✓\n");
        printf("\nOutput file '%s' generated successfully!\n", output_file);
        printf("========================================\n");
        return 0;
    } else {
        printf("Status: FAILED ✗\n");
        printf("========================================\n");
        return 1;
    }
}


