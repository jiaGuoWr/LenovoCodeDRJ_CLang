//===- PathTraversalCheck.h - SEC002 -------------------------------------===//
//
// Heuristic detector for path-like variables that are constructed through
// string concatenation (operator+ on std::string), which is a common shape of
// directory traversal vulnerabilities.
//
// Aligned with C# analyzer rule SEC002.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class PathTraversalCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
