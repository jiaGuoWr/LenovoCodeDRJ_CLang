// RUN: %check_clang_tidy %s lenovo-sec002-path-traversal %t

#include <string>

// Plain assignment of a string literal is fine.
std::string fixedPath = "/etc/myapp.conf";

// Variable name does not look like a path.
std::string greeting = "/hello" + std::string(" world");

// Concatenation present but the variable name is unrelated to filesystem.
std::string banner = std::string("v") + "1.0";

// CHECK-MESSAGES-NOT: warning:
