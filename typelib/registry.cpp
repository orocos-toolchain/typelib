#include "registry.h"

#include "typebuilder.h"
#include <memory>
#include <iostream>
#include <fstream>
using namespace std;

#include "parsing.h"
using namespace Parsing;

#include <libxml/xmlmemory.h>
#include <sys/types.h>
#include <dirent.h>

Registry::Registry()  { loadDefaultLibraries(); }
Registry::~Registry() { clear(); }

void Registry::loadDefaultLibraries()
{
    string home_dir = getenv("HOME") + string("/.genom/typelib/");
    loadLibraryDir(home_dir);
}

void Registry::loadLibraryDir(const std::string& path)
{
    DIR* dir = opendir(path.c_str());
    if (!dir)
    {
        if (errno != ENOENT)
            cerr << "Error opening " << path << ": " << strerror(errno) << endl;
        return;
    }

    clog << "Loading libraries in " << path << endl;
    dirent* dir_entry;
    while( (dir_entry = readdir(dir)) )
    {
        if (dir_entry -> d_type != DT_REG)
            continue;

        string file = path + dir_entry -> d_name;
        ifstream stream;
        stream.open(file.c_str()); 
        if (!stream) 
            continue;
        
        clog << "\t" << file << " ";

        int old_count = m_persistent.size();

        try 
        { 
            load(file); 
            int new_count = m_persistent.size();
            clog << "loaded, " << new_count - old_count << " types found" << endl;
        }
        catch(MalformedXML) { clog << "is not a valid XML file" << endl; }
        catch(BadRootElement) { clog << "is not a type library" << endl; }
        catch(ParsingError& error)
        { clog << "error, " << error.toString() << endl; }
    }
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

int Registry::getCount() const { return m_persistent.size(); }

Registry::StringList Registry::getTypeNames() const
{
    StringList ret;
    for (PersistentList::const_iterator it = m_persistent.begin(); it != m_persistent.end(); ++it)
        ret.push_back(it -> second -> getName());

    return ret;
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

void Registry::add(Type* new_type, const std::string& file)
{
    std::string name(new_type -> getName());
    const Type* old_type = get(name);
    if (old_type == new_type) return;

    if (old_type)
        throw AlreadyDefined(old_type, new_type);

    const Type::Category cat(new_type -> getCategory());
    if (cat != Type::Array && cat != Type::Pointer)
        m_persistent.push_back( make_pair(file, new_type) );

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
    
    template<typename Exception>
    void checkNodeName(xmlNodePtr node, const char* expected)
    {
        if (xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>(expected)))
            throw Exception("", reinterpret_cast<const char*>(node->name), expected);
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

        checkNodeName<UnexpectedElement>(field, "field");
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
    string full_path;
    if (path[0] != '/')
    {
        char cwd[PATH_MAX];
        getcwd(cwd, PATH_MAX);

        full_path = string(cwd) + "/" + path;
    } else full_path = path;

    xmlDocPtr doc = xmlParseFile(path.c_str());
    if (!doc) 
        throw MalformedXML(path);

    try
    {
        xmlNodePtr root_node = xmlDocGetRootElement(doc);
        if (!root_node) return;
        checkNodeName<BadRootElement>(root_node, "typelib");

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
                std::cerr << "\tThe definition found in " << path << " will be ignored" << endl;
            }
            else
                add(new_type.release(), full_path);
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

void Registry::dump(ostream& stream, int mode, const std::string& file) const
{
    stream << "Types in registry";
    if (file == "")
        stream << " not defined in any type library";
    else if (file != "*")
        stream << " defined in " << file;
    stream << endl;

    for (PersistentList::const_iterator it = m_persistent.begin(); it != m_persistent.end(); ++it)
    {
        if (file != "*" && file != it -> first)
            continue;

        if (mode & WithFile)
        {
            string it_file = it->first;
            if (it_file.empty()) stream << "\t\t";
            else stream << it -> first << "\t";
        }

        const Type* type = it -> second;
        if (mode & AllType)
            stream << type -> toString("\t", mode & Registry::RecursiveTypeDump) << endl;
        else
            stream << "\t" << type -> getName() << endl;
    }
}

