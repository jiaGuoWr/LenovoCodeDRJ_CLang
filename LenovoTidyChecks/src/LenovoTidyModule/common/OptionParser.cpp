#include "OptionParser.h"

namespace clang::tidy::lenovo::common {

std::vector<std::string> splitOption(llvm::StringRef value) {
  std::vector<std::string> result;
  while (!value.empty()) {
    auto pair = value.split(',');
    if (pair.first.contains(';')) {
      pair = value.split(';');
    }
    llvm::StringRef entry = pair.first.trim();
    if (!entry.empty()) {
      result.emplace_back(entry.str());
    }
    value = pair.second;
  }
  return result;
}

std::vector<std::string>
readListOption(const clang::tidy::ClangTidyCheck::OptionsView &options,
               llvm::StringRef name, llvm::StringRef defaultValue) {
  const std::string raw = options.get(name, defaultValue.str()).str();
  return splitOption(raw);
}

} // namespace clang::tidy::lenovo::common
