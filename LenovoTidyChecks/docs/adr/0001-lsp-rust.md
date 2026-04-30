# ADR-0001：用 Rust 实现 LSP 服务器

- 状态：Accepted
- 日期：2026-04-27
- 决策者：插件团队

## 背景

`v0.1` 中三条规则编译出 `libLenovoTidyChecks.so/.dll`，业务团队仍要手工配置
`-load`、`compile_commands.json`、IDE 全局选项。VS Code 用户更难受——
clangd 默认不支持 `-load`，规则在编辑器里看不到红波浪线。

为了让 **VS 2022 与 VS Code 装一个扩展即可使用**，我们决定加一层 LSP 服务器
作为统一中间件，让两端 IDE 用同一个二进制。

## 候选

- TypeScript（Node.js）：Microsoft 参考实现栈、生态成熟、与 VS Code 扩展同语言；
  但 VS 2022 端要么依赖系统 Node、要么打 Node SEA bundle。
- Rust：单文件 .exe、零运行时依赖、启动快、内存小；社区 LSP 库 `tower-lsp` 成熟；
  代价是团队要会 Rust。
- C++：与规则引擎同栈，但 LSP 生态最弱、错误处理代码量大。

## 决策

选 **Rust + `tower-lsp`**。

理由：

1. **单二进制分发** 对 VS 2022 端最友好（无需 Node）
2. **启动快、常驻内存小** — 多人开多个项目时累积差异显著
3. **不与 LLVM 直接链接**，仅 spawn `clang-tidy`，避免被 LLVM ABI 绑死
4. **类型严格的协议层** — 字段名拼错不会逃过编译期
5. 维护成本可控：核心逻辑就是 spawn + 解析 YAML + 推 LSP 消息

## 影响

- 团队需要 1~2 周 Rust 学习曲线（从未写过 Rust 的成员）
- CI 增加三平台 cargo build 任务（cold ~3min/平台，hot 缓存后 < 1min）
- 业务无感知：他们看到的是 VS Code / VS 2022 扩展

## 反向决策触发条件

如果未来出现以下情况，重新评估改用 TypeScript：

- 团队 Rust 维护者全部流失，找不到接手人
- 需要深度集成 VS Code Webview 等 Node 生态独家能力
- 业务要求把 LSP 嵌入 Electron / Web 应用
