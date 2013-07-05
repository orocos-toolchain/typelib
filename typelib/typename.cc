#include "typename.hh"
#include <boost/static_assert.hpp>
#include <utilmm/stringtools.hh>
#include <iostream>

using namespace std;
using namespace boost;
using namespace Typelib;
using namespace utilmm;

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
            if (c != '_' && c != ' ' && !isalnum(c) && c != '<' && c != '>')
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

    static bool isValidTypeBasename(std::string s, bool absolute, bool accept_integers)
    {
        if (accept_integers && s.find_first_not_of("0123456789") == string::npos)
            return true;

        int pos = 0;
        if (absolute && s[0] != '/')
            return false;

        if (s[0] == '/')
            ++pos;

        size_t modifiers = s.find_first_of("*[");
        if (modifiers != string::npos)
            return isValidIdentifier(string(s, pos, modifiers - pos));
        else
            return isValidIdentifier(string(s, pos));

    }

    std::list<std::string> splitTypename(std::string const& name)
    {
        unsigned int start_pos = 0;
        if (name[0] == '/')
            start_pos++;

        int template_level = 0;
        std::list<std::string> result;
        for (unsigned int i = start_pos; i < name.length(); ++i)
        {
            if (name[i] == '/')
            {
                if (template_level == 0)
                {
                    result.push_back(string(name, start_pos, i - start_pos));
                    start_pos = i + 1;
                }
            }
            else if (name[i] == '<')
                template_level++;
            else if (name[i] == '>')
                template_level--;
        }
        if (start_pos < name.length())
            result.push_back(string(name, start_pos, name.length() - start_pos));

        return result;
    }

    static pair<bool, int> isValidTypename(std::string const& s, int pos, bool absolute, bool accept_integers)
    {
        unsigned int start_pos = pos;
        if (s[pos] == '/')
            pos++;

        for (unsigned int i = pos; i < s.length(); ++i)
        {
            if (s[i] == '/')
            {
                if (!isValidTypeBasename(string(s, start_pos, i - start_pos), absolute, accept_integers))
                    return make_pair(false, i);
                start_pos = i;
            }
            else if (s[i] == '<')
            {
                if (!isValidTypeBasename(string(s, start_pos, i - start_pos), absolute, accept_integers))
                    return make_pair(false, i);

                while (s[i] != '>')
                {
                    pair<bool, int> valid_arg = isValidTypename(s, i + 1, true, true);
                    i = valid_arg.second;

                    if (!valid_arg.first)
                        return valid_arg;
                    else if (i == s.length() || (s[i] != ',' && s[i] != '>'))
                        return make_pair(false, i);
                }

                start_pos = i + 1;
                if (i + 1 < s.length())
                {
                    if (s[i + 1] == '/')
                        i++;
                    else if (s[i + 1] != '>' && s[i + 1] != '[' && s[i + 1] != ',')
                        return make_pair(false, i + 1);
                }
            }
            else if (s[i] == '[')
            {
                // start_pos == i if we are parsing an array of templates
                if (start_pos != i && !isValidTypeBasename(string(s, start_pos, i - start_pos), absolute, accept_integers))
                    return make_pair(false, i);

                if (i + 1 == s.length())
                    return make_pair(false, i);

                start_pos = i + 1;
            }
            else if (s[i] == ']')
            {
                if (start_pos == i)
                    return make_pair(false, start_pos);
                if (string(s, start_pos, i - start_pos).find_first_not_of("0123456789") != string::npos)
                    return make_pair(false, start_pos);

                start_pos = i + 1;
                if (i + 1 < s.length())
                {
                    if (s[i + 1] == '/')
                        i++;
                    else if (s[i + 1] != '>' && s[i + 1] != '[')
                        return make_pair(false, i + 1);
                }
            }
            else if (s[i] == '>' || s[i] == ',')
            {
                if (!isValidTypeBasename(string(s, start_pos, i - start_pos), true, true))
                    return make_pair(false, i);
                return make_pair(true, i);
            }
        }

        if (start_pos != s.length())
            return make_pair(isValidTypeBasename(string(s, start_pos, s.length() - start_pos), absolute, accept_integers), s.length());
        else return make_pair(true, s.length());
    }

    bool isValidTypename(const std::string& name, bool absolute)
    {
        if (name.empty())
            return false;
        return isValidTypename(name, 0, absolute, false).first;
    }

    bool isValidNamespace(const string& name, bool absolute)
    {
        if (name.empty()) return false;
        if (absolute && name[0] != NamespaceMark) return false;

        NameTokenizer tokenizer(name);

        NameTokenizer::const_iterator it = tokenizer.begin();
        for (; it != tokenizer.end(); ++it)
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
        std::string remainder(type, length, string::npos);
        return splitTypename(remainder).size() == 1;
    }

    std::string getNormalizedNamespace(const std::string& name)
    {
        if (name.empty()) return NamespaceMarkString;
        if (name[name.size() - 1] != NamespaceMark) return name + NamespaceMark;
        return name;
    }

    std::string getTypename(const std::string& name)
    {
        list<string> split = splitTypename(name);
        if (split.empty())
            return std::string();
        return split.back();
    }

    std::string getRelativeName(std::string const& name, std::string const& ns)
    {
	size_t size = ns.length();
	if (*ns.rbegin() != '/')
	    size += 1;
	return std::string(name, size, string::npos);
    }

    std::string getMinimalPathTo(std::string const& full_name, std::string const& ns)
    {
        string type_ns = getNamespace(full_name);
        if (isInNamespace(full_name, ns, true))
            return getRelativeName(getNamespace(full_name), ns);
        else if (ns.find(type_ns) != string::npos || ns.find(full_name) != string::npos) // need an absolute path
            return type_ns;

        list<string> tok1(splitTypename(type_ns));
        list<string> tok2(splitTypename(ns));
        list<string>::const_iterator it1 = tok1.begin();
        list<string>::const_iterator it2 = tok2.begin();
        std::vector<std::string> common_tokens;

        // Filter out the common NS parts
        std::string ns1, ns2;
        for (; it1 != tok1.end() && it2 != tok2.end(); ++it1, ++it2)
        {
            ns1 = *it1;
            ns2 = *it2;
            int value = ns1.compare(ns2);
            if (value) break;
            common_tokens.push_back(ns1);
        }

        if (common_tokens.empty())
            return type_ns;

        // Build the remainder of both namespaces, and verify that the remainder
        // of the type is unambiguous. If it is, go back in it1 until it is not.
        // We already checked the case where the full path is ambiguous
        std::string result = *it1;
        list<string>::const_iterator remainder_it = it1;
        for (++remainder_it; remainder_it != tok1.end(); ++remainder_it)
            result += Typelib::NamespaceMarkString + *remainder_it;

        while (!common_tokens.empty() && ns.find(result) != string::npos)
        {
            result = common_tokens.back() + Typelib::NamespaceMarkString + result;
            common_tokens.pop_back();
        }
        if (common_tokens.empty())
            return type_ns;
        else if (result.empty())
            return result;
        else
            return result + Typelib::NamespaceMarkString;
    }

    std::string getNamespace(const std::string& name)
    {
        list<string> split = splitTypename(name);
        if (split.empty())
            return "/";
        split.pop_back();

        std::string result;
        for (list<string>::const_iterator it = split.begin(); it != split.end(); ++it)
            result += "/" + *it;

        result += "/";
        return result;
    }
};

