"""lit configuration for LenovoTidyChecks regression tests.

Each test source contains a RUN line that invokes %check_clang_tidy with the
rule name under test. %check_clang_tidy is a shell one-liner that:
  1. Runs clang-tidy on the test source loading our plugin.
  2. Pipes the diagnostics to FileCheck against the embedded CHECK-MESSAGES.
"""

import os
import lit.formats
import lit.llvm

config.test_format = lit.formats.ShTest(execute_external=False)

config.environment["LENOVO_TIDY_PLUGIN"] = config.lenovo_plugin

# Build the %check_clang_tidy substitution. The token stream here intentionally
# mirrors the upstream LLVM test helper so future migration to the in-tree
# harness is mechanical.
check_clang_tidy = (
    "%s -load=%s --checks=-*,%%1 %%s -- -std=c++17 -xc++ "
    "2>&1 | %s --match-full-lines --check-prefixes=CHECK-MESSAGES "
    "-implicit-check-not='{{warning|error}}:'"
) % (config.clang_tidy, config.lenovo_plugin, config.filecheck)

config.substitutions.append(("%check_clang_tidy", check_clang_tidy))
config.substitutions.append(("%clang_tidy",       config.clang_tidy))
config.substitutions.append(("%lenovo_plugin",    config.lenovo_plugin))

config.available_features.add("lenovo-tidy-plugin")
