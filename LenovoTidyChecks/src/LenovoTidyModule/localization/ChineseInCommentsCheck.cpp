#include "ChineseInCommentsCheck.h"

#include "common/StringUtils.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/ADT/StringRef.h>

namespace clang::tidy::lenovo::localization {

namespace {

class CjkCommentHandler : public clang::CommentHandler {
public:
  explicit CjkCommentHandler(ChineseInCommentsCheck &parent) : parent_(parent) {}

  bool HandleComment(clang::Preprocessor &preprocessor,
                     clang::SourceRange range) override {
    const clang::SourceManager &sourceManager = preprocessor.getSourceManager();
    if (range.isInvalid()) {
      return false;
    }
    if (!sourceManager.isInMainFile(sourceManager.getExpansionLoc(range.getBegin()))) {
      return false;
    }

    bool invalid = false;
    const char *begin = sourceManager.getCharacterData(range.getBegin(), &invalid);
    if (invalid || begin == nullptr) {
      return false;
    }
    const char *end = sourceManager.getCharacterData(range.getEnd(), &invalid);
    if (invalid || end == nullptr || end < begin) {
      return false;
    }

    llvm::StringRef commentText(begin, static_cast<size_t>(end - begin));
    if (!common::containsCjk(commentText)) {
      return false;
    }

    parent_.diag(range.getBegin(), "comment contains Chinese characters");
    return false;
  }

private:
  ChineseInCommentsCheck &parent_;
};

} // namespace

void ChineseInCommentsCheck::registerPPCallbacks(
    const clang::SourceManager & /*sourceManager*/,
    clang::Preprocessor *preprocessor,
    clang::Preprocessor * /*moduleExpanderPP*/) {
  if (preprocessor == nullptr) {
    return;
  }
  preprocessor->addCommentHandler(new CjkCommentHandler(*this));
}

} // namespace clang::tidy::lenovo::localization
