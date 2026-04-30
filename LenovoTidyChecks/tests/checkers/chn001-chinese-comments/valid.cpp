// RUN: %check_clang_tidy %s lenovo-chn001-chinese-comments %t

// This is a plain English comment - no Chinese characters.
/* Block comment in pure English. */
/*
 * Multi-line comment with numbers 123 and symbols !@#$%
 * but nothing that would trigger the CJK detector.
 */

int main() {
  // ASCII only comment inside function
  return 0;
}

// CHECK-MESSAGES-NOT: warning:
