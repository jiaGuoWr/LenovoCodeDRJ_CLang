//===- InsecureRandomCheck.h - SEC005 ------------------------------------===//
//
// Detects calls to insecure random APIs (rand, srand, rand_r, drand48, ...) and
// recommends a cryptographic alternative. Aligned with C# analyzer rule SEC005.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class InsecureRandomCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
