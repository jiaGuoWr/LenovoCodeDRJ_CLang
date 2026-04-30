#include "InsecureTempFileCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

void InsecureTempFileCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      callExpr(callee(functionDecl(hasAnyName(
                   "tmpnam", "tmpnam_r", "mktemp", "tempnam",
                   "::std::tmpnam", "::std::mktemp", "::std::tempnam"))),
               unless(isExpansionInSystemHeader()))
          .bind("call"),
      this);
}

void InsecureTempFileCheck::check(const MatchFinder::MatchResult &result) {
  const auto *call = result.Nodes.getNodeAs<clang::CallExpr>("call");
  if (call == nullptr) {
    return;
  }
  const clang::FunctionDecl *fd = call->getDirectCallee();
  if (fd == nullptr) {
    return;
  }
  if (!result.SourceManager->isInMainFile(
          result.SourceManager->getExpansionLoc(call->getBeginLoc()))) {
    return;
  }
  diag(call->getBeginLoc(),
       "insecure temp-file API %0; use mkstemp / "
       "std::filesystem::temp_directory_path")
      << fd->getName();
}

} // namespace clang::tidy::lenovo::security
