//===- UnsafeDllLoadCheck.h - DLL003 -------------------------------------===//
//
// Detects calls to LoadLibrary / LoadLibraryEx / dlopen whose first argument is
// a string literal that is NOT an absolute path. Loading by short name lets
// the loader search PATH-like locations and is the classical DLL hijacking
// vector. Aligned with C# analyzer rule DLL003.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class UnsafeDllLoadCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
