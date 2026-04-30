#include "InsecureIpcCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

void InsecureIpcCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      callExpr(callee(functionDecl(hasAnyName(
                   "::grpc::InsecureChannelCredentials",
                   "::grpc::InsecureServerCredentials",
                   "::grpc_insecure_channel_create",
                   "::ng_http2_session_client_new"))),
               unless(isExpansionInSystemHeader()))
          .bind("call"),
      this);
}

void InsecureIpcCheck::check(const MatchFinder::MatchResult &result) {
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
       "insecure IPC API %0; use TLS-enabled credentials")
      << fd->getName();
}

} // namespace clang::tidy::lenovo::security
