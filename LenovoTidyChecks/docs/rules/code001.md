# CODE001 - 注释中的死代码（Commented-Out Code）

| 属性 | 取值 |
|---|---|
| **ID** | `CODE001` / `lenovo-code001-commented-out-code` |
| **类别** | Maintainability |
| **严重级别** | Warning |
| **默认启用** | 是 |
| **对齐 C#** | [`CODE001` in AnalyzerRules.md](../../AnalyzerRules.md#code001---commented-out-code) |

## 描述

死代码不应留在注释里——版本管理工具会保留历史。本规则用启发式判定一段注释是否"看起来像代码"：

- 含有分号 `;`
- 含有花括号 `{` `}`
- 含有赋值 ` = ` 或比较 `==`
- 以 `return / if / for / while / var / auto / int / std::` 开头

满足两条以上信号时报警。

`TODO` / `FIXME` / `XXX` / `NOTE` / `HACK` / `@param` / `Copyright` / `SPDX` 开头的注释会被白名单跳过。

## 触发示例

```cpp
// var oldImpl = new OldThing();    // ❌
// if (x > 0) { return x; }         // ❌
/* return foo + bar; */              // ❌
```

## 合规示例

```cpp
// This is a normal English explanation
// TODO: handle the new use case
// FIXME: see ticket DRJ-1234
```

## 已知限制

- 启发式可能漏报真实死代码（如纯文档化的代码片段，看起来像散文）
- 也可能误报描述代码但没有完整代码形态的注释
- 通过 NOLINT 或调整白名单可豁免
