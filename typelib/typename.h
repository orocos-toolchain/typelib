#ifndef __TYPELIB_TYPENAME_H__
#define __TYPELIB_TYPENAME_H__

#include <string>
#include <boost/tokenizer.hpp>

namespace Typelib
{
    typedef std::string::value_type   NamespaceMarkType;
    static const NamespaceMarkType    NamespaceMark = '/';
    static const char*                NamespaceMarkString = "/";

    struct NameSeparator : public boost::char_separator<NamespaceMarkType>
    {
        NameSeparator()
            : boost::char_separator<NamespaceMarkType>(NamespaceMarkString, "", boost::keep_empty_tokens) {}
    };
    typedef boost::tokenizer<NameSeparator> NameTokenizer;

    
    bool isAbsoluteName(const std::string& name);
    bool isValidNamespace(const std::string& name, bool absolute);
    bool isValidTypename(const std::string& name, bool absolute);
    bool isInNamespace(const std::string& type, const std::string& nspace, bool recursive = false);
    
    std::string getNormalizedNamespace(const std::string& name);
    std::string getTypename (const std::string& full_name);
    std::string getNamespace(const std::string& full_name);
};

#endif


