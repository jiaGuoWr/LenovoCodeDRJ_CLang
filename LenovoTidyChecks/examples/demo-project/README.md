# demo-project

业务项目最小化样例，演示 v0.2 三种使用方式：装 VS Code 扩展 / 装 VS 2022 扩展 / 纯 CLI。

## 文件

| 文件 | 作用 |
|---|---|
| `src/demo.cpp` | 故意违规：覆盖全部 15 条规则各一次 |
| `src/demo_ok.cpp` | 合规版本：业务意图相同，零告警 |
| `.clang-tidy` | 启用 `lenovo-*` 全部规则族 |
| `CMakeLists.txt` | 生成 `compile_commands.json` |

## 三种使用方式

### 方式 A：VS Code 扩展（推荐，最简单）

1. 在 VS Code Extensions 搜索 `Lenovo Tidy` → Install
2. 用 VS Code 打开本目录
3. 用 CMake Tools 配置项目（生成 `compile_commands.json`）
4. 保存 `src/demo.cpp` → Problems 面板出现 ~15 条 `lenovo-*` 告警
5. 保存 `src/demo_ok.cpp` → 应该零告警

### 方式 B：VS 2022 扩展（Windows）

1. Extensions → Manage Extensions → 搜索 `Lenovo Tidy` → Install
2. 用 VS 打开 `CMakeLists.txt`
3. 让 VS 配置 CMake（产生 `compile_commands.json`）
4. 保存 `demo.cpp` → Error List 出现 `lenovo-*` 告警

### 方式 C：纯 CLI（CI、Linux 服务器）

```bash
cmake -S . -B build -G Ninja
PLUGIN=/path/to/libLenovoTidyChecks.so

clang-tidy-18 -load=$PLUGIN -p build src/demo.cpp     # 期望 15+ 警告
clang-tidy-18 -load=$PLUGIN -p build src/demo_ok.cpp  # 期望 0 警告
```

或用 LSP 服务器跑一次：

```bash
lenovo-tidy-lsp < /dev/null    # stdin 关掉则立即 shutdown，仅用来验证二进制能跑
```
