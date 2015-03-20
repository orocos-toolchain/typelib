#include "IgnoredOrRenamedTypes.hpp"

#include <iostream>
#include <algorithm>

IgnoredOrRenamedType* IgnoredOrRenamedType::spInstance = NULL;

IgnoredOrRenamedType *IgnoredOrRenamedType::getInstance() {
    if (spInstance == NULL) {
        spInstance = new IgnoredOrRenamedType();
    }
    return spInstance;
}

void IgnoredOrRenamedType::printTemplateArgsToBeIgnored() {
    getInstance();

    std::cout << "templateArgsToBeIgnored:\n";
    for (std::vector<std::string>::const_iterator it =
             getInstance()->vTemplateArgsToBeIgnored.begin();
         it != getInstance()->vTemplateArgsToBeIgnored.end(); ++it) {
        std::cout << " - '" << *it << "'\n";
    }
}

void IgnoredOrRenamedType::printTypeRenames() {
    std::cout << "typeRenames:\n";
    for (std::vector<std::pair<std::string,std::string> >::const_iterator it =
             getInstance()->vTypeRenames.begin();
         it != getInstance()->vTypeRenames.end(); ++it) {
        std::cout << " - '" << (*it).first << "' -> '" << (*it).second << "'\n";
    }
}

bool IgnoredOrRenamedType::isTemplateArgumentIgnored(const std::string& typelibName) {
    for (std::vector<std::string>::const_iterator it =
             getInstance()->vTemplateArgsToBeIgnored.begin();
         it != getInstance()->vTemplateArgsToBeIgnored.end(); ++it) {
        if (typelibName.find(*it) == 0) {
            std::cout << "ignore '" << typelibName << "' because of '" << *it << "'\n";
            return true;
        }
    }
    return false;
}

std::pair<bool, std::string>
IgnoredOrRenamedType::isTypeRenamed(const std::string &typelibName) {
    std::pair<bool, std::string> retval(false, "");
    for (std::vector<std::pair<std::string, std::string> >::const_iterator it =
             getInstance()->vTypeRenames.begin();
         it != getInstance()->vTypeRenames.end(); ++it) {
        if ((*it).first == typelibName) {
            retval.first = true;
            retval.second = (*it).second;
            return retval;
        }
    }
    return retval;
}

IgnoredOrRenamedType::IgnoredOrRenamedType() {

    if (spInstance != NULL) {
        std::cerr << "woaaar\n";
    };
    spInstance = this;

    vTemplateArgsToBeIgnored.push_back("/std/allocator");
    vTemplateArgsToBeIgnored.push_back("/std/char_traits");

    vTypeRenames.push_back(std::pair<std::string, std::string>(
        "/std/basic_string", "/std/string"));
}
