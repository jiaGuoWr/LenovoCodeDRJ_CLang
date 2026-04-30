//===- UnsafeDeserializationCheck.h - SEC004 -----------------------------===//
//
// Detects construction of well-known unsafe deserialisation entry points:
//   - boost::archive::*_iarchive (binary / text / xml)
//   - cereal::*Archive
//   - msgpack::unpacker / object_handle from raw bytes
//
// Aligned with C# analyzer rule SEC004 (BinaryFormatter etc.).
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class UnsafeDeserializationCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
