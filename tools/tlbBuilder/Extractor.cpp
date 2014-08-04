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
#include "../../lang/tlb/export.hh"

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
//             std::cerr << " -- skipping '"
//                         << T->getDecl()->getQualifiedNameAsString()
//                         << "' because its not in main header\n";
                return;
            }

            
            builder.registerTypeDef(T);
        }
    }
};

class OpaqueCallback : public MatchFinder::MatchCallback {
    public:

    virtual void run(const MatchFinder::MatchResult &Result) {

        std::cout << "Found my Opaque" << std::endl;
        
   
        if(const TypeDecl *D = Result.Nodes.getNodeAs<TypeDecl>("typeDecl")) {

            builder.lookupOpaque(D);
            
        }
        else
        {
            std::cout << "WEIRED" << std::endl;
            exit(1);
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
//                 std::cerr << " -- skipping '"
//                           << D->getQualifiedNameAsString()
//                           << "' because its injected classname\n";
                return;
            }

            // this prohibits adding "struct __locale_struct" to the database
            // ust becuase it's declaration. if it is needed inside one of the
            // registered structs, we'll hit fast and hit hard by adding it to
            // the database.
            if (!Result.SourceManager->isInMainFile(D->getLocation())) {
//                 std::cerr << " -- skipping '"
//                           << D->getQualifiedNameAsString()
//                           << "' because its not in main header\n";
                return;
            }

            builder.registerNamedDecl(D);

        } else if(const TypeDecl *D = Result.Nodes.getNodeAs<TypeDecl>("typeDecl")) {

            if (!Result.SourceManager->isInMainFile(D->getLocation())) {
//                 std::cerr << " -- skipping '"
//                             << D->getQualifiedNameAsString()
//                             << "' because its not in main header\n";
                return;
            }
            builder.registerNamedDecl(D);

        }
    }

};
} // end anonymous namespace

static llvm::cl::opt<std::string> opaquePath(
        "opaquePath",
        llvm::cl::desc("A registry of opaques, which are defined in the given header files. These Types need to be resolved to their canonical names before the main run of the parser."),
        llvm::cl::Optional);

static llvm::cl::opt<std::string> tlbSavePath(
        "tlbSavePath",
        llvm::cl::desc("file of where to save resulting tlb-database. printed to stdout if not given"),
        llvm::cl::Optional);

int main(int argc, const char **argv) {

    // optparsing {{{1
    llvm::sys::PrintStackTraceOnErrorSignal();
    
    
    CommonOptionsParser OptionsParser(argc, argv);
    ClangTool Tool(OptionsParser.getCompilations(),
                    OptionsParser.getSourcePathList());
    // }}}

    //load opque registry
    if(!opaquePath.empty())
    {
        std::cout << "Loading opaque tlb-registry from '" << opaquePath << "'" << std::endl;
        builder.loadRegistry(opaquePath);

        //resolve opaues to canonical names
        ast_matchers::MatchFinder OpaqueFinder;
        internal::VariadicDynCastAllOfMatcher<Decl, TypeDecl> oTypeDecl;
        OpaqueCallback opaqueCallback;

        for(Typelib::Registry::Iterator it = builder.getRegistry().begin(); it != builder.getRegistry().end(); it++)
        {
            if(it->getCategory() == Typelib::Type::Opaque)
            {
                OpaqueFinder.addMatcher(oTypeDecl(hasName(builder.typlibtoCxxName(it->getName()))).bind("typeDecl"), &opaqueCallback);
            }
        }

        if (int retval = Tool.run(newFrontendActionFactory(&OpaqueFinder))) {
            std::cerr << "Parsing error in clang, cannot continue" << std::endl;
            return retval;
        }
    }

    ast_matchers::MatchFinder Finder;

    TypeDeclCallback typeDeclCallback;
    internal::VariadicDynCastAllOfMatcher<Decl, TypeDecl> typeDecl;
    Finder.addMatcher(typeDecl().bind("typeDecl"), &typeDeclCallback);

        TypeDefCallback typeDefCallback;
    Finder.addMatcher(typedefType().bind("typeDef"), &typeDefCallback);
    
    if (int retval = Tool.run(newFrontendActionFactory(&Finder))) {
        std::cerr << "Parsing error in clang, cannot continue" << std::endl;
        return retval;
    }

    TlbExport exporter;
    if(!tlbSavePath.empty())
    {
        std::cout << "Saving tlb-registry into file '" << tlbSavePath << "'" << std::endl;
        exporter.save(tlbSavePath, utilmm::config_set(), builder.getRegistry());
    } else {
        exporter.save(std::cout, utilmm::config_set(), builder.getRegistry());
    }
    
    return 0;
  
}
