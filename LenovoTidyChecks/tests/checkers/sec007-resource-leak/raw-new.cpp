// RUN: %check_clang_tidy %s lenovo-sec007-resource-leak %t

struct Widget {};

void Bad1() {
  Widget* w = new Widget();
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: raw 'new' allocation; prefer std::unique_ptr / std::make_unique [lenovo-sec007-resource-leak]
  (void)w;
}

void Bad2() {
  int* arr = new int[100];
  // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: raw 'new' allocation; prefer std::unique_ptr / std::make_unique [lenovo-sec007-resource-leak]
  (void)arr;
}
