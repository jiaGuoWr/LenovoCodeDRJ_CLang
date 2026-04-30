//===- NamingConventionCheck.h - NAME001 ---------------------------------===//
//
// Enforces Lenovo DRJ naming conventions (mirrors C# NAME001):
//   - Public classes / structs / enums -> PascalCase
//   - Public methods / free functions -> PascalCase
//   - Constants (const / constexpr globals) -> UPPER_SNAKE_CASE
//   - Interface-like classes (pure virtual, no fields) -> prefix with 'I'
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::naming {

class NamingConventionCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;

  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::naming
