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
//             std::cout << " -- skipping '"
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
            exit(EXIT_FAILURE);
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
//                 std::cout << " -- skipping '"
//                           << D->getQualifiedNameAsString()
//                           << "' because its injected classname\n";
                return;
            }

            // this prohibits adding "struct __locale_struct" to the database
            // ust becuase it's declaration. if it is needed inside one of the
            // registered structs, we'll hit fast and hit hard by adding it to
            // the database.
            if (!Result.SourceManager->isInMainFile(D->getLocation())) {
//                 std::cout << " -- skipping '"
//                           << D->getQualifiedNameAsString()
//                           << "' because its not in main header\n";
                return;
            }

            builder.registerNamedDecl(D);

        } else if(const TypeDecl *D = Result.Nodes.getNodeAs<TypeDecl>("typeDecl")) {

            if (!Result.SourceManager->isInMainFile(D->getLocation())) {
//                 std::cout << " -- skipping '"
//                             << D->getQualifiedNameAsString()
//                             << "' because its not in main header\n";
                return;
            }
            builder.registerNamedDecl(D);

        }
    }

};
} // end anonymous namespace

static llvm::cl::OptionCategory ToolCategory("typelib-clang-tlb-importer options");
static llvm::cl::opt<std::string> opaquePath(
        "opaquePath",
        llvm::cl::desc("registry of opaques, which have to be defined in the header files"),
        llvm::cl::cat(ToolCategory));

static llvm::cl::opt<std::string> tlbSavePath(
        "tlbSavePath",
        llvm::cl::desc("where to save tlb-database"),
        llvm::cl::Required,
        llvm::cl::cat(ToolCategory));

int main(int argc, const char **argv) {
    llvm::sys::PrintStackTraceOnErrorSignal();

    // optparsing {{{1

    // Hide unrelated options. This can be done nicely by the
    // CommonOptionsParser in later clang-releases
    StringMap<cl::Option *> Options;
    cl::getRegisteredOptions(Options);
    for (StringMap<cl::Option*>::const_iterator Option=Options.begin();
            Option!=Options.end();Option++)
        if (Option->second->Category != &ToolCategory &&
                Option->first() != "help" && Option->first() != "version")
            Option->second->setHiddenFlag(cl::ReallyHidden);

    // FIXME: using the "CommonOptionsParser" is bad -- if someone is giving
    // extra-compiler-options on the commandline and a build-path like:
    //
    //     typelib-clang-tlb-importer $HEADER -p=compile_commands.json -- -x c++
    //
    // the data from the compile-commands is overridden _override_ by extra
    // compiler options instead of augmenting them...

    CommonOptionsParser OptionsParser(
            argc, argv, "typelib-clang-tlb-importer: serialize/deserialize C++");
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
        OpaqueCallback opaqueCallback;

        for(Typelib::Registry::Iterator it = builder.getRegistry().begin(); it != builder.getRegistry().end(); it++)
        {
            if(it->getCategory() == Typelib::Type::Opaque)
            {
                Typelib::MetaData &mdata(it->getMetaData());
                mdata.add("found", "false");
                OpaqueFinder.addMatcher(namedDecl(hasName(builder.typlibtoCxxName(it->getName()))).bind("typeDecl"), &opaqueCallback);
            }
        }

        if (int retval = Tool.run(newFrontendActionFactory(&OpaqueFinder))) {
            std::cout << "Parsing error in clang, cannot continue" << std::endl;
            exit(retval);
        }
        
        bool opaquesFound = true;;
        for(Typelib::Registry::Iterator it = builder.getRegistry().begin(); it != builder.getRegistry().end(); it++)
        {
            if(it->getCategory() == Typelib::Type::Opaque)
            {
                Typelib::MetaData &mdata(it->getMetaData());
                if(mdata.get().count("found"))
                {
                    std::cout << "Error, opaque " << it->getName() << " could not be resolved " << std::endl;
                    opaquesFound = false;
                }
                OpaqueFinder.addMatcher(namedDecl(hasName(builder.typlibtoCxxName(it->getName()))).bind("typeDecl"), &opaqueCallback);
            }
        }
        if(!opaquesFound) {
            std::cout << "hello, boy..." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
    

    ast_matchers::MatchFinder Finder;

    TypeDeclCallback typeDeclCallback;
    Finder.addMatcher(namedDecl().bind("typeDecl"), &typeDeclCallback);

    TypeDefCallback typeDefCallback;
    Finder.addMatcher(typedefType().bind("typeDef"), &typeDefCallback);
    
    if (int retval = Tool.run(newFrontendActionFactory(&Finder))) {
        std::cout << "Parsing error in clang, cannot continue" << std::endl;
        exit(retval);
    }

    TlbExport exporter;
    std::cout << "Saving tlb-registry into file '" << tlbSavePath << "'" << std::endl;
    exporter.save(tlbSavePath, utilmm::config_set(), builder.getRegistry());
    
    exit(EXIT_SUCCESS);
}
