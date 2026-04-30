// RUN: %check_clang_tidy %s lenovo-sec003-sql-injection %t

#include <string>

void Q1(const std::string& userName) {
  std::string sql = "SELECT * FROM Users WHERE Name = '" + userName + "'";
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: SQL string built by concatenation may be vulnerable to injection [lenovo-sec003-sql-injection]
  (void)sql;
}

void Q2(const std::string& tableName) {
  std::string query = "DELETE FROM " + tableName + " WHERE id = 1";
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: SQL string built by concatenation may be vulnerable to injection [lenovo-sec003-sql-injection]
  (void)query;
}

void Q3(const std::string& v) {
  std::string s = std::string("UPDATE accounts SET balance = ") + v;
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: SQL string built by concatenation may be vulnerable to injection [lenovo-sec003-sql-injection]
  (void)s;
}
