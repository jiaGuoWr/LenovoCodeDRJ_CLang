// RUN: %check_clang_tidy %s lenovo-exc001-stack-trace-in-catch %t

#include <stdexcept>

namespace logging {
struct Logger {
  void Error(const char*, const std::exception&) {}
};
extern Logger logger;
}

void Ok() {
  try {
    throw std::runtime_error("boom");
  } catch (const std::exception& ex) {
    logging::logger.Error("operation failed", ex);  // structured logging
  }
}

// CHECK-MESSAGES-NOT: warning:
