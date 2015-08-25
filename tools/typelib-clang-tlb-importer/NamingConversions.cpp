#include "NamingConversions.hpp"
#include "IgnoredOrRenamedTypes.hpp"

#include <iostream>

#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ASTContext.h>

// helperfunction: using the given string, looking up all instances of "from"
// and replacing them with "to" and returning the new string
std::string stringFromToReplace(const std::string &string,
                                const std::string &from,
                                const std::string &to) {

    std::string result(string);

    for (size_t start_pos = result.find(from); start_pos != std::string::npos;
         start_pos = result.find(from, start_pos + to.length())) {
        result.replace(start_pos, from.length(), to);
    }

    return result;
}

// the actual string-conversion function converting single cxx-names:
std::string cxxToTypelibName(const std::string &cxxName) {
    std::string typelibName(cxxName);

    if (typelibName.empty()) {
        std::cerr << "Error: cxxToTypelibName() called with empty string?\n";
        exit(EXIT_FAILURE);
    }

    typelibName = stringFromToReplace(typelibName, "::", "/");
    // hmprf: this "hack" is still needed:
    typelibName = stringFromToReplace(typelibName, " [", "[");
    typelibName = stringFromToReplace(typelibName, "> ", ">");
    typelibName = stringFromToReplace(typelibName, "< ", "<");

    // in case we are called with the QualifiedNames member-variables of a
    // template, the full template-name is in the string. so we have to handle
    // all the peculiarities to put everything explicitly in the global
    // namespace. also template-arguments.
    typelibName = stringFromToReplace(typelibName, ", ", ",/");

    // hardcoded shit... sorry...
    size_t positionOfAngleBracket = typelibName.find_first_of("<");
    while (positionOfAngleBracket != std::string::npos) {
        if (typelibName.at(positionOfAngleBracket+1) != '/') {
            typelibName.insert(positionOfAngleBracket+1, "/");
        }
        positionOfAngleBracket =
            typelibName.find_first_of("<", positionOfAngleBracket+1);
    }

    // now go through the string and remove slashes in front of numbers (digits
    // and minuses)
    size_t positionOfSlash = typelibName.find_first_of("/");
    while (positionOfSlash != std::string::npos) {
        // we wann actually access the character after the "/"
        size_t positionAfterSlash = positionOfSlash + 1;
        // guard against end-of-string
        if (positionAfterSlash >= typelibName.size()) {
            break;
        }
        // check if one of our conditions is met
        if (isdigit(typelibName.at(positionAfterSlash)) ||
            ('-' == typelibName.at(positionAfterSlash))) {
            typelibName.erase(positionOfSlash, 1);
        }
        // prepare for next round. the iterator "positionAfterSlash" is
        // actually not valid anymore, since we changed the string. but we know
        // we only removed one character, and "//" is not allowed. so.
        positionOfSlash = typelibName.find_first_of("/", positionAfterSlash);
    }

    // again: explicitly set the whole thing into the global namespace
    if (typelibName.at(0) != '/' && typelibName.length() > 1)
        typelibName.insert(0, 1, '/');

    // woa, thats it, nearly.

    std::pair<bool, std::string> retval = IgnoredOrRenamedType::isTypeRenamed(typelibName);
    if (retval.first) {
        return retval.second;
    }

    return typelibName;
}

std::string cxxToTypelibName(const clang::NamedDecl* decl)
{
    clang::LangOptions o;
    clang::PrintingPolicy p(o);
    p.SuppressTagKeyword = true;

    // just convert the name
    if (const clang::TypedefNameDecl *tDecl = llvm::dyn_cast<clang::TypedefNameDecl>(decl)) {
        return cxxToTypelibName(tDecl->getQualifiedNameAsString());
    }else if (const clang::TypeDecl *tDecl = llvm::dyn_cast<clang::TypeDecl>(decl)) {
        return cxxToTypelibName(decl->getASTContext()
                                    .getTypeDeclType(tDecl)
                                    .getCanonicalType()
                                    .getAsString(p));
    } else {
        std::cerr << "woooha\n";
        exit(-1);
    }
}

// underlying types like "int" or "float[4]"
std::string cxxToTypelibName(const clang::QualType &type) {

    // this will give special handling for "Records" (e.g. classes, structs,
    // templates and so on)
    if (const clang::CXXRecordDecl *rDecl =
            type.getTypePtr()->getAsCXXRecordDecl()) {

        return cxxToTypelibName(rDecl);
    }

    // everything else is simply handled as a "Type" whose complete name is
    // printed into a string converted as-is
    clang::LangOptions o;
    clang::PrintingPolicy suppressTagKeyword(o);
    suppressTagKeyword.SuppressTagKeyword = true;

    return cxxToTypelibName(type.getAsString(suppressTagKeyword));
}

std::string typelibToCxxName(const std::string& typelibName)
{
    std::string cxxName = stringFromToReplace(typelibName, "/", "::");

    // this was done by the corresponding gccxml-ruby function. this will hide
    // if someone deliberately gave us classes or objects from the global
    // namespace... well, corner-cases... go along, nothing to see
    cxxName = stringFromToReplace(cxxName, "<::", "<");
    cxxName = stringFromToReplace(cxxName, ",::", ",");
    // note that one behaviour of the ruby-side was not ported -- adding a
    // space after a comma, like ',([^\s])' -> ', \1'

    // again, same behaviour which the gccxml-ruby code had.
    if ((cxxName.size() > 2) && (cxxName.substr(0,2) == std::string("::")))
        return cxxName.substr(2, std::string::npos);
    else
        return cxxName;
}
