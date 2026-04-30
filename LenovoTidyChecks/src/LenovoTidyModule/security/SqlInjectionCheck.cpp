#include "SqlInjectionCheck.h"

#include "common/AstUtils.h"

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

bool startsWithSqlKeyword(llvm::StringRef text) {
  std::string upper;
  upper.reserve(text.size());
  for (char ch : text.take_front(20)) {
    upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }
  llvm::StringRef view(upper);
  view = view.ltrim();
  static const llvm::StringRef kKeywords[] = {
      "SELECT ", "INSERT ", "UPDATE ", "DELETE ", "MERGE ", "EXEC ", "EXECUTE "};
  for (const auto &kw : kKeywords) {
    if (view.startswith(kw)) {
      return true;
    }
  }
  return false;
}

const clang::StringLiteral *findSqlLiteral(const clang::Expr *expr) {
  if (expr == nullptr) {
    return nullptr;
  }
  expr = expr->IgnoreParenImpCasts();
  if (const auto *literal = clang::dyn_cast<clang::StringLiteral>(expr)) {
    if (literal->getCharByteWidth() == 1 &&
        startsWithSqlKeyword(literal->getString())) {
      return literal;
    }
  }
  if (const auto *binary = clang::dyn_cast<clang::BinaryOperator>(expr)) {
    if (auto *l = findSqlLiteral(binary->getLHS())) return l;
    if (auto *r = findSqlLiteral(binary->getRHS())) return r;
  }
  if (const auto *call = clang::dyn_cast<clang::CXXOperatorCallExpr>(expr)) {
    if (call->getOperator() == clang::OO_Plus) {
      for (unsigned i = 0; i < call->getNumArgs(); ++i) {
        if (auto *l = findSqlLiteral(call->getArg(i))) return l;
      }
    }
  }
  if (const auto *construct = clang::dyn_cast<clang::CXXConstructExpr>(expr)) {
    for (const auto *arg : construct->arguments()) {
      if (auto *l = findSqlLiteral(arg)) return l;
    }
  }
  return nullptr;
}

bool isConcatenation(const clang::Expr *expr) {
  if (expr == nullptr) return false;
  expr = expr->IgnoreParenImpCasts();
  if (const auto *call = clang::dyn_cast<clang::CXXOperatorCallExpr>(expr)) {
    return call->getOperator() == clang::OO_Plus;
  }
  if (const auto *binary = clang::dyn_cast<clang::BinaryOperator>(expr)) {
    return binary->getOpcode() == clang::BO_Add;
  }
  return false;
}

} // namespace

void SqlInjectionCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      varDecl(hasInitializer(expr().bind("init")),
              unless(isExpansionInSystemHeader()),
              unless(isImplicit()),
              unless(parmVarDecl()))
          .bind("var"),
      this);
}

void SqlInjectionCheck::check(const MatchFinder::MatchResult &result) {
  const auto *var = result.Nodes.getNodeAs<clang::VarDecl>("var");
  const auto *init = result.Nodes.getNodeAs<clang::Expr>("init");
  if (var == nullptr || init == nullptr) {
    return;
  }
  if (!common::isInMainFile(var, *result.SourceManager)) {
    return;
  }
  if (!isConcatenation(init)) {
    return;
  }
  if (findSqlLiteral(init) == nullptr) {
    return;
  }

  diag(var->getLocation(),
       "SQL string built by concatenation may be vulnerable to injection");
}

} // namespace clang::tidy::lenovo::security
