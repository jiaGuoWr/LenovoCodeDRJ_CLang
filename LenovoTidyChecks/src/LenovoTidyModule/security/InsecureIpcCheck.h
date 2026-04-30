//===- InsecureIpcCheck.h - SEC011 ---------------------------------------===//
//
// Detects construction of plaintext / explicitly-insecure IPC primitives:
//   - grpc::InsecureChannelCredentials / InsecureServerCredentials
//   - zmq plain text bind without curve_server (future)
//
// Aligned with C# analyzer rule SEC011.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::lenovo::security {

class InsecureIpcCheck : public clang::tidy::ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &result) override;
};

} // namespace clang::tidy::lenovo::security
