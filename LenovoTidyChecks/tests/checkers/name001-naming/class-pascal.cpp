// RUN: %check_clang_tidy %s lenovo-name001-naming-convention %t

class user_service {};
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: class 'user_service' should use PascalCase (e.g. 'UserService') [lenovo-name001-naming-convention]

class userService {};
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: class 'userService' should use PascalCase (e.g. 'UserService') [lenovo-name001-naming-convention]

struct some_point {};
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: struct 'some_point' should use PascalCase (e.g. 'SomePoint') [lenovo-name001-naming-convention]

enum class error_code { kOk };
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: enum 'error_code' should use PascalCase (e.g. 'ErrorCode') [lenovo-name001-naming-convention]
