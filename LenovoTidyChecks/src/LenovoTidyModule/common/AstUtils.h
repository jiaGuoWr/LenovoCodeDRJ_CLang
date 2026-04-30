//===- AstUtils.h - Small helpers over Clang AST -------------------------===//
#pragma once

#include <clang/AST/Decl.h>
#include <llvm/ADT/StringRef.h>

namespace clang::tidy::lenovo::common {

/// Returns true if \p decl is declared in a system header or a builtin
/// location. Used to skip STL / platform headers during checks.
bool isInSystemHeaderOrBuiltin(const clang::Decl *decl,
                               const clang::SourceManager &sourceManager);

/// Returns true if \p decl lives in the main source file being analysed.
/// Rules typically only want to diagnose user code; use this to filter.
bool isInMainFile(const clang::Decl *decl,
                  const clang::SourceManager &sourceManager);

/// Returns true if \p recordDecl is intended to act as an "interface" in the
/// .NET sense: a record whose members are all pure virtual with no fields.
bool looksLikeInterface(const clang::CXXRecordDecl *recordDecl);

} // namespace clang::tidy::lenovo::common
