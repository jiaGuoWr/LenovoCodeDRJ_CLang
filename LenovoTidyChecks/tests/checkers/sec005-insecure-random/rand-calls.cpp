// RUN: %check_clang_tidy %s lenovo-sec005-insecure-random %t

#include <cstdlib>

unsigned GetToken() {
  return std::rand();
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: use of insecure random function 'rand'; use a cryptographic generator [lenovo-sec005-insecure-random]
}

void Seed() {
  std::srand(42);
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: use of insecure random function 'srand'; use a cryptographic generator [lenovo-sec005-insecure-random]
}
