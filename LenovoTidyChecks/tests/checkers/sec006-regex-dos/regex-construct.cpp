// RUN: %check_clang_tidy %s lenovo-sec006-regex-dos-risk %t

#include <regex>
#include <string>

void M1(const std::string& pattern) {
  std::regex re(pattern);
  // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: std::regex constructed without timeout/policy; potential ReDoS [lenovo-sec006-regex-dos-risk]
  (void)re;
}

void M2() {
  std::regex re("(a+)+");
  // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: std::regex constructed without timeout/policy; potential ReDoS [lenovo-sec006-regex-dos-risk]
  (void)re;
}
