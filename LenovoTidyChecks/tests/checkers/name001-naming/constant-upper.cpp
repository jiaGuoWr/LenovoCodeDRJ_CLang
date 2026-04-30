// RUN: %check_clang_tidy %s lenovo-name001-naming-convention %t

const int MaxSize = 100;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: constant 'MaxSize' should use UPPER_SNAKE_CASE (e.g. 'MAX_SIZE') [lenovo-name001-naming-convention]

const int defaultTimeout = 30;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: constant 'defaultTimeout' should use UPPER_SNAKE_CASE (e.g. 'DEFAULT_TIMEOUT') [lenovo-name001-naming-convention]

constexpr double Pi = 3.14;
// CHECK-MESSAGES: :[[@LINE-1]]:18: warning: constant 'Pi' should use UPPER_SNAKE_CASE (e.g. 'PI') [lenovo-name001-naming-convention]
