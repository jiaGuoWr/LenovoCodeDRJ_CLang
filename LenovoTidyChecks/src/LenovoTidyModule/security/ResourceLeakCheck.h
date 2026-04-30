//===- ResourceLeakCheck.h - SEC007 --------------------------------------===//
//
// Flags raw allocations that are common sources of resource leaks:
//   - C++ 'new' / 'new[]' expressions used outside of make_unique / smart
//     pointer constructors
//   - C-style 'fopen' / 'tmpfile' calls whose result is stored in a raw pointer
//
// Aligned with C# analyzer rule SEC007.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class ResourceLeakCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
