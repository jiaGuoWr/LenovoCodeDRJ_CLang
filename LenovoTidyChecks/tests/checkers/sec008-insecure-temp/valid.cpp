// RUN: %check_clang_tidy %s lenovo-sec008-insecure-temp-file %t

#include <cstdlib>

void Ok(char tpl[]) {
  int fd = ::mkstemp(tpl);  // safe: atomic create + open
  (void)fd;
}

// CHECK-MESSAGES-NOT: warning:
