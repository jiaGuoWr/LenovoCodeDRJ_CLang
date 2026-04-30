//===- LenovoTidyModule.cpp - Lenovo DRJ custom checks registry -----------===//
//
// Part of LenovoTidyChecks, an out-of-tree clang-tidy plugin that mirrors
// the Lenovo Direnjie C# Roslyn analyzer rule set for C++ codebases.
//
//===----------------------------------------------------------------------===//

#include "localization/ChineseInCommentsCheck.h"
#include "maintainability/CommentedOutCodeCheck.h"
#include "naming/NamingConventionCheck.h"
#include "reliability/StackTraceInCatchCheck.h"
#include "security/HardcodedSensitiveInfoCheck.h"
#include "security/InsecureIpcCheck.h"
#include "security/InsecureRandomCheck.h"
#include "security/InsecureTempFileCheck.h"
#include "security/PathTraversalCheck.h"
#include "security/RaceConditionCheck.h"
#include "security/RegexDosCheck.h"
#include "security/ResourceLeakCheck.h"
#include "security/SqlInjectionCheck.h"
#include "security/UnsafeDeserializationCheck.h"
#include "security/UnsafeDllLoadCheck.h"

#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

namespace clang::tidy::lenovo {

class LenovoTidyModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &factories) override {
    // Security
    factories.registerCheck<security::HardcodedSensitiveInfoCheck>(
        "lenovo-sec001-hardcoded-sensitive");
    factories.registerCheck<security::PathTraversalCheck>(
        "lenovo-sec002-path-traversal");
    factories.registerCheck<security::SqlInjectionCheck>(
        "lenovo-sec003-sql-injection");
    factories.registerCheck<security::UnsafeDeserializationCheck>(
        "lenovo-sec004-unsafe-deserialization");
    factories.registerCheck<security::InsecureRandomCheck>(
        "lenovo-sec005-insecure-random");
    factories.registerCheck<security::RegexDosCheck>(
        "lenovo-sec006-regex-dos-risk");
    factories.registerCheck<security::ResourceLeakCheck>(
        "lenovo-sec007-resource-leak");
    factories.registerCheck<security::InsecureTempFileCheck>(
        "lenovo-sec008-insecure-temp-file");
    factories.registerCheck<security::RaceConditionCheck>(
        "lenovo-sec010-race-condition");
    factories.registerCheck<security::InsecureIpcCheck>(
        "lenovo-sec011-insecure-ipc");
    factories.registerCheck<security::UnsafeDllLoadCheck>(
        "lenovo-dll003-unsafe-dll-load");

    // Localization
    factories.registerCheck<localization::ChineseInCommentsCheck>(
        "lenovo-chn001-chinese-comments");

    // Naming
    factories.registerCheck<naming::NamingConventionCheck>(
        "lenovo-name001-naming-convention");

    // Reliability
    factories.registerCheck<reliability::StackTraceInCatchCheck>(
        "lenovo-exc001-stack-trace-in-catch");

    // Maintainability
    factories.registerCheck<maintainability::CommentedOutCodeCheck>(
        "lenovo-code001-commented-out-code");
  }
};

} // namespace clang::tidy::lenovo

namespace clang::tidy {

static ClangTidyModuleRegistry::Add<lenovo::LenovoTidyModule>
    X("lenovo-module",
      "Adds Lenovo DRJ custom checks (CHN/SEC/NAME/EXC/CODE/DLL) aligned "
      "with the existing C# Roslyn analyzer rule set.");

// Pull in the registration so that the static global is not stripped when
// this translation unit is linked into a shared library.
volatile int LenovoTidyModuleAnchorSource = 0;

} // namespace clang::tidy
