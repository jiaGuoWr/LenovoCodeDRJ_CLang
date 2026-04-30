// RUN: %check_clang_tidy %s lenovo-chn001-chinese-comments %t

/* 块注释中的中文 */
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: comment contains Chinese characters [lenovo-chn001-chinese-comments]

/*
 * Multi-line comment
 * 混合英文 and 中文 content
 */
// CHECK-MESSAGES: :[[@LINE-4]]:1: warning: comment contains Chinese characters [lenovo-chn001-chinese-comments]
