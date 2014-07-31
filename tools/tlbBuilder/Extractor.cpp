//  vim: set foldmethod=marker:
//  Usage:
//  tool-template <header-to-look-at> -- <compile-flags-as-usual>
//
//  Example:
//  ./bin/extractor $ROCK_ROOT/base/types/base/Pose.hpp -- -I$ROCK_ROOT/base/types -I/usr/include/eigen3 -x c++
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

    virtual void run(const MatchFinder::MatchResult &Result) {

        const TypedefType *T = Result.Nodes.getNodeAs<TypedefType>("typeDef");

        if(T) {
            
            if (!Result.SourceManager->isInMainFile(T->getDecl()->getTypeSourceInfo()->getTypeLoc().getLocStart())) {
            std::cerr << " -- skipping '"
                        << T->getDecl()->getQualifiedNameAsString()
                        << "' because its not in main header\n";
                return;
            }

            
            builder.registerTypeDef(T);
        }
    }
};
    
class TypeDeclCallback : public MatchFinder::MatchCallback {
    public:

    virtual void run(const MatchFinder::MatchResult &Result) {

        // check the different possibilities of a match: "recordDecl" or
        // "typeDecl", and handle them accordingly. recordDecls are also
        // typeDecls, so prevent adding them both!
        if (const CXXRecordDecl *D = Result.Nodes.getNodeAs<CXXRecordDecl>("typeDecl")) {

            // this removes the bodus double-match of "nested_records::S2::S2"
            // for "struct nested_records::S2". it it not needed by us.
            if (D->isInjectedClassName()) {
                std::cerr << " -- skipping '"
                          << D->getQualifiedNameAsString()
                          << "' because its injected classname\n";
                return;
            }

            // this prohibits adding "struct __locale_struct" to the database
            // ust becuase it's declaration. if it is needed inside one of the
            // registered structs, we'll hit fast and hit hard by adding it to
            // the database.
            if (!Result.SourceManager->isInMainFile(D->getLocation())) {
                std::cerr << " -- skipping '"
                          << D->getQualifiedNameAsString()
                          << "' because its not in main header\n";
                return;
            }

            builder.registerNamedDecl(D);

        } else if(const TypeDecl *D = Result.Nodes.getNodeAs<TypeDecl>("typeDecl")) {

            if (!Result.SourceManager->isInMainFile(D->getLocation())) {
                std::cerr << " -- skipping '"
                            << D->getQualifiedNameAsString()
                            << "' because its not in main header\n";
                return;
            }
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

  TypeDeclCallback typeDeclCallback;
  internal::VariadicDynCastAllOfMatcher<Decl, TypeDecl> typeDecl;
  Finder.addMatcher(typeDecl().bind("typeDecl"), &typeDeclCallback);

  TypeDefCallback typeDefCallback;
  Finder.addMatcher(typedefType().bind("typeDef"), &typeDefCallback);
  
  if (int retval = Tool.run(newFrontendActionFactory(&Finder)) != 0) {
    std::cerr << "whoops\n";
    return retval;
  }
  
  builder.buildRegistry();
  
  return 0;
  
}
