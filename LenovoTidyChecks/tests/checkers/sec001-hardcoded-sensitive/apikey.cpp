// RUN: %check_clang_tidy %s lenovo-sec001-hardcoded-sensitive %t

const char* apiKey = "sk-1234567890abcdef";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable 'apiKey' may contain hardcoded sensitive information [lenovo-sec001-hardcoded-sensitive]

const char* api_token = "ghp_abcdefghijklmnop";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable 'api_token' may contain hardcoded sensitive information [lenovo-sec001-hardcoded-sensitive]

const char* secret = "s3cret-value";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable 'secret' may contain hardcoded sensitive information [lenovo-sec001-hardcoded-sensitive]

const char* access_token = "eyJhbGciOiJIUzI1NiJ9.abc.def";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable 'access_token' may contain hardcoded sensitive information [lenovo-sec001-hardcoded-sensitive]
