# 在 Visual Studio 2022 中使用

> v0.2 起推荐通过 **Lenovo Tidy** VSIX 扩展使用，下面是"装扩展即用"流程。

## 推荐流程：装 VSIX 扩展

1. **Extensions → Manage Extensions**，搜索 `Lenovo Tidy`
2. 点击 **Download**，关闭 VS 让 VSIX Installer 完成安装
3. 重启 VS 2022
4. 打开任何 C/C++ 项目（含 `compile_commands.json` 或 `.sln`）
5. 保存任意 `.cpp`，**Error List** 自动出现 `lenovo-*` 诊断

VSIX 内置：

- `lenovo-tidy-lsp.exe`（语言服务器）
- `LenovoTidyChecks.dll`（规则插件）
- LSP 客户端胶水（`LspClient.cs`）

无需另外安装 LLVM 18。但**业务项目依然要有 `compile_commands.json`**——参见
《[CMake 项目接入](#cmake-项目接入)》。

## 高级流程：手动用 `clang-tidy.exe -load=...`（v0.1 兼容）

如果你不想装扩展，仍可走 v0.1 的纯 CLI 方案：

1. 装 LLVM 18 至 `C:\Program Files\LLVM\`
2. 拷贝 `LenovoTidyChecks.dll` 到 `C:\Lenovo\TidyPlugin\`
3. **Tools → Options → Text Editor → C/C++ → Code Style → Code Analysis → Clang-Tidy**：
   - **Clang-Tidy Tool Path**: `C:\Program Files\LLVM\bin\clang-tidy.exe`
   - **Additional command-line options**: `-load=C:/Lenovo/TidyPlugin/LenovoTidyChecks.dll`
4. 项目根放 `.clang-tidy`：`Checks: 'lenovo-*'`
5. 右键 `.cpp` → **Run Code Analysis on File**

## CMake 项目接入

无论用扩展还是手动方案，都需要 `compile_commands.json`：

```cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

VS 2022 的 "Open Folder + CMake" 模式会自动生成它。

## 故障排查

| 现象 | 排查 |
|---|---|
| Error List 没有 `lenovo-*` 诊断 | 确认扩展已启用：Extensions → Installed |
| 报 `lenovo-tidy-lsp.exe not found` | 重新安装 VSIX，确认 server-bin 目录有该文件 |
| 仍走老 `clang-tidy.exe` | Code Analysis 配置仍指向旧路径，清空它，让 VSIX 全权处理 |
| 一个文件分析很慢 | clang-tidy 本身慢，正常；可在 `lenovoTidy.checks` 里精简启用规则 |
