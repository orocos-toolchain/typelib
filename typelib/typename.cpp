#include "typename.h"
#include <boost/static_assert.hpp>

using namespace std;
using namespace boost;
using namespace Typelib;

namespace
{
    typedef std::string::value_type   NamespaceMarkType;
    static const NamespaceMarkType    NamespaceMark = '/';
    static const char*                NamespaceMarkString = "/";

    struct NameSeparator : public char_separator<NamespaceMarkType>
    {
        NameSeparator()
            : char_separator<NamespaceMarkType>(NamespaceMarkString, "", boost::keep_empty_tokens) {}
    };
    typedef boost::tokenizer<NameSeparator> NameTokenizer;
    
    // helper function which checks that \c identifier does contain
    // only valid characters. Note that identifier is supposed
    // to be a normalized name, so that it cannot be empty
    bool isValidIdentifier(const string& identifier)
    {
        if (identifier.empty())   return false;

        const int length(identifier.length());
        for (int i = 0; i < length; ++i)
        {
            string::value_type c(identifier[i]);
            if (c != '_' && c != ' ' && ! isalnum(identifier[i]))
                return false;
        }

        return true;
    }
}

namespace Typelib
{
    bool isAbsoluteName(const string& identifier)
    {
        if (identifier.empty()) return false;
        return identifier[0] == NamespaceMark;
    }

    bool isValidTypename(const std::string& name, bool absolute)
    { 
        if (!isValidNamespace(name, absolute))
            return false;
        return (*name.rend()) != NamespaceMark;
    }

    bool isValidNamespace(const string& name, bool absolute)
    {
        if (name.empty()) return true;
        if (absolute && name[0] != NamespaceMark) return false;

        NameTokenizer tokenizer(name);

        NameTokenizer::const_iterator it = tokenizer.begin();
        if (it -> empty()) // remove the empty token we get because
            ++it;          // of a leading '/'

        for(; it != tokenizer.end(); ++it)
        {
            if (!isValidIdentifier(*it))
                return false;
        }

        return true;
    }

    bool isInNamespace(const string& type, const std::string& nspace, bool recursive)
    {
        std::string normalized_nspace( getNormalizedNamespace(nspace) );
        const int   length (normalized_nspace.length());

        bool begins_with = string(type, 0, length) == normalized_nspace;
        if (!begins_with) return false;
        if (recursive)    return true; 
        return type.find(NamespaceMark, length) == string::npos;
    }

    std::string getNormalizedNamespace(const std::string& name)
    {
        if (name.empty()) return NamespaceMarkString;
        if (*name.rend() != NamespaceMark) return name + NamespaceMark;
        return name;
    }

    std::string getTypename(const std::string& name)
    {
        int position = name.rfind(NamespaceMark);
        if (position == string::npos)
            return name;

        return string(name, position + 1, string::npos);
    }

    std::string getNamespace(const std::string& name)
    {
        int position = name.rfind(NamespaceMark);
        if (position == string::npos)
            return name;

        return string(name, 0, position + 1);
    }
};

