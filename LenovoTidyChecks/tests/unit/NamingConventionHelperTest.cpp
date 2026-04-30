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
  EXPECT_FALSE(lenovo::hasInterfacePrefix("IOStream"));  // acceptable but
                                                         // still matches I+Pascal
  // The above is intentionally true — it is the I+PascalCase form. We document
  // this as a limitation in docs/rules/name001.md.
  EXPECT_TRUE(lenovo::hasInterfacePrefix("IOStream"));
}
