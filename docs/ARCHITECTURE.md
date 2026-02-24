**SUBAS 架构与实现说明**

概览：
- 项目名称：SUBAS
- 目标：实现一个教育/实验用途的 16-bit MASM 3.0 子集汇编器，支持两遍（Pass 1/Pass 2）汇编、伪指令（SEGMENT/ENDS/ASSUME/ORG/DB/PROC/ENDP/END）、基础指令集（MOV/ADD/SUB/...）以及可扩展的表驱动指令定义。
- 语言与平台：C (C90 风格)，可用 GCC 在 Windows/Linux 下构建。

目的：
- 为开发者提供完整的设计文档，便于维护、扩展指令集、重写汇编器或实现自举（用 SUBAS 自编译自身）。

设计原则：
- 简单清晰：优先实现明确的、可验证的功能；避免过度复杂的表达式解析或宏系统。
- 表驱动：使用指令/伪指令表（InstructionInfo）驱动解析与生成，便于新增指令。
- 两遍编译：第一遍（Pass 1）做词法/语义收集与地址估算，第二遍（Pass 2）做代码生成与重定位解决。
- 模块化：词法、语义、代码生成、符号表、错误处理各自独立。

总体架构（模块划分）：
- `lexer`：把源文本分解成 `Token` 流（类型：IDENTIFIER, NUMBER, COLON, COMMA, LBRACKET, RBRACKET, NEWLINE, EOF 等）。
- `tables`：保存 `InstructionInfo` 表（助记符、类型、opcode、operand_count、is_pseudo），以及伪指令定义。
- `symtab`（符号表）：保存标签/符号的定义位置、是否已定义、行号等信息，提供查找/插入/遍历接口。
- `semantic`：Pass 1 的核心；从 Token 流解析单条“指令条目”（`InstructionEntry`），处理标签定义、伪指令（SEGMENT/DB/ORG 等）并估算指令长度，生成 `PassOne` 上下文。
- `codegen`：Pass 2；遍历 `PassOne.instructions`，调用基于 `InstructionInfo` 的生成器把指令转为字节序列，记录重定位（`Relocation`）并在后期解决。
- `error`：统一错误/诊断接口（错误码、行号、错误计数），保证可聚合输出并影响构建结果。
- `utils`：字符串、内存、哈希表、通用工具函数。
- `main`：CLI、流程驱动（读取文件 → tables_init → lexing → semantic_pass_one → codegen_pass_two → 写文件）。

主要数据结构细节：
- Token
  - type: TokenType
  - lexeme: 字符串（分配的拷贝）
  - int_value: 解析后的数值（当 type == TOK_NUMBER）
  - line: 源行号

- InstructionInfo (tables.h)
  - mnemonic: const char*
  - type: InstructionType (INSTR_MOV, PSEUDO_DB, ...)
  - opcode: u8（codegen 使用）
  - operand_count: u8
  - is_pseudo: int

- Operand / InstructionEntry (semantic.h)
  - Operand.type: OPERAND_REGISTER/IMMEDIATE/MEMORY/LABEL
  - Operand.value: 立即数或寄存器编号
  - Operand.name: 符号或标识名
  - InstructionEntry: address, length, line, mnemonic, operands[], operand_count, has_label, label

- PassOne
  - symtab, instructions (InstructionEntry*), instruction_count, current_address, current_line, has_errors

- CodeGen / Relocation
  - code_buffer, code_size, relocations[] (offset, instruction_index, operand_index, symbol_name)

关键实现要点与设计说明：
- 两遍实现的优点：第一遍可自由估算长度并收集符号，不强制即时求值，第二遍专注生成与重定位。缺点是需正确估算指令长度与伪指令处理边界。
- 表驱动解析：`tables_lookup_instruction()` 在语义中用于确定某个标识符是否为助记符/伪指令，从而判断前后标识符的语义（如 `label PROC`、`label DB`）。这避免了硬编码的巨型 switch-case，同时便于扩展。
- 伪指令处理：在 Pass 1 里处理伪指令（SEGMENT/ENDS/ASSUME/ORG/DB/PROC/ENDP/END），例如：
  - `SEGMENT` / `ENDS`：用于逻辑分段；Pass 1 需要跟踪当前段以便后续地址分配；目前实现把段语义简化为占位，仅保证 DB 等数据进入 DATA 段。
  - `ORG`：改变当前地址偏移用于 .com 文件起始偏移设定。
  - `DB`：在 Pass 1 中被解析为具有 N 个立即数操作数的伪指令，估算长度为立即数个数，Pass 2 将输出相应字节序列。
- 标签解析边界：支持多种写法：`label: MOV ...`、`label PROC`、`label DB ...`、以及标签独占一行（自动创建 `NOP` 占位）。为此语义扫描中做了多处判断：若碰到连续 IDENT IDENT，则查询 `tables` 决定语义。
- 操作数表示：为简化实现，对寄存器、立即数、标签/内存引用采用统一 `Operand` 结构，便于 codegen 统一处理：
  - 立即数在 codegen 中被写成 1 或 2 字节（依据大小）。
  - 标签/内存引用在 codegen 中通过记录重定位（Relocation）在最后填充实际地址。

可扩展性设计（添加新指令/寻址模式）：
- 新指令路径：在 `include/tables.h` / `src/tables.c` 中增加新的 `InstructionInfo` 条目（助记符、opcode、operand_count、is_pseudo）；在 `codegen_emit_instruction` 中基于 `instr_info->type` 增加相应的机器码生成逻辑（或拆成子函数）。
- 寻址模式扩展：增加 `OperandType` 枚举与 `semantic` 中对 `[` `]` 的更细粒度解析；在 `codegen` 中为每种寻址模式编写编码函数。
- 表达式与常量折叠：现阶段只支持简单数字，未来可以引入表达式解析器（中缀转后缀 -> 计算），并把结果填充到 `Operand.value`。

错误处理与诊断：
- 统一通过 `error_report(line, code, detail)` 记录错误并影响最终返回值。
- 在语义/生成阶段，尽量把错误与具体 token/lexeme 一起打印以便复现（已改进以显示未解析 token 或未定义符号）。

测试策略：
- 单元测试：对 `utils`、`lexer`、`symtab`、`tables` 编写明确的单元测试（已有部分 C 测试文件）。
- 集成测试：逐个运行 `tests/*.asm` 并比较生成的 `.com` 或字节序列大小/内容。
- 回归与 fuzz：用随机或半随机源输入测试词法与语义鲁棒性，发现边界条件。

性能与内存：
- 当前实现使用简单的动态分配（util_malloc），容量默认值（如 instruction_count 初始 512）。可根据需要用池分配或内存映射优化大项目。

路线图（扩展与自举）：
1) 稳定基础：
   - 完整实现所有 readme 列出的指令与伪指令
   - 完善表达式求值、立即数解析（支持十进制/十六进制/字符/串）
2) 指令集扩展：
   - 支持更多寻址模式（基址+变址+立即偏移）
   - 增加复杂指令（IMUL、IDIV、LES、LDS 等）
3) 目标格式与链接：
   - 支持生成更通用的目标格式（简易 ELF/REL/OBJ），或输出 NASM/MASM 格式目标以便与链接器协同
4) 宏、条件汇编与预处理：
   - 引入宏系统、IF/ELSE/ENDC 风格条件汇编、包括文件（INCLUDE）与常量定义
5) 自举（用 SUBAS 汇编 SUBAS）路径：
   - 将现有 C 实现逐步转为由 SUBAS 支持的汇编模块实现：
     a) 为 SUBAS 自举准备一套最小运行时/库（IO、内存、字符串）
     b) 用 SUBAS 能表示的子集逐步实现核心模块（lexer/semantic/codegen 的一部分）
     c) 先通过 C 编译器构建可用的工具链，之后用 SUBAS 生成的可执行替换原编译器产物并测试一致性

贡献指南（简要）：
- Fork → feature branch → PR
- 每次变更应包含单元测试（若可能）并运行 `make` 与 `tests/*.asm` 的一遍验证
- 遵循代码风格（详见 docs/CODING_GUIDE.md）

附录：关键文件位置
- `src/lexer.c`, `include/lexer.h`
- `src/semantic.c`, `include/semantic.h`
- `src/codegen.c`, `include/codegen.h`
- `src/tables.c`, `include/tables.h`
- `src/symtab.c`, `include/symtab.h`
- `src/error.c`, `include/error.h`
- `src/utils/*`：字符串/哈希/内存工具
- `tests/*.asm`：集成测试输入

结束语：
此文档旨在为后续开发者提供清晰的设计视图与实现细节。若要进一步推进自举与大型指令集支持，推荐先按路线图稳定表达式解析与寻址模式，再逐步编写回归测试以保证向后兼容。
