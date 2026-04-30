# LenovoTidyChecks

> C++ 版 Roslyn 分析器：基于 Clang-Tidy 的 **Lenovo DRJ 自定义规则插件**。
> 与现有 C# 侧分析器 [`AnalyzerRules.md`](../AnalyzerRules.md) **规则 ID 一一对齐**，支持 Windows / Linux / macOS 三平台。

## 它是什么

一个 **out-of-tree 的 clang-tidy 插件**：

- 源码：本仓库的 `src/LenovoTidyModule/`
- 产物：`libLenovoTidyChecks.so` / `.dll` / `.dylib`
- 消费：业务项目在 `.clang-tidy` 里启用 `lenovo-*` 规则族，配合 `clang-tidy -load=...`

当前版本（v0.1.0）交付 3 条代表性规则：

| ID | 规则名 | 类别 | 技术形态 |
|---|---|---|---|
| SEC001 | `lenovo-sec001-hardcoded-sensitive` | 安全 | 变量声明 + 字符串字面量匹配 |
| CHN001 | `lenovo-chn001-chinese-comments` | 本地化 | 注释 Token + CJK Unicode |
| NAME001 | `lenovo-name001-naming-convention` | 命名 | Decl 遍历 + 正则 |

## 快速开始

### 1. 准备 LLVM 18

```bash
# Ubuntu/Debian
sudo apt install llvm-18-dev libclang-18-dev clang-tools-18

# macOS
brew install llvm@18

# Windows
# 从 https://github.com/llvm/llvm-project/releases 下载 LLVM-18.x-win64.exe
```

### 2. 构建

```bash
cmake --preset linux-release        # 或 macos-release / windows-release
cmake --build --preset linux-release
```

产物位于 `build/linux-release/src/LenovoTidyModule/libLenovoTidyChecks.so`。

### 3. 跑测试（TDD 全绿灯）

```bash
ctest --preset linux-release --output-on-failure
```

### 4. 在业务项目里使用

业务项目根目录放一份 `.clang-tidy`：

```yaml
Checks: 'lenovo-sec001-*,lenovo-chn001-*,lenovo-name001-*'
```

#### Linux / macOS（plugin 模式）

```bash
clang-tidy -load=/path/to/libLenovoTidyChecks.so \
           -p build/ \
           src/main.cpp
```

#### Windows（推荐：standalone 可执行文件）

LLVM 官方 Windows 二进制是**完全静态链接**的，`-load=...dll` 实际不会让插件
里的 `ClangTidyModuleRegistry::Add<>` 注册到主程序的 registry（dll 与 exe
各持一份独立副本），表现是「dll 加载成功但 `--list-checks` 看不到 lenovo-*」。
具体可见 [LLVM #159710](https://github.com/llvm/llvm-project/issues/159710)。

为此，本仓库新增 `src/LenovoTidyMain/lenovo-clang-tidy` 目标：把所有 lenovo-*
规则 + 上游 clang-tidy 全部 module 静态链接进单个可执行文件，**不需要**
`-load=`：

```powershell
lenovo-clang-tidy.exe -p build\ src\main.cpp
```

跨平台都可用同样的命令；Linux/macOS 想保持现有 plugin 工作流也仍然可以。

见 `examples/demo-project/` 的完整示例。

## 目录速览

```
src/LenovoTidyModule/   # 规则实现（C++17）
  ├─ LenovoTidyModule.cpp   # 注册所有规则
  ├─ common/                # StringUtils / AstUtils
  ├─ security/              # SEC*
  ├─ localization/          # CHN*
  └─ naming/                # NAME*
src/LenovoTidyMain/     # standalone 可执行文件（Windows 推荐）
  ├─ main.cpp               # 调用 clang::tidy::clangTidyMain，携带规则 anchor
  └─ include/clang-tidy-config.h  # 补 LLVM Windows SDK 缺失的生成头
tests/
  ├─ checkers/              # lit 集成测试（每条规则一个子目录）
  └─ unit/                  # GoogleTest 单元测试
docs/                   # MkDocs 文档站
examples/demo-project/  # 演示业务项目
scripts/                # 脚手架与工具链
ci/                     # GitHub Actions + Dockerfile
```

## 文档

本地启动文档站：

```bash
pip install mkdocs mkdocs-material
mkdocs serve -f docs/mkdocs.yml
```

线上地址（CI 自动部署）：<https://example.internal.lenovo/drj-tidy/>

## 贡献新规则

```bash
python3 scripts/new_check.py --id SEC002 --name path-traversal --category security
```

脚手架会生成 `.h` / `.cpp` / lit 测试 / 文档模板；按 **TDD 工作流** 补齐即可，流程见 [docs/tdd-workflow.md](docs/tdd-workflow.md)。

## 规则 ID 对齐

所有规则 ID 与 C# 版 [`AnalyzerRules.md`](../AnalyzerRules.md) 一致（CHN/SEC/EXC/CODE/NAME + 数字），CI 里由 `scripts/generate_rules_index.py` 做一致性校验。

## 许可证

Apache License 2.0，见 [LICENSE](LICENSE)。
