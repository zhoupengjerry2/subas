**SUBAS 项目代码风格与贡献指南**

目标：保证代码风格一致、可维护、便于企业级协作与审查。

语言：C (兼容 C90/C99 子集，要求可移植到嵌入环境)。

格式约定：
- 缩进：使用 4 个空格缩进，禁止使用 Tab。
- 行宽：尽量不超过 100 列。
- 大括号：K&R 风格（函数名后大括号另起一行或与函数在同一行视项目一致性而定，当前项目使用函数签名后换行大括号）。
- 注释：
  - 文件头注释：每个源文件以文件说明、作者与日期开头（简短）。
  - 函数注释：简要说明功能、参数与返回值（必要时）。
- 命名：
  - 全局常量/宏：大写下划线（例：MAX_SOURCE_SIZE）
  - 结构体/类型：CamelCase 或以模块前缀（例：PassOne、InstructionEntry）
  - 函数/变量：小写加下划线（例：semantic_pass_one）

错误处理：
- 统一通过 `error_report(line, code, detail)` 报告错误，并在 `main` 读取 `error_get_count()` 决定是否中止。

测试：
- 添加/修改功能时应增加对应单元测试或集成测试（把 asm 放到 `tests/` 并运行 `subas` 验证生成结果）。

提交规范（Commit Message）：
- 模板：`<scope>: <short description>`
  - scope: `src`, `docs`, `tests`, `build` 等
  - short description: 50 字以内简短说明
  - 可选 body: 详细变更原因与实现要点

分支与代码评审：
- 功能分支命名：`feature/<短描述>`；修复：`fix/<短描述>`；文档：`docs/<短描述>`。
- PR 需至少 1 名审查者通过，且所有 CI（构建 + tests）通过。

本文件仅为快速约定，团队可依需把规则迁移到更细粒度的 linters 或 CI 配置中。


