// RUN: %check_clang_tidy %s lenovo-sec001-hardcoded-sensitive %t

const char* password = "MySecretPassword123";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable 'password' may contain hardcoded sensitive information [lenovo-sec001-hardcoded-sensitive]

const char* user_password = "another-secret";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable 'user_password' may contain hardcoded sensitive information [lenovo-sec001-hardcoded-sensitive]

const char* pwd = "1234";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable 'pwd' may contain hardcoded sensitive information [lenovo-sec001-hardcoded-sensitive]
