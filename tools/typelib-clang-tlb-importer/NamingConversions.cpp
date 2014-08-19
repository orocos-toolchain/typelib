#include "NamingConversions.hpp"

#include <vector>

bool isConstant(const std::string& name, size_t pos)
{
    if(name.size() > pos)
    {
        char c = name.at(pos);
        if(std::isalpha(c) || c == '_')
            return false;

        return true;
    }
    return false;
}

// removing leading or trailing whitespaces from a given string
// see http://stackoverflow.com/a/1798170/3520187
std::string trim(const std::string &str,
                 const std::string &whitespace = " \t") {
    const size_t strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const size_t strEnd = str.find_last_not_of(whitespace);
    const size_t strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

// private helper function for splitting a string like
//    'std::vector<std::pair<double, double> >'
// into a vector of strings like
//    'std::vector'
//    '<'
//    'std::pair'
//    '<'
//    'double'
//    ','
//    'double'
//    '>'
//    '>'
std::vector<std::string> template_tokenizer(std::string name)
{
    std::vector<std::string> result;
    const char* chars = "<,>";

    while(!name.empty()) {
        size_t idx = name.find_first_of(chars);
        std::string match = trim(name.substr(0, idx));

        if (!match.empty())
            result.push_back(match);

        if (idx == std::string::npos)
            break;

        char _char = name.at(idx);
        name = name.substr(idx+1, std::string::npos);

        result.push_back(std::string(&_char));
    }

    return result;
}

// using the given string, looking up all instances of "from" and replacing
// them with "to" and returning the new string
std::string stringFromToReplace(const std::string& string, const std::string& from,
        const std::string& to) {

    std::string result(string);

    for (size_t start_pos = result.find(from); start_pos != std::string::npos;
         start_pos = result.find(from, start_pos + to.length())) {
        result.replace(start_pos, from.length(), to);
    }

    return result;
}

std::string cxxToTyplibName(const std::string& name)
{
    std::string ret(name);

    if(!ret.empty() && ret.at(0) != '/')
    {
        ret.insert(0, "/");
    }

    std::string from("::");
    std::string to("/");

    for (size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos))
    {
        ret.replace(start_pos, from.length(), to);
    }

    from = std::string("<");
    to = std::string("</");

    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos))
    {
        if(!isConstant(ret, start_pos + 1))
        {
            ret.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        else
        {
            start_pos += from.length();
        }
    }

    from = std::string(", ");
    to = std::string(",/");
    std::string toConst(",");

    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos))
    {
        if(!isConstant(ret, start_pos + 2))
        {
            ret.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        else
        {
            ret.replace(start_pos, from.length(), toConst);
            start_pos += toConst.length();
        }
    }

    from = std::string(" >");
    to = std::string(">");

    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos + to.length()))
    {
        ret.replace(start_pos, from.length(), to);
    }

    from = std::string(" &");
    to = std::string("&");

    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos + to.length()))
    {
        ret.replace(start_pos, from.length(), to);
    }

    from = std::string(" [");
    to = std::string("[");

    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos + to.length()))
    {
        ret.replace(start_pos, from.length(), to);
    }

#warning HACK, renaming '/std/basic_string</char>' to '/std/string'
    if(ret == "/std/basic_string")
        return "/std/string";

    if(ret == "/std/basic_string</char>")
        return "/std/string";

    return ret;
}

std::string typlibtoCxxName(const std::string& name)
{
    std::string ret(name);

    std::string from("/");
    std::string to("::");

    for (size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos + to.length()))
    {
        ret.replace(start_pos, from.length(), to);
    }


    return ret;
}
