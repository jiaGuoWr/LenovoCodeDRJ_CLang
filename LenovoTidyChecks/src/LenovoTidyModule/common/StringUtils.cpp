#include "StringUtils.h"

#include <cctype>

namespace clang::tidy::lenovo::common {

size_t decodeUtf8(const char *data, size_t size, uint32_t *codePointOut) {
  if (size == 0) {
    return 0;
  }

  const auto byte0 = static_cast<uint8_t>(data[0]);
  uint32_t codePoint = 0;
  size_t consumed = 0;

  if ((byte0 & 0x80) == 0) {
    codePoint = byte0;
    consumed = 1;
  } else if ((byte0 & 0xE0) == 0xC0) {
    if (size < 2) {
      return 0;
    }
    const auto byte1 = static_cast<uint8_t>(data[1]);
    if ((byte1 & 0xC0) != 0x80) {
      return 0;
    }
    codePoint = (static_cast<uint32_t>(byte0 & 0x1F) << 6) |
                static_cast<uint32_t>(byte1 & 0x3F);
    consumed = 2;
  } else if ((byte0 & 0xF0) == 0xE0) {
    if (size < 3) {
      return 0;
    }
    const auto byte1 = static_cast<uint8_t>(data[1]);
    const auto byte2 = static_cast<uint8_t>(data[2]);
    if ((byte1 & 0xC0) != 0x80 || (byte2 & 0xC0) != 0x80) {
      return 0;
    }
    codePoint = (static_cast<uint32_t>(byte0 & 0x0F) << 12) |
                (static_cast<uint32_t>(byte1 & 0x3F) << 6) |
                static_cast<uint32_t>(byte2 & 0x3F);
    consumed = 3;
  } else if ((byte0 & 0xF8) == 0xF0) {
    if (size < 4) {
      return 0;
    }
    const auto byte1 = static_cast<uint8_t>(data[1]);
    const auto byte2 = static_cast<uint8_t>(data[2]);
    const auto byte3 = static_cast<uint8_t>(data[3]);
    if ((byte1 & 0xC0) != 0x80 || (byte2 & 0xC0) != 0x80 ||
        (byte3 & 0xC0) != 0x80) {
      return 0;
    }
    codePoint = (static_cast<uint32_t>(byte0 & 0x07) << 18) |
                (static_cast<uint32_t>(byte1 & 0x3F) << 12) |
                (static_cast<uint32_t>(byte2 & 0x3F) << 6) |
                static_cast<uint32_t>(byte3 & 0x3F);
    consumed = 4;
  } else {
    return 0;
  }

  if (codePointOut != nullptr) {
    *codePointOut = codePoint;
  }
  return consumed;
}

bool isCjkCodePoint(uint32_t codePoint) {
  // Only the most common / dense CJK ranges. Good enough to catch Chinese
  // characters in comments and string literals; false-positives for rare
  // extensions are acceptable and documented in docs/rules/chn001.md.
  return (codePoint >= 0x3000 && codePoint <= 0x303F) || // CJK Symbols & Punctuation
         (codePoint >= 0x3400 && codePoint <= 0x4DBF) || // CJK Unified Ext A
         (codePoint >= 0x4E00 && codePoint <= 0x9FFF) || // CJK Unified Ideographs
         (codePoint >= 0xF900 && codePoint <= 0xFAFF) || // CJK Compat Ideographs
         (codePoint >= 0xFF00 && codePoint <= 0xFFEF) || // Halfwidth/Fullwidth
         (codePoint >= 0x20000 && codePoint <= 0x2FFFF); // CJK Unified Ext B..F
}

bool containsCjk(llvm::StringRef text) {
  return findFirstCjk(text) != llvm::StringRef::npos;
}

size_t findFirstCjk(llvm::StringRef text) {
  const char *data = text.data();
  const size_t size = text.size();
  size_t offset = 0;

  while (offset < size) {
    uint32_t codePoint = 0;
    const size_t consumed = decodeUtf8(data + offset, size - offset, &codePoint);
    if (consumed == 0) {
      ++offset;
      continue;
    }
    if (isCjkCodePoint(codePoint)) {
      return offset;
    }
    offset += consumed;
  }
  return llvm::StringRef::npos;
}

bool isPascalCase(llvm::StringRef name) {
  if (name.empty()) {
    return false;
  }
  const char first = name.front();
  if (first < 'A' || first > 'Z') {
    return false;
  }
  for (char ch : name) {
    const bool upper = ch >= 'A' && ch <= 'Z';
    const bool lower = ch >= 'a' && ch <= 'z';
    const bool digit = ch >= '0' && ch <= '9';
    if (!(upper || lower || digit)) {
      return false;
    }
  }
  return true;
}

bool isUpperSnakeCase(llvm::StringRef name) {
  if (name.empty()) {
    return false;
  }
  bool sawLetter = false;
  for (char ch : name) {
    const bool upper = ch >= 'A' && ch <= 'Z';
    const bool digit = ch >= '0' && ch <= '9';
    const bool underscore = ch == '_';
    if (!(upper || digit || underscore)) {
      return false;
    }
    if (upper) {
      sawLetter = true;
    }
  }
  return sawLetter;
}

bool hasInterfacePrefix(llvm::StringRef name) {
  if (name.size() < 2) {
    return false;
  }
  if (name.front() != 'I') {
    return false;
  }
  const char second = name[1];
  if (!(second >= 'A' && second <= 'Z')) {
    return false;
  }
  return isPascalCase(name);
}

} // namespace clang::tidy::lenovo::common
