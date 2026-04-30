// RUN: %check_clang_tidy %s lenovo-sec010-race-condition %t

#include <atomic>
#include <mutex>

const int kMaxRetry = 3;          // const => safe
constexpr int kPi100 = 314;       // constexpr => safe

std::atomic<int> g_counter{0};    // atomic => safe
thread_local int t_state = 0;     // thread_local => safe

std::mutex g_mu;                  // mutex itself is fine

// CHECK-MESSAGES-NOT: warning:
