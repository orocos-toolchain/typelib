//  vim: set foldmethod=marker:
//  Usage:
//  tool-template <header-to-look-at> -- <compile-flags-as-usual>
//
//  Example:
//  ./bin/extractor $ROCK_ROOT/base/types/base/Pose.hpp -- \
//      -I$ROCK_ROOT/base/types \
//      -I/usr/include/eigen3 \
//      -x c++
//
//  keep in mind that this particular (not very complicated) example still takes 15seconds to
//  complete...
//

// include {{{1
#include <clang/Tooling/CommonOptionsParser.h>
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Type.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Signals.h"
#include "TypelibBuilder.hpp"

#include <iostream>
// }}}

// using namespace {{{1
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;
// }}}

TypelibBuilder builder;

namespace {

class TypeDefCallback : public MatchFinder::MatchCallback {
    public:

        
//  This routine will get called for each thing that the matchers find.
virtual void run(const MatchFinder::MatchResult &Result) {

    const TypedefType *typeType = Result.Nodes.getNodeAs<TypedefType>("typeDef");
    if(typeType)
    {
        builder.registerTypeDef(typeType);
    }
}
};
    
class ToolTemplateCallback : public MatchFinder::MatchCallback {
 public:
  //  This routine will get called for each thing that the matchers find.
  virtual void run(const MatchFinder::MatchResult &Result) {

      const TypeDecl *decl = Result.Nodes.getNodeAs<TypeDecl>("match");

      if(decl)
      {
          builder.registerNamedDecl(decl);
      }
      
    const CXXRecordDecl *D = Result.Nodes.getNodeAs<CXXRecordDecl>("match");
    if (D) {
        builder.registerNamedDecl(D);
    }
  }

};
} // end anonymous namespace

int main(int argc, const char **argv) {

  // optparsing {{{1
  llvm::sys::PrintStackTraceOnErrorSignal();
  CommonOptionsParser OptionsParser(argc, argv);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());
  // }}}

  ast_matchers::MatchFinder Finder;
  ToolTemplateCallback Callback;
  TypeDefCallback tdCallback;

  // AST matching ftw...
  //
  // the big table: http://clang.llvm.org/docs/LibASTMatchersReference.html

  // the "bind" will make the match referencable by the given string in the "run()" mathod of the
  // callback

  // the "isDefinition()" is needed to reject "Class Name Injection" and forward
  // declarations. see https://stackoverflow.com/questions/24761684 and
  // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/1994/N0444.pdf
  internal::VariadicDynCastAllOfMatcher<Decl, TypeDecl> typeDecl;
  
  DeclarationMatcher matcher = typeDecl().bind("match");

  Finder.addMatcher(matcher, &Callback);

  Finder.addMatcher(typedefType().bind("typeDef"), &tdCallback);
  
  Tool.run(newFrontendActionFactory(&Finder));
  
  builder.buildRegistry();
  
  return 0;
  
}
