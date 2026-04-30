//===- InsecureTempFileCheck.h - SEC008 ----------------------------------===//
//
// Detects calls to TOCTOU-prone temp-file APIs: tmpnam, mktemp, tempnam.
// Aligned with C# analyzer rule SEC008.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class InsecureTempFileCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
