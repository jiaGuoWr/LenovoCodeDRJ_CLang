// RUN: %check_clang_tidy %s lenovo-sec001-hardcoded-sensitive %t

// Variables whose names do not suggest sensitive content should be ignored,
// even when their string value looks suspicious.
const char* greeting = "sk-this-looks-like-a-key-but-isn-t";

// Sensitive-looking names are fine when the value is empty, short, or clearly
// a placeholder loaded from configuration at runtime.
const char* password_env_var = "DB_PASSWORD";
const char* api_key_config_name = "ApiKey";
const char* secret = "";

// No diagnostics should be emitted for this file.
// CHECK-MESSAGES-NOT: warning:
