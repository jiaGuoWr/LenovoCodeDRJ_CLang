//===- RegexDosCheck.h - SEC006 ------------------------------------------===//
//
// Flags every construction of std::regex / std::wregex because std::regex has
// no built-in timeout and is implemented with backtracking, making it prone to
// ReDoS. Aligned with C# analyzer rule SEC006.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class RegexDosCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
