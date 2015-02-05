#include "HandleMetaData.hpp"
#include "NamingConversions.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Comment.h>

#include <sstream>
#include <iostream>

#include "typelib/typemodel.hh"

#include <unistd.h>

// see http://stackoverflow.com/a/5525712/3520187
std::string do_readlink(std::string const &path) {
    char buff[PATH_MAX];
    ssize_t len = ::readlink(path.c_str(), buff, sizeof(buff) - 1);
    if (len != -1) {
        buff[len] = '\0';
        return std::string(buff);
    } else {
        return path;
    }
}

void setMetaDataSourceFileLine(const clang::Decl *decl, Typelib::Type *type) {
    const clang::SourceManager &sm = decl->getASTContext().getSourceManager();
    const clang::SourceLocation &loc = sm.getExpansionLoc(decl->getLocStart());

    // uses "readlink(3)" to normalize symlinks from system-header path. for
    // example
    //     "/usr/lib/gcc/x86_64-linux-gnu/4.9/../../../../include/c++/4.9/bits/$file"
    // would be turned into
    //     "/usr/include/c++/4.9/bits/$file"
    // this was done by the gccxml-importer before. this is needed because the
    // "preprocess" stage of clang++, run later to retrieve the "orogen_include",
    // can output smth like
    //     "/usr/bin/../lib/gcc/x86_64-linux-gnu/4.9/../../../../include/c++/4.9/bits/$file"
    // which cannot be string-compared... so there, is would have to be flattend as well
       clang::FileManager &fm = sm.getFileManager();
    const clang::FileEntry *fileEntry = sm.getFileEntryForID(sm.getFileID(loc));
    std::string dirName = fm.getCanonicalName(fileEntry->getDir()).str();
    std::string fullFileName(fileEntry->getName());
    std::string fileName = fullFileName.substr(fullFileName.find_last_of("/") + 1);
    if (fileName.empty()) {
        std::cerr << "ERROR: invalid filename '" << fileEntry->getDir()
                  << "', no forward slash?\n";
        exit(-1);
    }
    // some headers can be reached via two ways: actual filepath and symlink
    // inside the ".orogen" folder -- a hack of really bad taste... anyhow,
    // guard against this...
    std::string filePath  = do_readlink(dirName+"/"+fileName);

    // typelib needs the '/path/to/file:column' information
    std::ostringstream stream;
    stream << filePath << ":"
           << sm.getExpansionLineNumber(loc);
    type->setPathToDefiningHeader(stream.str());

    type->getMetaData().add("source_file_line",
                            type->getPathToDefiningHeader());
}

void setMetaDataOrogenInclude(const clang::Decl *decl, Typelib::Type *type) {
    const clang::SourceManager &sm = decl->getASTContext().getSourceManager();
    const clang::SourceLocation &loc = sm.getSpellingLoc(decl->getLocStart());
    // add a header from the include-chain as "orogen_include" entry to the
    // metadata. this leads to possibly adding the main-header itself if the
    // type is defined there, which would be mostly the same as
    // "source_file_line". or it is the first non-main-header if the type is
    // defined somewhere else.
    /* std::cout << "-- the file '" << sm.getFilename(loc).str() << "' is " */
    /*           << (sm.isInSystemHeader(loc) ? "a" : "no") << " system
     * header\n"; */
    clang::SourceLocation incLoc = loc;
    clang::SourceLocation lastValidLoc = loc;
    while (incLoc.isValid() && sm.isInSystemHeader(incLoc)) {
        lastValidLoc = incLoc;
        incLoc = sm.getIncludeLoc(sm.getFileID(incLoc));
        /* std::cout << "-- the file '" << sm.getFilename(loc).str() */
        /*           << "' includes '" << sm.getFilename(incLoc).str() */
        /*           << "' which is " << (sm.isInSystemHeader(incLoc) ? "a" :
         * "no") */
        /*           << " system header\n"; */
    }

    // uses the FileManager together with the FileEntry to normalize symlinks
    // from system-header path. for example
    //     "/usr/lib/gcc/x86_64-linux-gnu/4.9/../../../../include/c++/4.9/bits/stl_vector.h"
    // into
    //     "/usr/include/c++/4.9/bits"
    clang::FileManager &fm = sm.getFileManager();
    const clang::FileEntry *fileEntry = sm.getFileEntryForID(sm.getFileID(lastValidLoc));
    std::string dirName = fm.getCanonicalName(fileEntry->getDir()).str();
    std::string fileName(fileEntry->getName());
    fileName = fileName.substr(fileName.find_last_of("/") + 1);
    if (fileName.empty()) {
        std::cerr << "ERROR: invalid filename '" << fileEntry->getDir()
                  << "', no forward slash?\n";
        exit(-1);
    }

    if (lastValidLoc.isValid()) {
        type->getMetaData().add(
            "orogen_include_x",
            do_readlink(sm.getFilename(lastValidLoc).str()));
    } else {
        std::cout << "setMetaDataOrogenInclude() Warning: could not find "
                     "suitable include-file for '" << type->getName() << "'\n";
    }
}

void setMetaDataBaseClasses(const clang::Decl *decl, Typelib::Type *type) {
    // we are also required to note all base-classes of the decl in the
    // metadata
    if (const clang::CXXRecordDecl *cxxRecord =
            llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {

        // TODO hm, there is something in the clang-api broken. need this
        // additional test because some things don't have a definition, but
        // still think they are able to access internal undefined pointers via
        // "bases_begin()" for example -- which would segfault.
        //
        // maybe templates-declaration which have no definition...?
        //
        // only fails for "/base/geometry/Spline<1>" of base/orogen/types
        // because there is no ostream-operator defined in
        // "base/geometry/Spline.hpp" -- but one for Spline<3>
        if (!cxxRecord->hasDefinition()) {
            return;
        }
        clang::CXXRecordDecl::base_class_const_iterator base;
        for (base = cxxRecord->bases_begin(); base != cxxRecord->bases_end();
             base++) {
            const clang::QualType &qualType = base->getType();

            type->getMetaData().add("base_classes", cxxToTypelibName(qualType));
        }
    }
}

void setMetaDataDoc(const clang::Decl *decl, Typelib::Type *type) {

    clang::comments::FullComment *comment =
        decl->getASTContext().getCommentForDecl(decl, NULL);

    // if there is no comment the resulting task is simple...
    if (!comment)
        return;

    std::ostringstream stream;

    clang::ArrayRef<clang::comments::BlockContentComment *>::const_iterator i;
    for (i = comment->getBlocks().begin(); i != comment->getBlocks().end();
         i++) {

        switch ((*i)->getCommentKind()) {
        case clang::comments::Comment::ParagraphCommentKind: {
            const clang::comments::ParagraphComment *p =
                llvm::dyn_cast<clang::comments::ParagraphComment>((*i));

            // save us very empty comment blocks
            if (p->isWhitespace())
                break;

            clang::comments::ParagraphComment::child_iterator c =
                p->child_begin();
            ;
            while (c != p->child_end()) {
                const clang::comments::TextComment *TC =
                    llvm::dyn_cast<clang::comments::TextComment>(*c);
                if (TC) {

                    stream << TC->getText().str();
                }
                c++;
                // only do a new line if it is not the last TextComment in the
                // block
                if (c != p->child_end() && TC) {
                    stream << "\n";
                }
            }
            break;
        }
        case clang::comments::Comment::VerbatimLineCommentKind: {
            stream << llvm::dyn_cast<clang::comments::VerbatimLineComment>((*i))
                          ->getText()
                          .str();
            break;
        }
        case clang::comments::Comment::BlockCommandCommentKind: {
            const clang::comments::BlockCommandComment *bp =
                llvm::dyn_cast<clang::comments::BlockCommandComment>((*i));

            const clang::comments::ParagraphComment *p = bp->getParagraph();

            clang::comments::ParagraphComment::child_iterator c =
                p->child_begin();
            ;
            while (c != p->child_end()) {
                const clang::comments::TextComment *TC =
                    llvm::dyn_cast<clang::comments::TextComment>(*c);
                if (TC) {
                    stream << TC->getText().str();
                }
                c++;
                // only do a new line if it is not the last TextComment in the
                // block
                if (c != p->child_end() && TC) {
                    stream << "\n";
                }
            }

            break;
        }
        default: {
            std::cout << "Non-handled comment type '"
                      << (*i)->getCommentKindName() << "' for Type '"
                      << type->getName() << "' located at '"
                      << (*i)->getLocation().printToString(
                             decl->getASTContext().getSourceManager()) << "'\n";
            return;
        }
        }
    }

    type->getMetaData().add("doc", stream.str());
}

void setMetaDataCxxname(const clang::NamedDecl *decl, Typelib::Type *type) {
    clang::LangOptions o;
    clang::PrintingPolicy p(o);

    if (const clang::TypedefNameDecl *tDecl =
            llvm::dyn_cast<clang::TypedefNameDecl>(decl)) {
        // don't wanna have the tag-keyword here
        p.SuppressTagKeyword = true;
        // what the template-type itself is called
        std::string typdefname = decl->getASTContext()
                                     .getTypedefType(tDecl->getCanonicalDecl())
                                     .getAsString(p);
        // and to what type the typedef is pointing
        std::string underlyingtypename = decl->getASTContext()
                                             .getTypedefType(tDecl)
                                             .getCanonicalType()
                                             .getAsString(p);
        // these are typedefs... not sure how to handle them here, depends on
        // what we actually wanna do with the "cxxname" metadata... print for
        // debugging I suppose... so we create a pseudo-cxx-statement showing
        // the adept cxx-programmer what lead to this type... kinda...
        type->getMetaData().add("cxxname", "typedef " + typdefname + " " +
                                               underlyingtypename);
    } else if (const clang::TypeDecl *tDecl =
                   llvm::dyn_cast<clang::TypeDecl>(decl)) {
        // adds the tage, like "class" or "union", to the printed type
        p.SuppressTagKeyword = false;
        type->getMetaData().add(
            "cxxname",
            decl->getASTContext().getTypeDeclType(tDecl).getAsString(p));
    }
}
