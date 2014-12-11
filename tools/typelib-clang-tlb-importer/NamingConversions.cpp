#include "NamingConversions.hpp"
#include "IgnoredOrRenamedTypes.hpp"

#include <iostream>

#include <clang/AST/DeclTemplate.h>
#include <clang/AST/PrettyPrinter.h>

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

std::pair<std::string, std::vector<std::string> >
typelibNameSplitTemplate(const std::string &typelibName) {

    // reject templates-of-templates, where more than one "opening angle
    // bracket" can be found in the name-string
    if (std::count(typelibName.begin(), typelibName.end(), '<') > 1) {
        std::cerr
            << "Error: typelibNameSplitTemplate() Templates-of-templates are "
               "not supported, but support can 'easily' be added.\n";
        exit(EXIT_FAILURE);
    }

    // we'll return this later, need this now
    std::pair<std::string,std::vector<std::string> > retval;

    size_t firstOpeningAngleBracket = typelibName.find_first_of("<");
    // this is the actual template-type
    retval.first = typelibName.substr(0, firstOpeningAngleBracket);
    // we might get a non-templated name. if so, return now. we are done
    if (firstOpeningAngleBracket == std::string::npos) {
        return retval;
    }

    // we have actually a template... look up some marks
    size_t lastClosingAngleBracket =
        typelibName.find_last_of(">", firstOpeningAngleBracket);
    size_t firstCommaAfterFirstOpeningAngleBracket =
        typelibName.find_first_of(",", firstOpeningAngleBracket);

    // now we need top copy chunks of the given string into the vector of the
    // return-value

    // we need two marks for iterating the string: where we start copying
    // characters and where we stop.
    size_t startDelim = firstOpeningAngleBracket;
    size_t stopDelim = firstCommaAfterFirstOpeningAngleBracket;
    // if we got a template with only one argument there is no comma. we need
    // to copy up to the closing ">".  if we got a comma, we need to stop
    // there, and proceed afterwards. see "stopDelim" inside the do-while loop
    //
    // this is copying the single chunks into our return value
    do {
        // if we did not find a "," in the following part of the string this
        // will be our last chunk to copy. we need to fall-back to using the
        // "lastClosingAngleBracket" as stopDelim.
        if (stopDelim == std::string::npos)
            stopDelim = lastClosingAngleBracket;
        // copy the chunk
        retval.second.push_back(
            typelibName.substr(startDelim + 1, stopDelim - startDelim - 1));
        // set the new start-mark
        startDelim = stopDelim;
        // and look how far we have to copy -- looking for a ","
        stopDelim = typelibName.find_first_of(",", startDelim + 1);
        // only start a new round of chunk-copying if we have still room left
    } while (startDelim < lastClosingAngleBracket);
    // tadahh
    return retval;
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

    // woa, thats it.
    return typelibName;
}

// helperfunction: convert a decl of a "template specialization" into a string
// in typelib-lingo
std::string
templateToTypelibName(const clang::ClassTemplateSpecializationDecl *tDecl) {

    if (!tDecl->getTemplateArgs().size())
        return "";

    std::string retval("<");

    const clang::TemplateArgumentList &tmpArgs(tDecl->getTemplateArgs());
    for (size_t idx = 0; idx < tmpArgs.size(); idx++) {

        std::string toBeAppended;
        switch (tmpArgs.get(idx).getKind()) {
        case clang::TemplateArgument::Declaration:
            /* std::cerr */
            /*     << " -- template arg declaration: " */
            /*     << tmpArgs.get(idx).getAsDecl()->getQualifiedNameAsString() << "\n"; */
            toBeAppended = cxxToTypelibName(
                tmpArgs.get(idx).getAsDecl()->getQualifiedNameAsString());
            break;
        case clang::TemplateArgument::Type: {
            std::string typelibNameOfTemplateArg = cxxToTypelibName(tmpArgs.get(idx).getAsType());
            /* std::cerr << " -- template arg type: " << typelibNameOfTemplateArg << "\n"; */
            toBeAppended = typelibNameOfTemplateArg;
        } break;
        case clang::TemplateArgument::Integral:
            /* std::cerr << " -- template arg integral: " */
            /*           << tmpArgs.get(idx).getAsIntegral().toString(10) << "\n"; */
            toBeAppended = tmpArgs.get(idx).getAsIntegral().toString(10);
            break;
        case clang::TemplateArgument::Template:
            std::cerr << " -- template arg template expansion... recursion? "
                         "not implemented yet... but should be easy\n";
            // toBeAppended = templateToTypelibName(tmpArgs.get(idx).getAsDecl());
            exit(EXIT_FAILURE);
            break;
        default:
            std::cerr << " -- template arg unhandled case "
                      << tmpArgs.get(idx).getKind() << "\n";
            exit(EXIT_FAILURE);
        }

        // having a "static vector<string>" somewhere containing
        // "to-be-skipped" template-arguments.
        if (IgnoredOrRenamedType::isTemplateArgumentIgnored(toBeAppended))
            continue;

        // we need commas as separator at certain places
        if (idx != 0)
            retval += "," + toBeAppended;
        else
            retval += toBeAppended;
    }

    // and finally close everything
    retval += ">";

    return retval;
}

std::string cxxToTypelibName(const clang::NamedDecl* decl)
{
    // just convert the name
    std::string typelibName(cxxToTypelibName(decl->getQualifiedNameAsString()));

    // note: template declarations as such and partial specializations are ignored here!
    if (const clang::ClassTemplateSpecializationDecl *tDecl =
            llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl)) {

        typelibName += templateToTypelibName(tDecl);
    }

    return cxxToTypelibName(typelibName);
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
