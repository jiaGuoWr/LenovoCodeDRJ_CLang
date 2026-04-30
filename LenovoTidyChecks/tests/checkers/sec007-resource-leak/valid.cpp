// RUN: %check_clang_tidy %s lenovo-sec007-resource-leak %t

#include <fstream>
#include <memory>

struct Widget {};

void Ok1() {
  auto w = std::make_unique<Widget>();
  (void)w;
}

void Ok2() {
  std::ifstream in("a.txt");
  (void)in;
}

// CHECK-MESSAGES-NOT: warning:
