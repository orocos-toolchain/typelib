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
#include "NamingConversions.hpp"

#include "typelib/registryiterator.hh"

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

// first level: lookup typdefs named like entries from a tlb-database. then add
// aliases to the tlb-database to the resolved underlying type in the "builder"
// class OpaqueCallback {{{1
class OpaqueCallback : public MatchFinder::MatchCallback {
  public:
    virtual void run(const MatchFinder::MatchResult &Result) {

        if (const TypeDecl *decl = Result.Nodes.getNodeAs<TypeDecl>("opaque")) {
            builder.registerOpaque(decl);
        }
    }
};
// }}}

// second stage: do the heavy lifting to fill the tlb-database with all types
// defined in the main-header file
// class TlbImportCallback {{{1
class TlbImportCallback : public MatchFinder::MatchCallback {
    public:

    virtual void run(const MatchFinder::MatchResult &Result) {

        if (const TypeDecl *tDecl = Result.Nodes.getNodeAs<TypeDecl>("namedDecl")) {

            // don't add system-header-stuff like "struct __locale_struct" to
            // the database just because it's a declaration. if it is used
            // later _inside_ an other registered struct, we'll add it later to
            // the database nevertheless.
            if (!Result.SourceManager->isInMainFile(tDecl->getLocation())) {
                return;
            }

            builder.registerTypeDecl(tDecl);

        } else if(const TypedefType *tType = Result.Nodes.getNodeAs<TypedefType>("typedefType")) {

            if (!Result.SourceManager->isInMainFile(tType->getDecl()
                                                        ->getTypeSourceInfo()
                                                        ->getTypeLoc()
                                                        .getLocStart())) {
                return;
            }

            builder.registerTypedefNameDecl(tType->getDecl());
        }
    }

};
// }}}

} // end anonymous namespace

// commandline options {{{1
static llvm::cl::OptionCategory ToolCategory("typelib-clang-tlb-importer options");
static llvm::cl::opt<std::string> opaquePath(
        "opaquePath",
        llvm::cl::desc("registry of opaques, which have to be defined in the header files"),
        llvm::cl::Optional,
        llvm::cl::cat(ToolCategory));

static llvm::cl::opt<std::string> tlbSavePath(
        "tlbSavePath",
        llvm::cl::desc("where to save tlb-database"),
        llvm::cl::Required,
        llvm::cl::cat(ToolCategory));
// }}}

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

    //load opaque registry
    if(!opaquePath.empty())
    {
        std::cout << "Loading opaque tlb-registry from '" << opaquePath << "'" << std::endl;
        builder.loadRegistry(opaquePath);

        //resolve opaues to canonical names
        ast_matchers::MatchFinder OpaqueFinder;
        OpaqueCallback opaqueCallback;

        for (Typelib::Registry::Iterator it = builder.getRegistry().begin();
             it != builder.getRegistry().end(); it++) {
            if (it->getCategory() == Typelib::Type::Opaque) {
                OpaqueFinder.addMatcher(
                    namedDecl(hasName(typlibtoCxxName(it->getName())))
                        .bind("opaque"),
                    &opaqueCallback);
            }
        }

        if (int retval = Tool.run(newFrontendActionFactory(&OpaqueFinder))) {
            std::cout << "Parsing error in clang, cannot continue" << std::endl;
            exit(retval);
        }
        std::cout << "Finished resolving opaque-registry\n\n";
    }

    ast_matchers::MatchFinder Finder;

    TlbImportCallback tlbImportCallback;
    Finder.addMatcher(namedDecl().bind("namedDecl"), &tlbImportCallback);
    Finder.addMatcher(typedefType().bind("typedefType"), &tlbImportCallback);
    
    if (int retval = Tool.run(newFrontendActionFactory(&Finder))) {
        std::cout << "Parsing error in clang, cannot continue" << std::endl;
        exit(retval);
    }

    std::cout << "Saving tlb-registry into file '" << tlbSavePath << "'" << std::endl;
    builder.saveRegistry(tlbSavePath);
    
    exit(EXIT_SUCCESS);
}
