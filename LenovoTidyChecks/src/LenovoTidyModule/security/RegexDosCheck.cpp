#include "RegexDosCheck.h"

#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

void RegexDosCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      cxxConstructExpr(
          hasType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(
              cxxRecordDecl(hasAnyName(
                  "::std::basic_regex",
                  "::std::regex",
                  "::std::wregex")))))),
          unless(isExpansionInSystemHeader()))
          .bind("ctor"),
      this);
}

void RegexDosCheck::check(const MatchFinder::MatchResult &result) {
  const auto *ctor = result.Nodes.getNodeAs<clang::CXXConstructExpr>("ctor");
  if (ctor == nullptr) {
    return;
  }
  if (!result.SourceManager->isInMainFile(
          result.SourceManager->getExpansionLoc(ctor->getBeginLoc()))) {
    return;
  }
  diag(ctor->getBeginLoc(),
       "std::regex constructed without timeout/policy; potential ReDoS");
}

} // namespace clang::tidy::lenovo::security
