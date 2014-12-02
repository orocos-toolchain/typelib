#ifndef IGNORED_OR_RENAMED_TYPE
#define IGNORED_OR_RENAMED_TYPE

#include <string>
#include <vector>

class IgnoredOrRenamedType {
  public:
    static bool isTemplateArgumentIgnored(const std::string &typelibName);

    static std::pair<bool, std::string>
    isTypeRenamed(const std::string &typelibName);

    static void printTemplateArgsToBeIgnored();
    static void printTypeRenames();

  private:
    IgnoredOrRenamedType();
    static IgnoredOrRenamedType *getInstance();

    std::vector<std::string> vTemplateArgsToBeIgnored;
    std::vector<std::pair<std::string, std::string> > vTypeRenames;

    static IgnoredOrRenamedType *spInstance;
};

#endif//IGNORED_OR_RENAMED_TYPE
