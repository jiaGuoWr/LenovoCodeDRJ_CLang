# Lenovo Tidy for Visual Studio 2022

Wraps [`lenovo-tidy-lsp`](../LenovoTidyLsp/) as a Visual Studio 2022 LSP client
extension. Diagnostics from [LenovoTidyChecks](../LenovoTidyChecks/) appear in
the **Error List** automatically as you save C/C++ files.

## 安装（业务开发者视角）

1. 在 VS 2022 → **Extensions → Manage Extensions**，搜索 **Lenovo Tidy**（或本地 `Install.. → LenovoTidy.vsix`）
2. 关闭并重新启动 VS 2022（VSIX 安装一定要 idle）
3. 打开任何含 `compile_commands.json` 或解决方案的 C/C++ 项目
4. 保存任意 `.cpp/.h`，看 **Error List** 即可

## 构建

```bash
dotnet restore
dotnet build -c Release
# 产出：bin/Release/net472/LenovoTidy.vsix
```

发布前需把以下三个文件放到 `server-bin/win32-x64/` 目录（CI `extension-package.yml` 会自动完成）：

| 文件 | 作用 |
|---|---|
| `lenovo-tidy-lsp.exe` | LSP 服务器，VS 2022 的 LspClient 启动子进程 |
| `lenovo-clang-tidy.exe` | 自带 lenovo-* 规则的 standalone clang-tidy；LSP 默认调用它 |
| `LenovoTidyChecks.dll` | 保留兼容 Linux/macOS 的 plugin 形态，**Windows 上 LSP 会自动忽略**（双 ManagedStatic 会 crash） |

## 已知限制

- 仅支持 64 位 Windows 上的 VS 2022 17.0+
- 用 `Microsoft.VisualStudio.LanguageServer.Client` 标准 API；高级 LSP 协议（CodeAction、CodeLens 等）尚未启用
- VSIX 体积约 47 MB（其中 `lenovo-clang-tidy.exe` 占 ~98 MB 未压缩、压缩后 ~30 MB），因为它静态包含了完整的 LLVM/Clang/clang-tidy
