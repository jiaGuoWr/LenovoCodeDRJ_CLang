//===- HardcodedSensitiveInfoCheck.h - SEC001 ----------------------------===//
//
// Detects variable declarations whose names suggest sensitive content
// (password / secret / token / api-key / ...) and whose initializer is a
// non-trivial string literal.
//
// Aligned with C# analyzer rule SEC001.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>
#include <llvm/Support/Regex.h>

#include <memory>
#include <string>

namespace clang::tidy::lenovo::security {

class HardcodedSensitiveInfoCheck : public clang::tidy::ClangTidyCheck {
public:
  HardcodedSensitiveInfoCheck(llvm::StringRef name, ClangTidyContext *context);

  void storeOptions(ClangTidyOptions::OptionMap &opts) override;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;

  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;

private:
  /// Regex matching suspicious variable names. Default pattern covers
  /// password / pwd / secret / token / api[-_]?key / credential.
  std::string suspiciousPattern_;

  /// Minimum string literal length before we treat it as a hardcoded value.
  /// Short values like "", "x" or trivial placeholders are ignored.
  unsigned minimumLiteralLength_;

  std::unique_ptr<llvm::Regex> suspiciousRegex_;
};

} // namespace clang::tidy::lenovo::security
