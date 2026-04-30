// RUN: %check_clang_tidy %s lenovo-code001-commented-out-code %t

// var oldImpl = new OldThing();
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: comment looks like commented-out code; use version control instead [lenovo-code001-commented-out-code]

// if (x > 0) { return x; }
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: comment looks like commented-out code; use version control instead [lenovo-code001-commented-out-code]

/* return foo + bar; */
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: comment looks like commented-out code; use version control instead [lenovo-code001-commented-out-code]
