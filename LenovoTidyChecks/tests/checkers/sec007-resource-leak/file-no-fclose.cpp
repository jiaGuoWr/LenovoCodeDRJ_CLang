// RUN: %check_clang_tidy %s lenovo-sec007-resource-leak %t

#include <cstdio>

void Bad() {
  std::FILE* f = std::fopen("a.txt", "r");
  // CHECK-MESSAGES: :[[@LINE-1]]:18: warning: 'fopen' result not wrapped in RAII; potential resource leak [lenovo-sec007-resource-leak]
  (void)f;
}
