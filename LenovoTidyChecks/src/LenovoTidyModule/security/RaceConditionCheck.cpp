#include "RaceConditionCheck.h"

#include "common/AstUtils.h"

#include <clang/AST/Decl.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

#include <string>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

namespace {

bool isAtomicOrMutex(const clang::QualType type) {
  const std::string name = type.getAsString();
  return name.find("std::atomic") != std::string::npos ||
         name.find("std::__1::atomic") != std::string::npos ||
         name.find("std::mutex") != std::string::npos ||
         name.find("std::recursive_mutex") != std::string::npos ||
         name.find("std::shared_mutex") != std::string::npos ||
         name.find("std::__1::mutex") != std::string::npos;
}

} // namespace

void RaceConditionCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      varDecl(hasGlobalStorage(),
              unless(isExpansionInSystemHeader()),
              unless(parmVarDecl()),
              unless(isImplicit()))
          .bind("var"),
      this);
}

void RaceConditionCheck::check(const MatchFinder::MatchResult &result) {
  const auto *var = result.Nodes.getNodeAs<clang::VarDecl>("var");
  if (var == nullptr) {
    return;
  }
  if (!common::isInMainFile(var, *result.SourceManager)) {
    return;
  }

  const clang::QualType type = var->getType();
  if (type.isConstQualified() || var->isConstexpr()) {
    return;
  }
  if (var->getTLSKind() != clang::VarDecl::TLS_None) {
    return; // thread_local
  }
  if (isAtomicOrMutex(type)) {
    return;
  }
  // Skip function-static (TLS_None && local). hasGlobalStorage matches both
  // namespace-scope and function-static; we only want namespace-scope.
  if (var->isStaticLocal()) {
    return;
  }

  const std::string name = var->getNameAsString();
  if (name.empty()) {
    return;
  }

  diag(var->getLocation(),
       "mutable global %0 is shared without synchronisation; potential race "
       "condition")
      << name;
}

} // namespace clang::tidy::lenovo::security
