#include "registry.h"

#include "typebuilder.h"
#include <libxml/xmlmemory.h>
#include <memory>
#include <iostream>
#include <fstream>
using namespace std;

#include "parsing.h"
using namespace Parsing;

Registry::Registry() 
{
}

Registry::~Registry()
{
    clear();
}

void Registry::addStandardTypes()
{
    add(new Type("char", sizeof(char), Type::SInt));
    add(new Type("signed char", sizeof(char), Type::SInt));
    add(new Type("unsigned char", sizeof(unsigned char), Type::UInt));
    add(new Type("short", sizeof(short), Type::SInt));
    add(new Type("signed short", sizeof(short), Type::SInt));
    add(new Type("unsigned short", sizeof(unsigned short), Type::UInt));
    add(new Type("int", sizeof(int), Type::SInt));
    add(new Type("signed", sizeof(signed), Type::SInt));
    add(new Type("signed int", sizeof(int), Type::SInt));
    add(new Type("unsigned", sizeof(unsigned), Type::UInt));
    add(new Type("unsigned int", sizeof(unsigned int), Type::UInt));
    add(new Type("long", sizeof(long), Type::SInt));
    add(new Type("unsigned long", sizeof(unsigned long), Type::UInt));

    add(new Type("float", sizeof(float), Type::Float));
    add(new Type("double", sizeof(double), Type::Float));
}

bool Registry::has(const std::string& name, bool build) const
{
    TypeMap::const_iterator it = m_typemap.find(name);
    if (it != m_typemap.end()) return true;
    if (! build) return false;

    const Type* base_type = TypeBuilder::getBaseType(this, name);
    return base_type;
}

const Type* Registry::build(const std::string& name)
{
    const Type* type = get(name);
    if (type)
        return type;
    
    return TypeBuilder::build(this, name);
}

const Type* Registry::get(const std::string& name) const
{
    TypeMap::const_iterator it = m_typemap.find(name);
    if (it != m_typemap.end()) return it->second;
    return 0;
}

void Registry::add(Type* new_type)
{
    std::string name(new_type -> getName());
    const Type* old_type = get(name);
    if (old_type == new_type) return;

    if (old_type)
        throw AlreadyDefined(old_type, new_type);

    const Type::Category cat(new_type -> getCategory());
    if (cat != Type::Array && cat != Type::Pointer)
        m_persistent.push_back( make_pair("", new_type) );

    m_typemap.insert(std::make_pair(name, new_type));
}

void Registry::clear()
{
    for (TypeMap::iterator it = m_typemap.begin(); it != m_typemap.end(); ++it)
        delete it->second;
    m_typemap.clear();
    m_persistent.clear();
}




/******************************************************************************************
 *  Serialization support
 */

namespace 
{
    using std::string;

    struct XMLCategory
    {
        const xmlChar* node_name;
        Type::Category node_category;
    };
    XMLCategory categories[] = {
        { reinterpret_cast<const xmlChar*>("sint"), Type::SInt },
        { reinterpret_cast<const xmlChar*>("uint"), Type::UInt },
        { reinterpret_cast<const xmlChar*>("float"), Type::Float },
        { reinterpret_cast<const xmlChar*>("struct"), Type::Struct },
        { reinterpret_cast<const xmlChar*>("union"), Type::Union },
        { 0, Type::Array /* Don't mind */}
    };
    
    void checkNodeName(xmlNodePtr node, const char* expected)
    {
        if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>(expected)))
            throw UnexpectedElement("", reinterpret_cast<const char*>(node->name), expected);
    }

    std::string getStringFromCategory(Type::Category cat)
    {
        for(const XMLCategory* cur_cat = categories; cur_cat -> node_name; ++cur_cat)
        {
            if (cur_cat->node_category == cat)
                return std::string( reinterpret_cast<const char*>(cur_cat->node_name) );
        }
        // should not happen
        return "";
    }

    Type::Category getCategoryFromNode(xmlNodePtr node)
    {
        
        for(const XMLCategory* cur_cat = categories; cur_cat -> node_name; ++cur_cat)
        {
            if (!xmlStrcmp(node->name, cur_cat->node_name))
                return cur_cat -> node_category;
        }

        throw UnexpectedElement("", reinterpret_cast<const char*>(node->name), "");
    }

    std::string getStringAttribute(xmlNodePtr type, const char* att_name)
    {
        xmlChar* att = xmlGetProp(type, reinterpret_cast<const xmlChar*>(att_name) );
        if (! att)
            throw MissingAttribute("", att_name);
        std::string ret( reinterpret_cast<const char*>(att));
        xmlFree(att);
        return ret;
    }
    int getIntAttribute(xmlNodePtr type, const char* att_name)
    {
        std::string as_string = getStringAttribute(type, att_name);
        // TODO: format checking
        return atoi(as_string.c_str());
    }
}

void Registry::getFields(xmlNodePtr node, Type* type)
{
    // <field> elements are allowed since it is a complex type
    for(xmlNodePtr field = node->xmlChildrenNode; field; field=field->next)
    {
        if (!xmlStrcmp(field->name, reinterpret_cast<const xmlChar*>("text")))
            continue;

        checkNodeName(field, "field");
        std::string name = getStringAttribute(field, "name");
        std::string tname = getStringAttribute(field, "type");
        int offset = getIntAttribute(field, "offset");

        // TODO allow recursive types    
        const Type* type_object = build(tname);
        if (! type_object)
            throw Undefined(tname);

        type -> addField(name, type_object);
    }
}

std::string Registry::getDefinitionFile(const Type* type) const
{
    PersistentList::const_iterator entry;
    for (entry = m_persistent.begin(); entry != m_persistent.end(); ++entry)
    {
        if (entry -> second == type)
            return entry -> first;
    }

    return "";
}

void Registry::load(const std::string& path)
{
    xmlDocPtr doc = xmlParseFile(path.c_str());
    if (!doc) 
        throw MalformedXML(path);

    try
    {
        xmlNodePtr root_node = xmlDocGetRootElement(doc);
        if (!root_node) return;
        checkNodeName(root_node, "typelib");

        for(xmlNodePtr type = root_node -> xmlChildrenNode; type; type=type->next)
        {
            if (!xmlStrcmp(type->name, reinterpret_cast<const xmlChar*>("text")))
                continue;

            Type::Category cat = getCategoryFromNode(type);

            std::string name = getStringAttribute(type, "name");
            int         size = getIntAttribute(type, "size");

            auto_ptr<Type>new_type(new Type(name, size, cat));
            if (!new_type -> isSimple())
                getFields(type, new_type.get());

            const Type* old_type = get(name);
            if (old_type)
            {
                std::string old_file = getDefinitionFile(old_type);
                std::cerr << "Type " << name << " has already been defined in " << old_file << endl;
                std::cerr << "\tI won't overwrite with the definition found in " << path << endl;
            }
            else
                add(new_type.release());
        }
    }
    catch(ParsingError& error)
    {
        xmlFreeDoc(doc);
        error.setFile(path);
        throw;
    }

    xmlFreeDoc(doc);
}

static const xmlChar* header = (const xmlChar*)"<?xml version=\"1.0\"?>";
bool Registry::save(const std::string& path, bool save_all) const
{
    ofstream out;
    out.open(path.c_str());

    if (! out)
        return false;

    out << header << "\n"
        << "<typelib>\n";

    for (PersistentList::const_iterator it = m_persistent.begin(); it != m_persistent.end(); ++it)
    {
        const Type* type(it->second);

        std::string cat_string = getStringFromCategory(type->getCategory());
        out << "\t<" << cat_string << " " << "name=\"" << type->getName() << "\" " << "size=\"" << type->getSize() << "\"";

        const Type::FieldList& fields(type -> getFields());
        if (fields.empty())
            out << " />\n";
        else
        {
            out << ">\n";
            for (Type::FieldList::const_iterator field = fields.begin(); field != fields.end(); ++field)
            {
                const Type* field_type = field->getType();
                out << "\t\t<field name=\"" << field -> getName()
                    << "\" offset=\"" << field -> getOffset() 
                    << "\" type=\"" << field_type -> getName() << "\" />\n"; 
            }
            out << "\t</" << cat_string << ">\n";
        }
    }

    out << "</typelib>\n";
    
    return true;
}

void Registry::dump(bool verbose) const
{
    cerr << "Persistent types" << endl;
    if (verbose)
    {
        for (PersistentList::const_iterator it = m_persistent.begin(); it != m_persistent.end(); ++it)
            cerr << it->second -> toString("\t") << endl;
    }
    else
    {
        for (PersistentList::const_iterator it = m_persistent.begin(); it != m_persistent.end(); ++it)
            cerr << "\t" << it->second->getName() << endl;
    }
}
    
