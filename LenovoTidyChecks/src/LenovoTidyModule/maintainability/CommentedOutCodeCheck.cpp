#include "CommentedOutCodeCheck.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/ADT/StringRef.h>

namespace clang::tidy::lenovo::maintainability {

namespace {

bool looksLikeCode(llvm::StringRef text) {
  // Strip the leading // or /* */ markers.
  llvm::StringRef body = text.ltrim();
  if (body.startswith("//")) body = body.drop_front(2);
  if (body.startswith("/*")) body = body.drop_front(2);
  if (body.endswith("*/")) body = body.drop_back(2);
  body = body.trim();

  if (body.empty() || body.size() < 4) {
    return false;
  }

  // Skip common documentation-style markers.
  static const llvm::StringRef kDocPrefixes[] = {
      "TODO", "FIXME", "XXX", "NOTE", "HACK", "@", "Copyright", "SPDX"};
  for (const auto &p : kDocPrefixes) {
    if (body.startswith(p)) {
      return false;
    }
  }

  bool hasSemicolon = body.contains(';');
  bool hasBraces    = body.contains('{') || body.contains('}');
  bool hasAssign    = body.contains(" = ") || body.contains("==");
  bool hasKeyword   = body.startswith("return ") || body.startswith("if ") ||
                      body.startswith("if(")     || body.startswith("for ") ||
                      body.startswith("for(")    || body.startswith("while ") ||
                      body.startswith("while(")  || body.startswith("var ") ||
                      body.startswith("auto ")   || body.startswith("int ") ||
                      body.startswith("std::");

  // Two or more "code-shaped" signals reduce false positives on prose.
  int signals = (hasSemicolon ? 1 : 0) + (hasBraces ? 1 : 0) +
                (hasAssign ? 1 : 0) + (hasKeyword ? 1 : 0);
  return signals >= 2;
}

class CodeCommentHandler : public clang::CommentHandler {
public:
  explicit CodeCommentHandler(CommentedOutCodeCheck &parent) : parent_(parent) {}

  bool HandleComment(clang::Preprocessor &preprocessor,
                     clang::SourceRange range) override {
    const clang::SourceManager &sm = preprocessor.getSourceManager();
    if (range.isInvalid()) return false;
    if (!sm.isInMainFile(sm.getExpansionLoc(range.getBegin()))) return false;

    bool invalid = false;
    const char *begin = sm.getCharacterData(range.getBegin(), &invalid);
    if (invalid || begin == nullptr) return false;
    const char *end = sm.getCharacterData(range.getEnd(), &invalid);
    if (invalid || end == nullptr || end < begin) return false;

    llvm::StringRef text(begin, static_cast<size_t>(end - begin));
    if (!looksLikeCode(text)) return false;

    parent_.diag(range.getBegin(),
                 "comment looks like commented-out code; use version control "
                 "instead");
    return false;
  }

private:
  CommentedOutCodeCheck &parent_;
};

} // namespace

void CommentedOutCodeCheck::registerPPCallbacks(
    const clang::SourceManager & /*sm*/,
    clang::Preprocessor *preprocessor,
    clang::Preprocessor * /*moduleExpanderPP*/) {
  if (preprocessor == nullptr) return;
  preprocessor->addCommentHandler(new CodeCommentHandler(*this));
}

} // namespace clang::tidy::lenovo::maintainability
