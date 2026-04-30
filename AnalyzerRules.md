# Analyzer Rules Reference

This document provides detailed information about all diagnostic rules implemented in the Lenovo Direnjie code analyzer.

> **C++ port status (LenovoTidyChecks v0.2.0)**
>
> 15 of the 17 rules below have been ported to C++ (Clang-Tidy). The C++ rule
> ids are `lenovo-<id-lower>-<short-name>`, see `LenovoTidyChecks/docs/rules/`
> for per-rule pages.
>
> Two rules are intentionally **not ported** because the underlying language
> feature does not exist in C++:
>
> - **DLL002** (`DefaultDllImportSearchPaths`) - C# P/Invoke specific.
>   The closest C++ analogue is enforced by **DLL003** instead.
> - **SEC009** (Unsafe Reflection) - C++ has no reflection runtime.
>
> See `LenovoTidyChecks/docs/adr/0002-skip-dll002-sec009.md` for full rationale.

<!-- AUTO-GENERATED: RULES_REFERENCE_START -->

## Summary

| Category | Rules | Description |
|----------|-------|-------------|
| Localization | CHN001 | Chinese character detection |
| Security | DLL002, DLL003, SEC001-SEC011 | Security vulnerability detection |
| Reliability | EXC001 | Exception handling issues |
| Maintainability | CODE001 | Code quality issues |
| Naming | NAME001 | Naming convention enforcement |

---

## Localization Rules

### CHN001 - Chinese Characters in Comments

| Property | Value |
|----------|-------|
| **ID** | CHN001 |
| **Category** | Localization |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Chinese in comments

**Description**: Code comments should be in English for international team collaboration.

**Message Format**: `Comment contains Chinese characters: '{0}'`

**Example**:

```csharp
// ❌ Triggers CHN001
// 这是一个中文注释

// ✅ Correct
// This is an English comment
```

---

## Security Rules

### DLL002 - Missing DllImportSearchPaths

| Property | Value |
|----------|-------|
| **ID** | DLL002 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Missing DllImportSearchPaths

**Description**: Specify DefaultDllImportSearchPaths to prevent DLL hijacking attacks.

**Message Format**: `DllImport missing DefaultDllImportSearchPaths attribute`

**Example**:

```csharp
// ❌ Triggers DLL002
[DllImport("user32.dll")]
public static extern int MessageBox(IntPtr hWnd, string text, string caption, uint type);

// ✅ Correct
[DllImport("user32.dll")]
[DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
public static extern int MessageBox(IntPtr hWnd, string text, string caption, uint type);
```

---

### DLL003 - Unsafe DLL Signature

| Property | Value |
|----------|-------|
| **ID** | DLL003 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Unsafe DLL signature

**Description**: Referenced DLLs should have valid digital signatures.

---

### SEC001 - Sensitive Information in Code

| Property | Value |
|----------|-------|
| **ID** | SEC001 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Sensitive info in code

**Description**: Sensitive information should not be hardcoded.

**Message Format**: `Variable '{0}' may contain hardcoded sensitive information`

**Example**:

```csharp
// ❌ Triggers SEC001
string password = "MySecretPassword123";
string apiKey = "sk-1234567890abcdef";

// ✅ Correct - use configuration or secrets manager
string password = Environment.GetEnvironmentVariable("DB_PASSWORD");
string apiKey = configuration["ApiKey"];
```

---

### SEC002 - Path Traversal Risk

| Property | Value |
|----------|-------|
| **ID** | SEC002 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Path traversal risk

**Description**: Validate and normalize paths to prevent directory traversal attacks.

**Example**:

```csharp
// ❌ Triggers SEC002
string filePath = basePath + userInput;

// ✅ Correct
string filePath = Path.GetFullPath(Path.Combine(basePath, userInput));
if (!filePath.StartsWith(basePath))
    throw new SecurityException("Invalid path");
```

---

### SEC003 - SQL Injection Risk

| Property | Value |
|----------|-------|
| **ID** | SEC003 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: SQL injection risk

**Description**: Use parameterized queries to prevent SQL injection.

**Example**:

```csharp
// ❌ Triggers SEC003
string query = "SELECT * FROM Users WHERE Name = '" + userName + "'";
string query = $"SELECT * FROM Users WHERE Name = '{userName}'";

// ✅ Correct
string query = "SELECT * FROM Users WHERE Name = @name";
command.Parameters.AddWithValue("@name", userName);
```

---

### SEC004 - Unsafe Deserialization

| Property | Value |
|----------|-------|
| **ID** | SEC004 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Unsafe deserialization

**Description**: Deserializers like BinaryFormatter pose remote code execution risks.

**Example**:

```csharp
// ❌ Triggers SEC004
var formatter = new BinaryFormatter();
var obj = formatter.Deserialize(stream);

// ✅ Correct - use System.Text.Json or other safe alternatives
var obj = JsonSerializer.Deserialize<MyType>(jsonString);
```

---

### SEC005 - Insecure Random Number Generator

| Property | Value |
|----------|-------|
| **ID** | SEC005 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Insecure random

**Description**: Use cryptographically secure random number generators for security-sensitive operations.

**Example**:

```csharp
// ❌ Triggers SEC005 (for security-sensitive use)
var random = new Random();
var token = random.Next();

// ✅ Correct
using var rng = RandomNumberGenerator.Create();
var bytes = new byte[32];
rng.GetBytes(bytes);
```

---

### SEC006 - Regex DoS Risk

| Property | Value |
|----------|-------|
| **ID** | SEC006 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Regex DoS risk

**Description**: Set reasonable timeouts for regex operations to prevent ReDoS attacks.

**Example**:

```csharp
// ❌ Triggers SEC006
var regex = new Regex(pattern);

// ✅ Correct
var regex = new Regex(pattern, RegexOptions.None, TimeSpan.FromSeconds(1));
```

---

### SEC007 - Resource Leak

| Property | Value |
|----------|-------|
| **ID** | SEC007 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Resource leak

**Description**: Use 'using' statements to ensure proper resource disposal.

**Message Format**: `IDisposable object '{0}' not in using statement`

**Example**:

```csharp
// ❌ Triggers SEC007
var stream = new FileStream(path, FileMode.Open);
// stream might not be disposed on exception

// ✅ Correct
using var stream = new FileStream(path, FileMode.Open);
```

---

### SEC008 - Insecure Temporary File

| Property | Value |
|----------|-------|
| **ID** | SEC008 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Insecure temp file

**Description**: Use secure methods when creating temporary files to prevent race conditions.

---

### SEC009 - Unsafe Reflection

| Property | Value |
|----------|-------|
| **ID** | SEC009 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Unsafe reflection

**Description**: Validate and restrict reflectable types and methods.

---

### SEC010 - Race Condition

| Property | Value |
|----------|-------|
| **ID** | SEC010 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Race condition

**Description**: Use thread-safe collections and proper synchronization in multi-threaded environments.

---

### SEC011 - Insecure IPC

| Property | Value |
|----------|-------|
| **ID** | SEC011 |
| **Category** | Security |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Insecure IPC

**Description**: Use secure binding types for WCF and other IPC communication.

---

## Reliability Rules

### EXC001 - Invalid Stack Trace Usage

| Property | Value |
|----------|-------|
| **ID** | EXC001 |
| **Category** | Reliability |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Stack trace in catch

**Description**: Printing full stack traces may expose internal system structure.

**Example**:

```csharp
// ❌ Triggers EXC001
catch (Exception ex)
{
    Console.WriteLine(ex.StackTrace);
}

// ✅ Correct - use structured logging
catch (Exception ex)
{
    logger.LogError(ex, "Operation failed");
}
```

---

## Maintainability Rules

### CODE001 - Commented-Out Code

| Property | Value |
|----------|-------|
| **ID** | CODE001 |
| **Category** | Maintainability |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Code in comment

**Description**: Comments should not contain commented-out code. Use source control for history.

**Example**:

```csharp
// ❌ Triggers CODE001
// var oldCode = new OldImplementation();
// oldCode.DoSomething();

// ✅ Correct - remove dead code, use git for history
var newCode = new NewImplementation();
newCode.DoSomething();
```

---

## Naming Rules

### NAME001 - Naming Convention Violation

| Property | Value |
|----------|-------|
| **ID** | NAME001 |
| **Category** | Naming |
| **Severity** | Warning |
| **Default** | Enabled |

**Title**: Naming convention violation

**Description**: Enforces .NET naming conventions: PascalCase for public types/members, UPPER_CASE for constants, and 'I' prefix for interfaces.

**Checked Members**:

| Member Type | Convention | Example |
|-------------|------------|---------|
| Public classes | PascalCase | `UserService`, `HttpClient` |
| Public structs | PascalCase | `Point`, `Rectangle` |
| Public enums | PascalCase | `Status`, `ErrorCode` |
| Interfaces | I + PascalCase | `IUserService`, `IDisposable` |
| Public methods | PascalCase | `GetUserData`, `ProcessRequest` |
| Public properties | PascalCase | `UserName`, `MaxSize` |
| Constants | UPPER_CASE | `MAX_SIZE`, `DEFAULT_TIMEOUT` |

**Not Checked**:

- Private fields
- Internal members
- Protected members
- Local variables
- Parameters

**Examples**:

```csharp
// ❌ Triggers NAME001
public interface UserService { }           // Missing 'I' prefix
public class user_service { }              // Not PascalCase
public void getUserData() { }              // Not PascalCase
public const int MaxSize = 100;            // Not UPPER_CASE

// ✅ Correct
public interface IUserService { }
public class UserService { }
public void GetUserData() { }
public const int MAX_SIZE = 100;
```

See [Naming Conventions](../.cursor/rules/naming-conventions.md) for detailed documentation.

<!-- AUTO-GENERATED: RULES_REFERENCE_END -->

---

## Configuring Rules

### Via .editorconfig

```ini
# Disable a rule
dotnet_diagnostic.CHN001.severity = none

# Change severity to error
dotnet_diagnostic.SEC001.severity = error

# Suppress in specific files
[*.Designer.cs]
dotnet_diagnostic.CODE001.severity = none
```

### Via #pragma

```csharp
#pragma warning disable SEC001
string legacyPassword = "oldpassword"; // Suppress for legacy code
#pragma warning restore SEC001
```

### Via SuppressMessage Attribute

```csharp
[SuppressMessage("Security", "SEC001:Sensitive info in code", Justification = "Test data")]
public class TestFixture { }
```
