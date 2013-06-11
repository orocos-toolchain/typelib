#ifndef TYPELIB_LANG_TLB_XMLTOOLS_HH
#define TYPELIB_LANG_TLB_XMLTOOLS_HH

#include <libxml/xmlmemory.h>
#include "parsing.hh"
#include <boost/lexical_cast.hpp>

namespace 
{
    using std::string;

    template<typename Exception>
    void checkNodeName(xmlNodePtr node, const char* expected)
    {
        if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>(expected)))
            throw Exception(reinterpret_cast<const char*>(node->name), expected, "");
    }

    std::string getStringAttribute(xmlNodePtr type, const char* att_name)
    {
        xmlChar* att = xmlGetProp(type, reinterpret_cast<const xmlChar*>(att_name) );
        if (! att)
            throw Parsing::MissingAttribute(att_name, "");
        std::string ret( reinterpret_cast<const char*>(att));
        xmlFree(att);
        return ret;
    }

    template<typename T>
    T getAttribute(xmlNodePtr type, char const* att_name)
    { return boost::lexical_cast<T>(getStringAttribute(type, att_name)); }
    template<>
    std::string getAttribute<std::string>(xmlNodePtr type, const char* att_name)
    { return getStringAttribute(type, att_name); }
}

#endif

