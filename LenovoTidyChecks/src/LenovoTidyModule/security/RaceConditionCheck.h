//===- RaceConditionCheck.h - SEC010 -------------------------------------===//
//
// Heuristic: flags non-const, non-atomic, non-thread_local global / namespace
// scope variables that are common race-condition footguns. Aligned with C#
// analyzer rule SEC010.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class RaceConditionCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
