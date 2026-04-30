#include "UnsafeDeserializationCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace clang::tidy::lenovo::security {

void UnsafeDeserializationCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(
      cxxConstructExpr(
          hasType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(
              cxxRecordDecl(hasAnyName(
                  "::boost::archive::binary_iarchive",
                  "::boost::archive::text_iarchive",
                  "::boost::archive::xml_iarchive",
                  "::cereal::BinaryInputArchive",
                  "::cereal::JSONInputArchive",
                  "::cereal::PortableBinaryInputArchive",
                  "::cereal::XMLInputArchive",
                  "::msgpack::unpacker")))))),
          unless(isExpansionInSystemHeader()))
          .bind("ctor"),
      this);
}

void UnsafeDeserializationCheck::check(const MatchFinder::MatchResult &result) {
  const auto *ctor = result.Nodes.getNodeAs<clang::CXXConstructExpr>("ctor");
  if (ctor == nullptr) {
    return;
  }
  if (!result.SourceManager->isInMainFile(
          result.SourceManager->getExpansionLoc(ctor->getBeginLoc()))) {
    return;
  }
  const clang::CXXRecordDecl *rec = ctor->getConstructor()->getParent();
  diag(ctor->getBeginLoc(),
       "use of unsafe deserialization API %0; validate or migrate to a "
       "sandboxed format")
      << rec->getName();
}

} // namespace clang::tidy::lenovo::security
