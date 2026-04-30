# Changelog

本项目遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/) 与 [SemVer](https://semver.org/lang/zh-CN/) 2.0.0。

## [0.2.0] - 2026-04-27

### 新增

12 条新规则，覆盖矩阵从 3/17 提升到 15/17（DLL002、SEC009 不适用 C++，见 ADR-0002）：

- **SEC002** `lenovo-sec002-path-traversal`：路径遍历风险（拼接构造路径）
- **SEC003** `lenovo-sec003-sql-injection`：SQL 注入（SELECT/INSERT/... 字面量与拼接）
- **SEC004** `lenovo-sec004-unsafe-deserialization`：boost.archive / cereal / msgpack 反序列化
- **SEC005** `lenovo-sec005-insecure-random`：rand / srand / drand48 系列调用
- **SEC006** `lenovo-sec006-regex-dos-risk`：std::regex 构造（无超时）
- **SEC007** `lenovo-sec007-resource-leak`：裸 new、fopen 未 RAII
- **SEC008** `lenovo-sec008-insecure-temp-file`：tmpnam / mktemp / tempnam
- **SEC010** `lenovo-sec010-race-condition`：可变全局无同步
- **SEC011** `lenovo-sec011-insecure-ipc`：grpc Insecure* 凭据
- **DLL003** `lenovo-dll003-unsafe-dll-load`：LoadLibrary / dlopen 短名加载
- **EXC001** `lenovo-exc001-stack-trace-in-catch`：catch 中打印栈跟踪 / stderr
- **CODE001** `lenovo-code001-commented-out-code`：被注释掉的代码（启发式）

### 新增组件（本仓库 monorepo 兄弟项目）

- `LenovoTidyLsp/`：Rust 编写的 LSP 服务器，包装 clang-tidy 子进程
- `LenovoTidyVscode/`：VS Code 薄扩展，启动 LSP
- `LenovoTidyVs2022/`：VS 2022 薄扩展（VSIX），启动 LSP

### 变更

- `AnalyzerRules.md` 头部增加 C++ port status 说明
- 新增 `docs/adr/0001-lsp-rust.md` 与 `docs/adr/0002-skip-dll002-sec009.md`
- `docs/ide-integration/{visual-studio,vscode}.md` 改为"装扩展即用"
- `examples/demo-project/` 扩充全部 15 条规则的违规与合规示例

### 规划中（v0.3）

- FixIt 自动修复（SEC005 / SEC006 / SEC007 优先）
- 规则 Options 参数化（白名单类型、最小长度等）
- LSP 常驻分析（不依赖保存触发）
- 业务接入 dogfood 与误报回归

## [0.1.0] - 2026-04-27

### 新增

- 项目基础骨架：CMake + LLVM 18 依赖、CMakePresets 三平台、自我检查 `.clang-tidy`
- 注册模块 `LenovoTidyModule`，产出 `libLenovoTidyChecks` 动态库
- 通用工具：`StringUtils` 提供 UTF-8 / CJK 字符判断
- **SEC001** `lenovo-sec001-hardcoded-sensitive`：硬编码敏感信息检测
- **CHN001** `lenovo-chn001-chinese-comments`：注释中文字符检测
- **NAME001** `lenovo-name001-naming-convention`：命名规范检查
- 测试框架：lit + FileCheck + GoogleTest
- 示例业务项目 `examples/demo-project`
- MkDocs Material 文档站
- 脚本工具：`scripts/new_check.py` / `generate_rules_index.py` / `run-tests.sh`
- CI 管线：build-test / release / docs-deploy
- 开发容器 `ci/Dockerfile.dev`
