# NAME001 - 命名规范（Naming Convention）

| 属性 | 取值 |
|---|---|
| **ID** | `NAME001` / `lenovo-name001-naming-convention` |
| **类别** | Naming |
| **严重级别** | Warning |
| **默认启用** | 是 |
| **对齐 C#** | [`NAME001` in AnalyzerRules.md](../../AnalyzerRules.md#name001---naming-convention-violation) |

## 描述

在 C++ 侧沿用 .NET 的命名惯例，便于跨语言代码的一致性（特别是当同一个团队同时维护 C# 与 C++ 两侧时）：

| 成员类型 | 约定 | 示例 |
|---|---|---|
| 公共 class / struct / enum | PascalCase | `UserService`, `Point`, `ErrorCode` |
| 接口类（纯虚 + 无字段）| `I` + PascalCase | `IUserService`, `IDisposable` |
| 公共方法 / 自由函数 | PascalCase | `GetUserData`, `ProcessRequest` |
| 全局常量 / constexpr | UPPER_SNAKE_CASE | `MAX_SIZE`, `PI`, `DEFAULT_TIMEOUT_MS` |

**不检查的成员**（与 C# 版一致）：

- `private` / `protected` 成员
- 局部变量、函数参数
- 匿名类型、Lambda
- 隐式生成的成员

## 消息格式

```
<kind> 'NAME' should use <CASE> (e.g. 'SUGGESTED') [lenovo-name001-naming-convention]
```

`<kind>` 取值：`class` / `struct` / `enum` / `public method` / `interface-like class` / `constant`。

## 触发示例

```cpp
// ❌ 不是 PascalCase
class user_service {};
struct some_point {};
enum class error_code { kOk };

// ❌ 方法不是 PascalCase
class MyService {
public:
  void getUserData();
};

// ❌ 常量不是 UPPER_SNAKE_CASE
const int MaxSize = 100;
constexpr double Pi = 3.14;

// ❌ 接口类缺少 'I' 前缀
class UserService {
public:
  virtual ~UserService() = default;
  virtual void DoWork() = 0;
};
```

## 合规示例

```cpp
// ✅
class UserService {
public:
  void GetUserData();
  int  ProcessRequest();
  const char* UserName();
};

struct Point {};
enum class Status { kOk, kErr };

class IUserService {
public:
  virtual ~IUserService() = default;
  virtual void DoWork() = 0;
};

const int MAX_SIZE = 100;
constexpr double PI = 3.14;
const int DEFAULT_TIMEOUT_MS = 5000;
```

## 接口类的判定规则

当一个 `class` / `struct` 同时满足下列条件时被视为接口类：

1. 有完整定义（非前向声明）
2. 无字段（`field_empty()`）
3. 所有非隐式、非构造/析构方法都是 **纯虚**
4. 至少包含一个纯虚方法

构造/析构函数（包括虚析构）不会干扰判定。

## 配置选项

当前版本无可调选项。未来版本计划引入命名例外清单与自定义模式。

## 抑制方式

```cpp
// NOLINT(lenovo-name001-naming-convention)
class legacy_api {};
```

或使用 `.clang-tidy` 的 `HeaderFilterRegex` 把第三方头文件排除在外（默认即仅分析主文件）。

## 已知限制

- **与 `readability-identifier-naming` 的关系**：官方的 `readability-identifier-naming` 覆盖面更广，但默认不与 C# 语义强对齐。NAME001 的定位是：**精准匹配 C# 的检查项与消息格式**，方便在统一的仪表盘上看到两种语言的同类告警
- 不处理 `using` 别名、模板参数命名
- `I` + 长全大写（如 `IOStream`）会被放行（其第二个字母是大写，满足 PascalCase 起始）；如需严格区分，配合 `readability-identifier-length` 等规则使用
- 宏命名不在此规则范围内
