# ============================================================================
# Makefile for SUBAS - 16-bit MASM 3.0 Subset Assembler
# ============================================================================

# 编译器和选项
CC = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LDFLAGS = 

# 源文件
SRCS = src/main.c \
       src/lexer.c \
       src/semantic.c \
       src/codegen.c \
       src/tables.c \
       src/symtab.c \
       src/utils/memory.c \
       src/utils/string.c \
       src/utils/hash.c \
       src/error.c

# 单元测试源文件
TEST_SOURCES = tests/test_lexer.c \
               tests/test_utils_error.c \
               tests/test_tables_symtab.c \
               tests/test_semantic_codegen.c

# 目标输出
TARGET = subas
TESTS_DIR = tests

# 默认目标
.PHONY: all clean test help

all: $(TARGET)

# 编译主程序
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Build successful: ./$(TARGET)"

# 运行所有测试
test: test-utils-error test-lexer test-tables-symtab test-semantic-codegen
	@echo "✓ All tests completed"

# 测试 Utils 和 Error 模块
test-utils-error:
	@echo "Running utils/error tests..."
	$(CC) $(CFLAGS) -o $(TESTS_DIR)/test_utils_error \
		$(TESTS_DIR)/test_utils_error.c \
		src/utils/memory.c src/utils/string.c src/utils/hash.c src/error.c
	@./$(TESTS_DIR)/test_utils_error

# 测试 Lexer 模块
test-lexer:
	@echo "Running lexer tests..."
	$(CC) $(CFLAGS) -o $(TESTS_DIR)/test_lexer \
		$(TESTS_DIR)/test_lexer.c \
		src/lexer.c src/utils/memory.c src/utils/string.c src/utils/hash.c src/error.c
	@./$(TESTS_DIR)/test_lexer

# 测试 Tables 和 Symtab 模块
test-tables-symtab:
	@echo "Running tables/symtab tests..."
	$(CC) $(CFLAGS) -o $(TESTS_DIR)/test_tables_symtab \
		$(TESTS_DIR)/test_tables_symtab.c \
		src/tables.c src/symtab.c src/utils/memory.c src/utils/string.c \
		src/utils/hash.c src/error.c
	@./$(TESTS_DIR)/test_tables_symtab

# 测试 Semantic 和 CodeGen 模块
test-semantic-codegen:
	@echo "Running semantic/codegen tests..."
	$(CC) $(CFLAGS) -o $(TESTS_DIR)/test_semantic_codegen \
		$(TESTS_DIR)/test_semantic_codegen.c \
		src/semantic.c src/codegen.c src/tables.c src/symtab.c \
		src/lexer.c src/utils/memory.c src/utils/string.c \
		src/utils/hash.c src/error.c
	@./$(TESTS_DIR)/test_semantic_codegen

# 清理生成的文件
clean:
	@rm -f $(TARGET)
	@rm -f $(TESTS_DIR)/test_utils_error
	@rm -f $(TESTS_DIR)/test_lexer
	@rm -f $(TESTS_DIR)/test_tables_symtab
	@rm -f $(TESTS_DIR)/test_semantic_codegen
	@rm -f *.o *.com *.bin
	@echo "✓ Cleaned up"

# 显示帮助
help:
	@echo "SUBAS Makefile targets:"
	@echo "  make              Build the assembler (default)"
	@echo "  make test         Run all unit tests"
	@echo "  make test-*       Run specific test (utils-error, lexer, tables-symtab, semantic-codegen)"
	@echo "  make clean        Remove all generated files"
	@echo "  make help         Show this help message"
	@echo ""
	@echo "Usage: ./$(TARGET) [options] INPUT_FILE"
	@echo "  -o FILE         Output file (default: input.com)"
	@echo "  -v              Verbose mode"
	@echo "  -h, --help      Show help"
	@echo "  --version       Show version"
