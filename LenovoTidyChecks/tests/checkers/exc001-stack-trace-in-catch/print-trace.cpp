// RUN: %check_clang_tidy %s lenovo-exc001-stack-trace-in-catch %t

#include <execinfo.h>
#include <stdexcept>
#include <stdio.h>

void Bad1() {
  try {
    throw std::runtime_error("boom");
  } catch (const std::exception&) {
    void* buf[20];
    int n = backtrace(buf, 20);
    backtrace_symbols_fd(buf, n, 2);
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: printing stack trace from catch may leak internal structure; use structured logging [lenovo-exc001-stack-trace-in-catch]
  }
}

void Bad2() {
  try {
    throw std::runtime_error("boom");
  } catch (const std::exception& ex) {
    fprintf(stderr, "%s", ex.what());
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: printing exception message to stderr from catch may leak internal structure; use structured logging [lenovo-exc001-stack-trace-in-catch]
  }
}
