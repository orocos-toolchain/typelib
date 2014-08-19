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
