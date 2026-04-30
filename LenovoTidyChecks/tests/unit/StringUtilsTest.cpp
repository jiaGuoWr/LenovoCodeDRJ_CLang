#include "common/StringUtils.h"

#include <gtest/gtest.h>

namespace lenovo = clang::tidy::lenovo::common;

TEST(StringUtils, DecodeAsciiSingleByte) {
  uint32_t codePoint = 0;
  const size_t consumed = lenovo::decodeUtf8("A", 1, &codePoint);
  EXPECT_EQ(consumed, 1u);
  EXPECT_EQ(codePoint, static_cast<uint32_t>('A'));
}

TEST(StringUtils, DecodeChineseThreeBytes) {
  // U+4E2D '中' encoded as 0xE4 0xB8 0xAD
  const char data[] = {static_cast<char>(0xE4), static_cast<char>(0xB8),
                       static_cast<char>(0xAD), '\0'};
  uint32_t codePoint = 0;
  const size_t consumed = lenovo::decodeUtf8(data, 3, &codePoint);
  EXPECT_EQ(consumed, 3u);
  EXPECT_EQ(codePoint, 0x4E2Du);
  EXPECT_TRUE(lenovo::isCjkCodePoint(codePoint));
}

TEST(StringUtils, ContainsCjkDetectsChineseInComment) {
  EXPECT_TRUE(lenovo::containsCjk("这是一个中文注释"));
  EXPECT_TRUE(lenovo::containsCjk("hello 世界"));
  EXPECT_FALSE(lenovo::containsCjk("plain ascii only"));
  EXPECT_FALSE(lenovo::containsCjk(""));
}

TEST(StringUtils, FindFirstCjkReturnsByteOffset) {
  // "ab中" -> 'a'(0), 'b'(1), '中' starts at 2.
  EXPECT_EQ(lenovo::findFirstCjk("ab中"), 2u);
  EXPECT_EQ(lenovo::findFirstCjk("abc"), llvm::StringRef::npos);
}

TEST(StringUtils, IsPascalCase) {
  EXPECT_TRUE(lenovo::isPascalCase("UserService"));
  EXPECT_TRUE(lenovo::isPascalCase("HttpClient"));
  EXPECT_TRUE(lenovo::isPascalCase("A"));
  EXPECT_FALSE(lenovo::isPascalCase(""));
  EXPECT_FALSE(lenovo::isPascalCase("userService"));
  EXPECT_FALSE(lenovo::isPascalCase("user_service"));
  EXPECT_FALSE(lenovo::isPascalCase("User-Service"));
}

TEST(StringUtils, IsUpperSnakeCase) {
  EXPECT_TRUE(lenovo::isUpperSnakeCase("MAX_SIZE"));
  EXPECT_TRUE(lenovo::isUpperSnakeCase("DEFAULT_TIMEOUT_MS"));
  EXPECT_TRUE(lenovo::isUpperSnakeCase("PI"));
  EXPECT_FALSE(lenovo::isUpperSnakeCase(""));
  EXPECT_FALSE(lenovo::isUpperSnakeCase("MaxSize"));
  EXPECT_FALSE(lenovo::isUpperSnakeCase("max_size"));
  EXPECT_FALSE(lenovo::isUpperSnakeCase("123"));
}

TEST(StringUtils, HasInterfacePrefix) {
  EXPECT_TRUE(lenovo::hasInterfacePrefix("IUserService"));
  EXPECT_TRUE(lenovo::hasInterfacePrefix("IDisposable"));
  EXPECT_FALSE(lenovo::hasInterfacePrefix("Interface"));
  EXPECT_FALSE(lenovo::hasInterfacePrefix("IUser_Service"));
  EXPECT_FALSE(lenovo::hasInterfacePrefix("I"));
  EXPECT_FALSE(lenovo::hasInterfacePrefix("UserService"));
  EXPECT_FALSE(lenovo::hasInterfacePrefix("Iuser"));
}
