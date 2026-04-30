//===- OptionParser.h - Tiny helpers for clang-tidy .Options -------------===//
#pragma once

#include <clang-tidy/ClangTidyCheck.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

#include <string>
#include <vector>

namespace clang::tidy::lenovo::common {

/// Splits a comma/semicolon separated option string into a vector of trimmed
/// entries. Empty entries are skipped. Whitespace surrounding each entry is
/// stripped.
std::vector<std::string> splitOption(llvm::StringRef value);

/// Reads a `.clang-tidy` option that is a comma-separated list and falls back
/// to \p defaultValue if the option is missing. Returns the parsed list.
std::vector<std::string>
readListOption(const clang::tidy::ClangTidyCheck::OptionsView &options,
               llvm::StringRef name, llvm::StringRef defaultValue);

} // namespace clang::tidy::lenovo::common
