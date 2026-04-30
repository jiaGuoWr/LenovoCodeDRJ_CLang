#include "PathTraversalCheck.h"

#include "common/AstUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <llvm/ADT/StringRef.h>

#include <cctype>
#include <string>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

namespace {

bool nameLooksLikePath(llvm::StringRef name) {
  std::string lowered;
  lowered.reserve(name.size());
  for (char ch : name) {
    lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  llvm::StringRef view(lowered);
  return view.contains("path") || view.contains("dir") || view.contains("file") ||
         view.contains("location") || view.contains("uri") || view.endswith("name");
}

bool initializerLooksLikeConcat(const clang::Expr *init) {
  if (init == nullptr) {
    return false;
  }
  const clang::Expr *expr = init->IgnoreParenImpCasts();

  if (const auto *binary = clang::dyn_cast<clang::CXXOperatorCallExpr>(expr)) {
    if (binary->getOperator() == clang::OO_Plus) {
      return true;
    }
  }
  if (const auto *binary = clang::dyn_cast<clang::BinaryOperator>(expr)) {
    if (binary->getOpcode() == clang::BO_Add) {
      return true;
    }
  }
  return false;
}

} // namespace

void PathTraversalCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      varDecl(hasInitializer(expr().bind("init")),
              unless(isExpansionInSystemHeader()),
              unless(parmVarDecl()),
              unless(isImplicit()))
          .bind("var"),
      this);
}

void PathTraversalCheck::check(const MatchFinder::MatchResult &result) {
  const auto *var = result.Nodes.getNodeAs<clang::VarDecl>("var");
  const auto *init = result.Nodes.getNodeAs<clang::Expr>("init");
  if (var == nullptr || init == nullptr) {
    return;
  }
  if (!common::isInMainFile(var, *result.SourceManager)) {
    return;
  }

  const std::string name = var->getNameAsString();
  if (!nameLooksLikePath(name)) {
    return;
  }
  if (!initializerLooksLikeConcat(init)) {
    return;
  }

  diag(var->getLocation(),
       "variable %0 built by concatenation may allow directory traversal")
      << name;
}

} // namespace clang::tidy::lenovo::security
