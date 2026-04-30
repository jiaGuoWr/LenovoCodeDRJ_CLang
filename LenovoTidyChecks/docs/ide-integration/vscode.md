# 在 VS Code 中使用

> v0.2 起推荐通过 **Lenovo Tidy** 扩展使用，VS Code 端体验从"半实时"升级到"接近实时"（保存后 100~800ms）。

## 推荐流程：装扩展

1. 打开 VS Code，**Extensions** 面板搜索 `Lenovo Tidy`
2. 点击 **Install**
3. 重新加载窗口
4. 打开项目根（含 `compile_commands.json`）
5. 保存任意 `.cpp`，**Problems 面板**（Ctrl+Shift+M）显示 `lenovo-*` 诊断

扩展内置：

- `lenovo-tidy-lsp` 三平台二进制（linux-x64 / darwin-arm64 / win32-x64）
- `libLenovoTidyChecks.{so,dll,dylib}` 规则插件
- LSP 客户端胶水代码

## 配置项（可选）

`settings.json` 里可覆盖：

```json
{
  "lenovoTidy.clangTidyPath": "/opt/homebrew/opt/llvm@18/bin/clang-tidy",
  "lenovoTidy.pluginPath": "/path/to/your/own/libLenovoTidyChecks.so",
  "lenovoTidy.compileCommandsDir": "${workspaceFolder}/build/Release",
  "lenovoTidy.checks": "lenovo-*,bugprone-*",
  "lenovoTidy.extraArgs": ["--header-filter=src/.*"]
}
```

## 与 clangd 共存

本扩展只负责 `lenovo-*` 诊断；通用 IntelliSense / 跳转 / 补全建议仍走 clangd。
推荐在同一项目里**同时安装 `llvm-vs-code-extensions.vscode-clangd`**。

为避免诊断重复，把 clangd 的 tidy 关掉：

```json
{
  "clangd.arguments": ["--clang-tidy=false"]
}
```

## 高级流程：手动 Task 触发（v0.1 兼容）

不想装扩展的话，仍可走 v0.1 的 `tasks.json` + `runonsave` 方案；详见 git 仓库
v0.1 文档历史，或本项目 [`AnalyzerRules.md`](../../AnalyzerRules.md) 注释。

## 故障排查

| 现象 | 排查 |
|---|---|
| Problems 面板始终空白 | 用 **View → Output → Lenovo Tidy** 看 LSP 日志；常见是 `compile_commands.json` 缺失 |
| `failed to spawn lenovo-tidy-lsp` | 重新安装扩展或换平台预编译版本 |
| 启动慢 | 第一次会等待 LSP 启动 + clang-tidy 子进程；之后会快很多 |
