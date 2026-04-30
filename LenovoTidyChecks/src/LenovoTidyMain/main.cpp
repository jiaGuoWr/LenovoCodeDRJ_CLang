//===- main.cpp - lenovo-clang-tidy entry point ---------------------------===//
//
// A standalone clang-tidy executable that statically links every upstream
// clang-tidy module plus the Lenovo DRJ custom checks (lenovo_tidy_static).
//
// Rationale: LLVM's official Windows binaries ship with plugin loading
// effectively disabled (clang-tidy.exe is fully statically linked, so the
// ClangTidyModuleRegistry inside any -load=...dll is a separate copy that
// the executable never reads). Bundling the rules directly into a custom
// executable sidesteps the issue and gives the same UX on every platform.
//
//===----------------------------------------------------------------------===//

#include <clang-tidy/ClangTidyForceLinker.h>
#include <clang-tidy/tool/ClangTidyMain.h>

namespace clang::tidy {

// LenovoTidyModule.cpp defines the matching `volatile int
// LenovoTidyModuleAnchorSource = 0;` so that the registry's static initializer
// is preserved by the linker even when the rule sources land in a static
// archive that would otherwise be eligible for dead-stripping.
extern volatile int LenovoTidyModuleAnchorSource;
static int LLVM_ATTRIBUTE_UNUSED LenovoTidyModuleAnchorDestination =
    LenovoTidyModuleAnchorSource;

} // namespace clang::tidy

int main(int argc, const char **argv) {
  return clang::tidy::clangTidyMain(argc, argv);
}
