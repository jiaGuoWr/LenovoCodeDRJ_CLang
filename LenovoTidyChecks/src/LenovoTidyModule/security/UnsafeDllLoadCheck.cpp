#include "UnsafeDllLoadCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <llvm/ADT/StringRef.h>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

namespace {

bool looksAbsolute(llvm::StringRef path) {
  if (path.empty()) return false;
  if (path.front() == '/') return true;                             // POSIX
  if (path.size() >= 3 && path[1] == ':' &&
      (path[2] == '/' || path[2] == '\\')) return true;             // C:\
  if (path.startswith("\\\\") || path.startswith("\\?\\")) return true;
  return false;
}

const clang::StringLiteral *firstStringLiteral(const clang::CallExpr *call) {
  if (call == nullptr || call->getNumArgs() == 0) return nullptr;
  const clang::Expr *arg = call->getArg(0)->IgnoreParenImpCasts();
  return clang::dyn_cast<clang::StringLiteral>(arg);
}

} // namespace

void UnsafeDllLoadCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      callExpr(callee(functionDecl(hasAnyName(
                   "LoadLibraryA", "LoadLibraryW", "LoadLibrary",
                   "LoadLibraryExA", "LoadLibraryExW", "LoadLibraryEx",
                   "dlopen", "::dlopen"))),
               unless(isExpansionInSystemHeader()))
          .bind("call"),
      this);
}

void UnsafeDllLoadCheck::check(const MatchFinder::MatchResult &result) {
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

  const clang::StringLiteral *literal = firstStringLiteral(call);
  if (literal != nullptr && literal->getCharByteWidth() == 1) {
    if (looksAbsolute(literal->getString())) {
      return;
    }
  } else if (literal == nullptr) {
    // Non-literal arg => can't tell, conservatively flag
  }

  diag(call->getBeginLoc(),
       "dynamic library load %0 without an absolute path; vulnerable to DLL "
       "hijacking")
      << fd->getName();
}

} // namespace clang::tidy::lenovo::security
