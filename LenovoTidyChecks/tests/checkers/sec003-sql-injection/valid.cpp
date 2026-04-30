// RUN: %check_clang_tidy %s lenovo-sec003-sql-injection %t

#include <string>

// Plain literal, no concatenation
const char* fixedQuery = "SELECT * FROM Users WHERE Name = ?";
std::string param      = "SELECT id FROM Users WHERE Name = @name";

// Parameterised at runtime by binding mechanism
std::string status     = "INSERT INTO logs(line) VALUES(?)";

// CHECK-MESSAGES-NOT: warning:
