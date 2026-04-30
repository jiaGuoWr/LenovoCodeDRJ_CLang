// RUN: %check_clang_tidy %s lenovo-sec010-race-condition %t

int g_counter = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: mutable global 'g_counter' is shared without synchronisation; potential race condition [lenovo-sec010-race-condition]

static int s_state = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: mutable global 's_state' is shared without synchronisation; potential race condition [lenovo-sec010-race-condition]

namespace {
double cache = 0.0;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: mutable global 'cache' is shared without synchronisation; potential race condition [lenovo-sec010-race-condition]
}
