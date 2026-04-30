#include "NamingConventionCheck.h"

#include "common/AstUtils.h"
#include "common/StringUtils.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <llvm/ADT/StringRef.h>

#include <cctype>
#include <string>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::naming {

namespace {

std::string toPascalCase(llvm::StringRef name) {
  std::string result;
  result.reserve(name.size());
  bool capitalizeNext = true;
  for (char ch : name) {
    if (ch == '_' || ch == '-') {
      capitalizeNext = true;
      continue;
    }
    if (capitalizeNext && ch >= 'a' && ch <= 'z') {
      result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
      capitalizeNext = false;
    } else if (capitalizeNext && ch >= 'A' && ch <= 'Z') {
      result.push_back(ch);
      capitalizeNext = false;
    } else {
      result.push_back(ch);
      capitalizeNext = false;
    }
  }
  return result;
}

std::string toUpperSnakeCase(llvm::StringRef name) {
  std::string result;
  result.reserve(name.size() + 4);
  for (size_t i = 0; i < name.size(); ++i) {
    const char ch = name[i];
    const bool upper = ch >= 'A' && ch <= 'Z';
    const bool lower = ch >= 'a' && ch <= 'z';
    const bool digit = ch >= '0' && ch <= '9';
    if (upper && i > 0 && !result.empty() && result.back() != '_') {
      const char prev = name[i - 1];
      const bool prevLower = prev >= 'a' && prev <= 'z';
      if (prevLower) {
        result.push_back('_');
      }
    }
    if (lower) {
      result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    } else if (upper || digit) {
      result.push_back(ch);
    } else if (ch == '_' || ch == '-') {
      if (!result.empty() && result.back() != '_') {
        result.push_back('_');
      }
    }
  }
  while (!result.empty() && result.back() == '_') {
    result.pop_back();
  }
  return result;
}

bool isConstGlobalOrStatic(const clang::VarDecl *var) {
  if (var == nullptr) {
    return false;
  }
  if (!var->hasGlobalStorage()) {
    return false;
  }
  const clang::QualType type = var->getType();
  if (!type.isConstQualified() && !var->isConstexpr()) {
    return false;
  }
  return true;
}

} // namespace

void NamingConventionCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      cxxRecordDecl(
          isDefinition(),
          unless(isImplicit()),
          unless(isExpansionInSystemHeader()))
          .bind("record"),
      this);

  finder->addMatcher(
      enumDecl(unless(isExpansionInSystemHeader()),
               unless(isImplicit()))
          .bind("enum"),
      this);

  finder->addMatcher(
      functionDecl(
          unless(cxxConstructorDecl()),
          unless(cxxDestructorDecl()),
          unless(isImplicit()),
          unless(isExpansionInSystemHeader()))
          .bind("func"),
      this);

  finder->addMatcher(
      varDecl(unless(isExpansionInSystemHeader()),
              unless(parmVarDecl()),
              unless(isImplicit()))
          .bind("var"),
      this);
}

void NamingConventionCheck::check(const MatchFinder::MatchResult &result) {
  const auto &sourceManager = *result.SourceManager;

  if (const auto *record =
          result.Nodes.getNodeAs<clang::CXXRecordDecl>("record")) {
    if (!common::isInMainFile(record, sourceManager)) {
      return;
    }
    if (record->isLambda() || record->isAnonymousStructOrUnion() ||
        record->getNameAsString().empty()) {
      return;
    }
    const std::string name = record->getNameAsString();

    if (common::looksLikeInterface(record)) {
      if (!common::hasInterfacePrefix(name)) {
        diag(record->getLocation(),
             "interface-like class %0 should start with 'I' (e.g. %1)")
            << name << ("I" + toPascalCase(name));
        return;
      }
      return;
    }

    if (!common::isPascalCase(name)) {
      const char *kind = record->isStruct() ? "struct" : "class";
      diag(record->getLocation(),
           "%0 %1 should use PascalCase (e.g. %2)")
          << kind << name << toPascalCase(name);
    }
    return;
  }

  if (const auto *enumeration =
          result.Nodes.getNodeAs<clang::EnumDecl>("enum")) {
    if (!common::isInMainFile(enumeration, sourceManager)) {
      return;
    }
    const std::string name = enumeration->getNameAsString();
    if (name.empty()) {
      return;
    }
    if (!common::isPascalCase(name)) {
      diag(enumeration->getLocation(),
           "enum %0 should use PascalCase (e.g. %1)")
          << name << toPascalCase(name);
    }
    return;
  }

  if (const auto *func =
          result.Nodes.getNodeAs<clang::FunctionDecl>("func")) {
    if (!common::isInMainFile(func, sourceManager)) {
      return;
    }
    if (func->isOverloadedOperator() || func->isMain()) {
      return;
    }
    if (const auto *method = clang::dyn_cast<clang::CXXMethodDecl>(func)) {
      if (method->getAccess() == clang::AS_private ||
          method->getAccess() == clang::AS_protected) {
        return;
      }
    }
    const std::string name = func->getNameAsString();
    if (name.empty()) {
      return;
    }
    if (!common::isPascalCase(name)) {
      diag(func->getLocation(),
           "public method %0 should use PascalCase (e.g. %1)")
          << name << toPascalCase(name);
    }
    return;
  }

  if (const auto *var = result.Nodes.getNodeAs<clang::VarDecl>("var")) {
    if (!common::isInMainFile(var, sourceManager)) {
      return;
    }
    if (!isConstGlobalOrStatic(var)) {
      return;
    }
    const std::string name = var->getNameAsString();
    if (name.empty()) {
      return;
    }
    if (!common::isUpperSnakeCase(name)) {
      diag(var->getLocation(),
           "constant %0 should use UPPER_SNAKE_CASE (e.g. %1)")
          << name << toUpperSnakeCase(name);
    }
    return;
  }
}

} // namespace clang::tidy::lenovo::naming
