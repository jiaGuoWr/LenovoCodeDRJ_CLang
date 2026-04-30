//===- CommentedOutCodeCheck.h - CODE001 ---------------------------------===//
//
// Heuristically flags comments that look like commented-out code, e.g. lines
// containing trailing semicolons, balanced braces, or assignment-like keyword
// patterns. Aligned with C# analyzer rule CODE001.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::maintainability {

class CommentedOutCodeCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerPPCallbacks(const clang::SourceManager &sourceManager,
                           clang::Preprocessor *preprocessor,
                           clang::Preprocessor *moduleExpanderPP) override;
};

} // namespace clang::tidy::lenovo::maintainability
