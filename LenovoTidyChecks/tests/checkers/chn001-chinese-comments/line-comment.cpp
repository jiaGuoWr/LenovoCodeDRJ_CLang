// RUN: %check_clang_tidy %s lenovo-chn001-chinese-comments %t

// 这是一个中文注释
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: comment contains Chinese characters [lenovo-chn001-chinese-comments]

int x = 0; // 行尾中文注释
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: comment contains Chinese characters [lenovo-chn001-chinese-comments]

// hello 世界 with mixed content
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: comment contains Chinese characters [lenovo-chn001-chinese-comments]
