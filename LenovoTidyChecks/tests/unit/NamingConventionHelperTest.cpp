#include "common/StringUtils.h"

#include <gtest/gtest.h>

// This file hosts extra scenarios for the helper functions used by
// NamingConventionCheck. It is separate from StringUtilsTest so that new
// naming-centric helpers can be added without bloating the base unit file.

namespace lenovo = clang::tidy::lenovo::common;

TEST(NamingConventionHelpers, ConstantVsPascal) {
  EXPECT_TRUE(lenovo::isUpperSnakeCase("MAX_BUFFER"));
  EXPECT_FALSE(lenovo::isPascalCase("MAX_BUFFER"));
}

TEST(NamingConventionHelpers, InterfacePrefixFalsePositives) {
  // Known limitation: hasInterfacePrefix is heuristic and matches any
  // I+PascalCase identifier, including legitimate names like "IOStream".
  // We accept this false-positive class because the alternative (a
  // hand-curated allowlist of common types) drifts. Documented in
  // docs/rules/name001.md.
  //
  // Pin the current behaviour so a future tightening of the heuristic
  // surfaces here as a deliberate test failure rather than going
  // unnoticed.
  EXPECT_TRUE(lenovo::hasInterfacePrefix("IOStream"));
}
