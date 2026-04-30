#include "InsecureRandomCheck.h"

#include "common/AstUtils.h"

#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

void InsecureRandomCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      callExpr(
          callee(functionDecl(hasAnyName(
              "rand", "srand", "rand_r",
              "drand48", "lrand48", "mrand48", "srand48", "seed48",
              "::std::rand", "::std::srand"))),
          unless(isExpansionInSystemHeader()))
          .bind("call"),
      this);
}

void InsecureRandomCheck::check(const MatchFinder::MatchResult &result) {
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
       "use of insecure random function %0; use a cryptographic generator")
      << fd->getName();
}

} // namespace clang::tidy::lenovo::security
