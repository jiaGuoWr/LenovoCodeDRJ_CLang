#include "StackTraceInCatchCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::reliability {

void StackTraceInCatchCheck::registerMatchers(MatchFinder *finder) {
  // Calls to backtrace_symbols / backtrace_symbols_fd inside any catch.
  finder->addMatcher(
      cxxCatchStmt(forEachDescendant(
          callExpr(
              callee(functionDecl(hasAnyName(
                  "backtrace", "backtrace_symbols", "backtrace_symbols_fd",
                  "abi::__cxa_demangle"))),
              unless(isExpansionInSystemHeader()))
              .bind("trace"))),
      this);

  // Calls to fprintf(stderr, ...) inside any catch.
  finder->addMatcher(
      cxxCatchStmt(forEachDescendant(
          callExpr(
              callee(functionDecl(hasAnyName(
                  "fprintf", "vfprintf", "fputs", "fwrite"))),
              unless(isExpansionInSystemHeader()))
              .bind("printerr"))),
      this);
}

void StackTraceInCatchCheck::check(const MatchFinder::MatchResult &result) {
  const auto &sm = *result.SourceManager;
  if (const auto *trace = result.Nodes.getNodeAs<clang::CallExpr>("trace")) {
    if (!sm.isInMainFile(sm.getExpansionLoc(trace->getBeginLoc()))) return;
    diag(trace->getBeginLoc(),
         "printing stack trace from catch may leak internal structure; "
         "use structured logging");
    return;
  }
  if (const auto *p = result.Nodes.getNodeAs<clang::CallExpr>("printerr")) {
    if (!sm.isInMainFile(sm.getExpansionLoc(p->getBeginLoc()))) return;
    diag(p->getBeginLoc(),
         "printing exception message to stderr from catch may leak internal "
         "structure; use structured logging");
  }
}

} // namespace clang::tidy::lenovo::reliability
