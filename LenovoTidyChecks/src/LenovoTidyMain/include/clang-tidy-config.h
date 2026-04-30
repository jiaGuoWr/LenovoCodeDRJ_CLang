//===- clang-tidy-config.h - generated header substitute -----*- C++ -*----===//
//
// LLVM 18.1.8's official Windows SDK ships clang-tidy headers but omits the
// build-generated `clang-tidy-config.h`, which `ClangTidyForceLinker.h`
// transitively includes. We recreate it with the values used by the upstream
// release build so out-of-tree consumers can include
// <clang-tidy/ClangTidyForceLinker.h> on Windows the same way as on Linux.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CONFIG_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CONFIG_H

// The official binary release of clang-tidy on every platform enables the
// static analyzer. Mirror that here so the corresponding anchor symbols
// (MPIModuleAnchorSource, ...) are referenced from main.cpp.
#define CLANG_TIDY_ENABLE_STATIC_ANALYZER 1

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CONFIG_H
