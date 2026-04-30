//===- ChineseInCommentsCheck.h - CHN001 ---------------------------------===//
//
// Emits a diagnostic for every source comment (// or /* */) that contains at
// least one CJK character. Keeps international code review friction low by
// nudging contributors to English comments.
//
// Aligned with C# analyzer rule CHN001.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::localization {

class ChineseInCommentsCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerPPCallbacks(const clang::SourceManager &sourceManager,
                           clang::Preprocessor *preprocessor,
                           clang::Preprocessor *moduleExpanderPP) override;
};

} // namespace clang::tidy::lenovo::localization
