// RUN: %check_clang_tidy %s lenovo-sec005-insecure-random %t

#include <random>

unsigned GetToken() {
  std::random_device rd;
  std::mt19937 eng(rd());
  std::uniform_int_distribution<unsigned> dist;
  return dist(eng);
}

// CHECK-MESSAGES-NOT: warning:
