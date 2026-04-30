// RUN: %check_clang_tidy %s lenovo-sec008-insecure-temp-file %t

#include <cstdio>
#include <cstdlib>

void Bad1(char* buf) {
  std::tmpnam(buf);
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: insecure temp-file API 'tmpnam'; use mkstemp / std::filesystem::temp_directory_path [lenovo-sec008-insecure-temp-file]
}

void Bad2(char* tpl) {
  ::mktemp(tpl);
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: insecure temp-file API 'mktemp'; use mkstemp / std::filesystem::temp_directory_path [lenovo-sec008-insecure-temp-file]
}

void Bad3() {
  char* p = ::tempnam(nullptr, "pre");
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: insecure temp-file API 'tempnam'; use mkstemp / std::filesystem::temp_directory_path [lenovo-sec008-insecure-temp-file]
  (void)p;
}
