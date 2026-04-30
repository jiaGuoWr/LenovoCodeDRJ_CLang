//===- SqlInjectionCheck.h - SEC003 --------------------------------------===//
//
// Detects strings that look like SQL statements (start with SELECT / INSERT /
// UPDATE / DELETE / MERGE / EXEC) being constructed via string concatenation
// (operator+). Aligned with C# analyzer rule SEC003.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class SqlInjectionCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
