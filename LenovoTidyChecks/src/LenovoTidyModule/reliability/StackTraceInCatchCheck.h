//===- StackTraceInCatchCheck.h - EXC001 ---------------------------------===//
//
// Detects calls inside catch handlers that print raw stack-trace / exception
// info to stderr / stdout, leaking internal structure. Aligned with C#
// analyzer rule EXC001.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::reliability {

class StackTraceInCatchCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::reliability
