#include "ResourceLeakCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

namespace {

bool isInsideSmartPointerCtor(const clang::CXXNewExpr *newExpr,
                              clang::ASTContext &ctx) {
  for (const auto &parent : ctx.getParents(*newExpr)) {
    if (const auto *call = parent.get<clang::CXXConstructExpr>()) {
      const std::string name = call->getConstructor()->getParent()->getQualifiedNameAsString();
      if (name == "std::unique_ptr" || name == "std::shared_ptr" ||
          name == "std::__1::unique_ptr" || name == "std::__1::shared_ptr") {
        return true;
      }
    }
    if (const auto *call = parent.get<clang::CallExpr>()) {
      if (const auto *fd = call->getDirectCallee()) {
        const std::string name = fd->getQualifiedNameAsString();
        if (name == "std::make_unique" || name == "std::make_shared") {
          return true;
        }
      }
    }
  }
  return false;
}

} // namespace

void ResourceLeakCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      cxxNewExpr(unless(isExpansionInSystemHeader())).bind("new"),
      this);

  finder->addMatcher(
      callExpr(callee(functionDecl(hasAnyName(
                   "fopen", "tmpfile", "freopen",
                   "::std::fopen", "::std::tmpfile", "::std::freopen"))),
               unless(isExpansionInSystemHeader()))
          .bind("fopen"),
      this);
}

void ResourceLeakCheck::check(const MatchFinder::MatchResult &result) {
  const auto &sm = *result.SourceManager;

  if (const auto *newExpr = result.Nodes.getNodeAs<clang::CXXNewExpr>("new")) {
    if (!sm.isInMainFile(sm.getExpansionLoc(newExpr->getBeginLoc()))) {
      return;
    }
    if (isInsideSmartPointerCtor(newExpr, *result.Context)) {
      return;
    }
    diag(newExpr->getBeginLoc(),
         "raw 'new' allocation; prefer std::unique_ptr / std::make_unique");
    return;
  }

  if (const auto *fopen = result.Nodes.getNodeAs<clang::CallExpr>("fopen")) {
    if (!sm.isInMainFile(sm.getExpansionLoc(fopen->getBeginLoc()))) {
      return;
    }
    const clang::FunctionDecl *fd = fopen->getDirectCallee();
    if (fd == nullptr) {
      return;
    }
    diag(fopen->getBeginLoc(),
         "%0 result not wrapped in RAII; potential resource leak")
        << fd->getName();
  }
}

} // namespace clang::tidy::lenovo::security
