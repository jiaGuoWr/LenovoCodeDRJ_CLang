// RUN: %check_clang_tidy %s lenovo-code001-commented-out-code %t

// This is a normal English explanation of the function below.
// It contains no obvious code patterns.

/*
 * Multi-line block comment describing the algorithm
 * with prose only and no semicolons.
 */

// TODO: handle the new use case
// FIXME: see ticket DRJ-1234

// CHECK-MESSAGES-NOT: warning:
