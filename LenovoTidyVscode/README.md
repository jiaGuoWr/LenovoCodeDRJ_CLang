# Lenovo Tidy for VS Code

Bundles the [`lenovo-tidy-lsp`](../LenovoTidyLsp/) language server and surfaces
the [LenovoTidyChecks](../LenovoTidyChecks/) rule diagnostics in VS Code's
**Problems** panel.

## 安装（业务开发者视角）

1. 在 VS Code Extensions 面板里搜索 **Lenovo Tidy**（或从内部 Marketplace 双击安装 `.vsix`）
2. 重启 VS Code
3. 打开任意 C/C++ 项目（含 `compile_commands.json`），保存 `.cpp` 即可看到诊断

## 自动化行为（v0.2+）

扩展在激活时会按这个顺序找 `compile_commands.json`：

1. 用户显式设置的 `lenovoTidy.compileCommandsDir`；
2. 常见位置扫描：`build/`, `out/`, `cmake-build-debug/`, `cmake-build-release/`,
   `cmake-build-relwithdebinfo/`, `.vscode/`, 工作区根目录；
3. 探测到 `CMakeLists.txt` → 弹确认框 → 自动运行
   `cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`（带进度条、可取消）；
4. 探测到 `.sln` / `.vcxproj` / `Makefile` → 弹提示给出 `compdb` / `bear` 命令；
5. 以上都没有 → 提示说只有文本类规则（CHN001/CODE001/SEC001）会触发。

选 "Never for this workspace" 后可以随时用命令面板（`Ctrl+Shift+P`）跑
**Lenovo Tidy: Regenerate compile_commands.json** 重新触发。

## 命令面板

| 命令 | 说明 |
|---|---|
| `Lenovo Tidy: Regenerate compile_commands.json` | 重新扫描 + 按需自动生成 |
| `Lenovo Tidy: Show Server Output` | 打开 LSP 输出面板排错 |
| `Lenovo Tidy: Restart Language Server` | 重启 LSP（改完 settings 常用） |

## 配置项

| 设置 | 默认 | 说明 |
|---|---|---|
| `lenovoTidy.clangTidyPath` | `""` | `clang-tidy` 路径。**留空即可**：扩展自动使用 bundled 的 `lenovo-clang-tidy.exe`（Windows）或 PATH 上的 `clang-tidy-18/clang-tidy`（Linux/macOS） |
| `lenovoTidy.pluginPath` | `""` | `libLenovoTidyChecks.*` 路径。**仅 Linux/macOS 用**；Windows 上 LSP 会自动忽略（bundled exe 已含规则） |
| `lenovoTidy.compileCommandsDir` | `""` | `compile_commands.json` 目录。**留空 = 自动探测**（见上节）；或显式指定如 `${workspaceFolder}/out/Release` |
| `lenovoTidy.autoConfigureCmake` | `true` | CMake 项目里没 `compile_commands.json` 时是否提示自动运行 cmake |
| `lenovoTidy.cmakeExecutable` | `"cmake"` | 自动 configure 用的 CMake 可执行文件，名称或绝对路径 |
| `lenovoTidy.checks` | `"lenovo-*"` | clang-tidy `--checks` 过滤 |
| `lenovoTidy.extraArgs` | `[]` | 透传给 clang-tidy 的额外参数 |

## 开发者构建

```bash
npm install
npm run compile
npm run package        # → lenovo-tidy-vscode-0.1.0.vsix
```

发布前，把以下二进制复制到 `server-bin/<platform>-<arch>/` 下：
- `lenovo-tidy-lsp[.exe]`（LSP 服务器，必需）
- **Windows**：`lenovo-clang-tidy.exe`（已静态链接 lenovo-* 规则，必需）
- **Linux/macOS**：`libLenovoTidyChecks.{so,dylib}`（plugin dll，必需）；可选附带 `lenovo-clang-tidy` 也支持

CI 在 `extension-package.yml` 自动完成此步。

## 实现说明

参见 `src/extension.ts` 与 [LSP 服务器架构](../LenovoTidyChecks/docs/lsp-server.md)。
