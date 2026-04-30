# LenovoTidyChecks

> 基于 Clang-Tidy 的 Lenovo DRJ 自定义规则插件，对标现有 C# Roslyn 分析器。

## 它解决什么问题

当同一个团队同时维护 C# 与 C++ 代码库时，往往会出现"规则两张皮"的尴尬：C# 的 Roslyn 分析器已经把编码规范、敏感信息检测、命名约定做得很全，但 C++ 这边没有对应品。

本项目通过 **out-of-tree Clang-Tidy 插件** 的方式，把 [`AnalyzerRules.md`](../AnalyzerRules.md) 描述的 17 条 C# 规则按类别移植到 C++ 侧，**规则 ID 完全对齐**，让团队的 CI 报告、审查清单、抑制记录可以在两种语言之间自然映射。

## v0.2.0 覆盖哪些规则

15 条规则与 C# 版 AnalyzerRules.md 对齐（DLL002、SEC009 因 C++ 无对应语言特性而 N/A，详见 [ADR-0002](adr/0002-skip-dll002-sec009.md)）：

| ID | 名称 | 类别 |
|---|---|---|
| SEC001 | 硬编码敏感信息 | Security |
| SEC002 | 路径遍历 | Security |
| SEC003 | SQL 注入 | Security |
| SEC004 | 不安全反序列化 | Security |
| SEC005 | 不安全随机数 | Security |
| SEC006 | 正则 DoS | Security |
| SEC007 | 资源泄漏 | Security |
| SEC008 | 不安全临时文件 | Security |
| SEC010 | 竞态条件 | Security |
| SEC011 | 不安全 IPC | Security |
| DLL003 | 不安全 DLL 加载 | Security |
| EXC001 | catch 中打印栈跟踪 | Reliability |
| CODE001 | 注释中的代码 | Maintainability |
| CHN001 | 注释中文 | Localization |
| NAME001 | 命名规范 | Naming |

完整规则页面在 [规则参考](rules/sec001.md) 一节；路线图见 [`CHANGELOG.md`](https://github.com/your-org/LenovoTidyChecks/blob/main/CHANGELOG.md)。

完整规则路线图见 [`CHANGELOG.md`](https://github.com/your-org/LenovoTidyChecks/blob/main/CHANGELOG.md)。

## 快速开始

=== "Linux / macOS"

    ```bash
    # 安装依赖
    sudo apt install llvm-18-dev libclang-18-dev clang-tools-18 ninja-build cmake
    # 构建
    cmake --preset linux-release
    cmake --build --preset linux-release
    # 跑测试
    ctest --preset linux-release --output-on-failure
    ```

=== "Windows"

    ```powershell
    # 安装 LLVM 18 官方预编译包
    choco install llvm --version=18.1.8
    cmake --preset windows-release
    cmake --build --preset windows-release
    ctest --preset windows-release
    ```

加载到业务项目：

```bash
clang-tidy -load=/path/to/libLenovoTidyChecks.so -p build/ src/main.cpp
```

## 下一步

- 架构与模块划分见 [架构](architecture.md)
- 想写一条新规则？参阅 [开发指南](development-guide.md) 与 [TDD 工作流](tdd-workflow.md)
- 集成到 IDE：[Visual Studio](ide-integration/visual-studio.md) / [VS Code](ide-integration/vscode.md)
- 接入 CI：[CI 集成](ci-integration.md)
