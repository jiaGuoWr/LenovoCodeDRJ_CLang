#include "HardcodedSensitiveInfoCheck.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Regex.h>

#include <string>
#include <vector>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

namespace {

constexpr llvm::StringLiteral kDefaultPattern =
    "^(.*_)?(password|passwd|pwd|secret|token|api[_-]?key|access[_-]?key|"
    "credential|credentials)(_.*)?$";

constexpr unsigned kDefaultMinLength = 6;

/// Returns true if \p value looks like a placeholder / configuration key
/// rather than an actual secret. We whitelist values that are ALL_UPPER +
/// underscores + digits (typical env-var reference) or well-known config
/// property names (camelCase / PascalCase without digits or punctuation).
bool looksLikePlaceholder(llvm::StringRef value) {
  if (value.empty()) {
    return true;
  }

  bool allUpperEnv = true;
  for (char ch : value) {
    const bool upper = ch >= 'A' && ch <= 'Z';
    const bool digit = ch >= '0' && ch <= '9';
    const bool underscore = ch == '_';
    if (!(upper || digit || underscore)) {
      allUpperEnv = false;
      break;
    }
  }
  if (allUpperEnv) {
    return true;
  }

  // Config property names, e.g. "ApiKey", "Password" -> treat as reference.
  bool onlyLetters = true;
  for (char ch : value) {
    const bool upper = ch >= 'A' && ch <= 'Z';
    const bool lower = ch >= 'a' && ch <= 'z';
    if (!(upper || lower)) {
      onlyLetters = false;
      break;
    }
  }
  if (onlyLetters && value.size() <= 16) {
    return true;
  }

  return false;
}

/// Extract the UTF-8 byte content of a StringLiteral while skipping wide
/// literals we do not want to reason about in this check.
std::string extractStringLiteralValue(const clang::StringLiteral *literal) {
  if (literal == nullptr) {
    return {};
  }
  if (literal->getCharByteWidth() != 1) {
    return {};
  }
  return literal->getString().str();
}

} // namespace

HardcodedSensitiveInfoCheck::HardcodedSensitiveInfoCheck(
    llvm::StringRef name, ClangTidyContext *context)
    : ClangTidyCheck(name, context),
      suspiciousPattern_(
          Options.get("SuspiciousNamePattern", kDefaultPattern.str())),
      minimumLiteralLength_(
          Options.get("MinimumLiteralLength", kDefaultMinLength)),
      suspiciousRegex_(std::make_unique<llvm::Regex>(
          suspiciousPattern_, llvm::Regex::IgnoreCase)) {
  std::string error;
  if (!suspiciousRegex_->isValid(error)) {
    // Fall back to default pattern silently if the user-provided regex is
    // malformed; clang-tidy will still surface a diagnostic via storeOptions.
    suspiciousRegex_ = std::make_unique<llvm::Regex>(
        kDefaultPattern.str(), llvm::Regex::IgnoreCase);
  }
}

void HardcodedSensitiveInfoCheck::storeOptions(
    ClangTidyOptions::OptionMap &opts) {
  Options.store(opts, "SuspiciousNamePattern", suspiciousPattern_);
  Options.store(opts, "MinimumLiteralLength", minimumLiteralLength_);
}

void HardcodedSensitiveInfoCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      varDecl(hasInitializer(ignoringParenImpCasts(
                  stringLiteral().bind("literal"))),
              unless(isExpansionInSystemHeader()))
          .bind("var"),
      this);
}

void HardcodedSensitiveInfoCheck::check(
    const MatchFinder::MatchResult &result) {
  const auto *var = result.Nodes.getNodeAs<clang::VarDecl>("var");
  const auto *literal =
      result.Nodes.getNodeAs<clang::StringLiteral>("literal");
  if (var == nullptr || literal == nullptr) {
    return;
  }

  const std::string name = var->getNameAsString();
  if (name.empty()) {
    return;
  }

  llvm::SmallVector<llvm::StringRef> matches;
  if (!suspiciousRegex_->match(name, &matches)) {
    return;
  }

  const std::string value = extractStringLiteralValue(literal);
  if (value.size() < minimumLiteralLength_) {
    return;
  }
  if (looksLikePlaceholder(value)) {
    return;
  }

  diag(var->getLocation(),
       "variable %0 may contain hardcoded sensitive information")
      << name;
}

} // namespace clang::tidy::lenovo::security
