# lenovo-tidy-lsp

Rust 语言服务器（LSP），把 [LenovoTidyChecks](../LenovoTidyChecks/) 的 clang-tidy 插件包装成 LSP 协议，**任何支持 LSP 的编辑器都能直接消费**。

## 它怎么工作

```
[IDE 保存文件]
     ↓
LSP 收到 textDocument/didSave
     ↓
启动 clang-tidy 子进程，传 --export-fixes=tmp.yaml
     ↓
读 tmp.yaml，转成 LSP Diagnostic
     ↓
publishDiagnostics 推回 IDE
```

## 构建

```bash
cargo build --release
# 产出：target/release/lenovo-tidy-lsp(.exe)
```

## 配置

LSP 客户端通过 `initializationOptions` 传：

```json
{
  "clangTidyPath": "/usr/bin/clang-tidy-18",
  "pluginPath":     "/path/to/libLenovoTidyChecks.so",
  "compileCommandsDir": "${workspaceFolder}/build",
  "checks": "lenovo-*"
}
```

未提供时回退到环境变量：`LENOVO_CLANG_TIDY` / `LENOVO_TIDY_PLUGIN` / `LENOVO_COMPILE_DB`。

## 独立运行

如果你的编辑器（Neovim / Helix / Emacs）支持 LSP，可以直接配置：

```bash
lenovo-tidy-lsp
```

读 stdin / 写 stdout，按 LSP 协议帧通信。

## 相关项目

- [`LenovoTidyChecks`](../LenovoTidyChecks/) - C++ 规则引擎
- [`LenovoTidyVscode`](../LenovoTidyVscode/) - VS Code 薄扩展
- [`LenovoTidyVs2022`](../LenovoTidyVs2022/) - VS 2022 薄扩展
