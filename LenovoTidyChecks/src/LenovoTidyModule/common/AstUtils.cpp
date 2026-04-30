#include "AstUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Basic/SourceManager.h>

namespace clang::tidy::lenovo::common {

bool isInSystemHeaderOrBuiltin(const clang::Decl *decl,
                               const clang::SourceManager &sourceManager) {
  if (decl == nullptr) {
    return true;
  }
  const clang::SourceLocation location = decl->getLocation();
  if (location.isInvalid()) {
    return true;
  }
  return sourceManager.isInSystemHeader(location) ||
         sourceManager.isInSystemMacro(location) ||
         !location.isFileID();
}

bool isInMainFile(const clang::Decl *decl,
                  const clang::SourceManager &sourceManager) {
  if (decl == nullptr) {
    return false;
  }
  const clang::SourceLocation location = decl->getLocation();
  if (location.isInvalid()) {
    return false;
  }
  return sourceManager.isInMainFile(
      sourceManager.getExpansionLoc(location));
}

bool looksLikeInterface(const clang::CXXRecordDecl *recordDecl) {
  if (recordDecl == nullptr || !recordDecl->isCompleteDefinition()) {
    return false;
  }
  if (recordDecl->isUnion() || recordDecl->isLambda()) {
    return false;
  }
  if (!recordDecl->field_empty()) {
    return false;
  }

  bool hasVirtualMethod = false;
  for (const clang::CXXMethodDecl *method : recordDecl->methods()) {
    if (method->isImplicit()) {
      continue;
    }
    if (clang::isa<clang::CXXConstructorDecl>(method) ||
        clang::isa<clang::CXXDestructorDecl>(method)) {
      continue;
    }
    if (!method->isVirtual()) {
      return false;
    }
    if (!method->isPureVirtual()) {
      return false;
    }
    hasVirtualMethod = true;
  }
  return hasVirtualMethod;
}

} // namespace clang::tidy::lenovo::common
