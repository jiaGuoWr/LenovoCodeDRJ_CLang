# LSP 服务器（lenovo-tidy-lsp）

> Rust 编写的语言服务器，把 LenovoTidyChecks 暴露成 LSP 协议，**任何支持 LSP 的编辑器都能用**。

## 总体设计

```mermaid
flowchart LR
  IDE[IDE / 编辑器] -- LSP stdio --> LSP[lenovo-tidy-lsp]
  LSP -- spawn --> CT[clang-tidy]
  CT -- -load --> Plug[libLenovoTidyChecks]
  CT -- export-fixes yaml --> LSP
  LSP -- publishDiagnostics --> IDE
```

- 协议层：`tower-lsp`
- 子进程：`clang-tidy --export-fixes=tmp.yaml`，规则插件通过 `-load=` 加载
- 解析：`serde_yaml` 反序列化 YAML 诊断
- 推送：`textDocument/publishDiagnostics`

## 协议覆盖

| 方法 | 实现状态 |
|---|---|
| `initialize` / `initialized` | 是 |
| `shutdown` / `exit` | 是 |
| `textDocument/didOpen` | 触发首次分析 |
| `textDocument/didChange` | 仅缓存文本 |
| `textDocument/didSave` | 触发分析（主入口） |
| `textDocument/didClose` | 清空诊断 |
| `textDocument/codeAction` | v0.3 计划（FixIt） |
| `workspace/didChangeConfiguration` | v0.3 计划 |

## 配置（initializationOptions）

```json
{
  "clangTidyPath": "/usr/bin/clang-tidy-18",
  "pluginPath": "/path/to/libLenovoTidyChecks.so",
  "compileCommandsDir": "${workspaceFolder}/build",
  "checks": "lenovo-*",
  "extraArgs": []
}
```

未提供时回退到环境变量：

| Init 字段 | 环境变量回退 |
|---|---|
| `clangTidyPath` | `LENOVO_CLANG_TIDY` 或 PATH 上的 `clang-tidy-18` / `clang-tidy` |
| `pluginPath` | `LENOVO_TIDY_PLUGIN` |
| `compileCommandsDir` | `LENOVO_COMPILE_DB` |

## 直接独立运行（高级用户）

任何支持 LSP 的编辑器都能配置：

### Neovim

```lua
require'lspconfig'.lenovo_tidy.setup{
  cmd = { "lenovo-tidy-lsp" },
  filetypes = { "cpp", "c", "objcpp" },
  init_options = {
    clangTidyPath = "/usr/bin/clang-tidy-18",
    pluginPath = "/path/to/libLenovoTidyChecks.so",
    compileCommandsDir = vim.fn.getcwd() .. "/build",
  },
}
```

### Helix `languages.toml`

```toml
[[language]]
name = "cpp"
language-servers = ["lenovo-tidy-lsp", "clangd"]

[language-server.lenovo-tidy-lsp]
command = "lenovo-tidy-lsp"
```

## 性能

| 指标 | 量级 |
|---|---|
| 启动时间 | < 50 ms（Rust 二进制 cold） |
| 单文件分析延迟 | 100~800 ms（主要是 clang-tidy 子进程） |
| 常驻内存 | < 20 MB |
| 二进制大小 | 5~10 MB（release + lto + strip） |

## 故障定位

设置环境变量 `LENOVO_TIDY_LOG=debug` 可在 stderr 看到详细日志。
LSP 客户端通常会把 stderr 收到 IDE 的 Output 面板。
