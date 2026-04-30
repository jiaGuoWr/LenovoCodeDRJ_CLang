//===- StringUtils.h - UTF-8 / CJK helpers -------------------------------===//
#pragma once

#include <llvm/ADT/StringRef.h>

#include <cstdint>

namespace clang::tidy::lenovo::common {

/// Decode one UTF-8 code point starting at \p data (of length \p size).
///
/// \returns the number of bytes consumed, or 0 on malformed input.
/// The decoded code point is written to \p codePointOut when non-null.
size_t decodeUtf8(const char *data, size_t size, uint32_t *codePointOut);

/// Returns true if \p codePoint is a CJK (Chinese / Japanese / Korean) Unicode
/// character. Covers the major contiguous ranges sufficient to reliably detect
/// Chinese characters in source code. See docs/rules/chn001.md for the full
/// list of ranges considered.
bool isCjkCodePoint(uint32_t codePoint);

/// Returns true if the UTF-8 encoded \p text contains at least one CJK
/// character. Malformed bytes are ignored.
bool containsCjk(llvm::StringRef text);

/// Returns the column offset (0-based, in UTF-8 bytes) of the first CJK
/// character in \p text, or llvm::StringRef::npos if none is present.
size_t findFirstCjk(llvm::StringRef text);

/// Returns true if \p name follows PascalCase
///   - starts with an uppercase ASCII letter
///   - contains only alphanumerics
///   - no underscores
///   - no consecutive uppercase segments wider than 4 (common acronym)
bool isPascalCase(llvm::StringRef name);

/// Returns true if \p name is ALL_UPPER_SNAKE_CASE (A-Z, 0-9, _).
bool isUpperSnakeCase(llvm::StringRef name);

/// Returns true if \p name has an I prefix followed by PascalCase content,
/// e.g. "IUserService" -> true, "Interface" -> false, "Iuser" -> false.
bool hasInterfacePrefix(llvm::StringRef name);

} // namespace clang::tidy::lenovo::common
