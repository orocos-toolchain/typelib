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

#include <ctype.h>

#include <boost/static_assert.hpp>



#include <iostream>


using namespace Typelib;

namespace 
{   
    bool sort_names ( const std::string& name1, const std::string& name2 )
    {
        NameTokenizer tok1(name1);
        NameTokenizer tok2(name2);
        NameTokenizer::const_iterator it1 = tok1.begin();
        NameTokenizer::const_iterator it2 = tok2.begin();

        std::string 
            ns1(*it1), 
            ns2(*it2);
        // sort /A/B/C/class2 after /A/B/class1
        for (++it1, ++it2; it1 != tok1.end() && it2 != tok2.end(); ++it1, ++it2)
        {
            int value = ns1.compare(ns2);
            if (value) return value < 0;

            ns1 = *it1;
            ns2 = *it2;
        }

        if (it1 != tok1.end()) return true;  // there is namespace names left in name1, and not in name1
        if (it2 != tok2.end()) return false; // there is namespace names left in name2, and not in name2

        int value = ns1.compare(ns2);
        if (value) return value < 0;

        return false;
    }
}

namespace Typelib
{
    Registry::Registry()
        : m_global(sort_names), m_current(sort_names),
        m_namespace("/")
    { 
        addStandardTypes();
        loadDefaultLibraries(); 
    }
    Registry::~Registry() { clear(); }

    std::string Registry::getDefaultNamespace() const { return m_namespace; }
    bool Registry::setDefaultNamespace(const string& space)
    {
        if (isValidNamespace(space, true))
            return false;

        m_namespace = getNormalizedNamespace(space);

        m_current = m_global;
        NameTokenizer tokens(space);
        NameTokenizer::const_iterator ns_it = tokens.begin();

        std::string cur_space;

        for (++ns_it; ns_it != tokens.end(); ++ns_it)
        {
            cur_space += *ns_it + '/';
            importNamespace(cur_space + '/');
        }

        for (TypeMap::const_iterator it = m_current.begin(); it != m_current.end(); ++it)
            std::cout << it->first << std::endl;
        
        return true;
    }

    void Registry::importNamespace(const std::string& name)
    {
        const std::string norm_name(getNormalizedNamespace(name));
        const int         norm_length(norm_name.length());
            
        TypeMap::const_iterator it = m_global.find(norm_name);
        if (it == m_global.end())
            return;

        while ( isInNamespace(it->first, norm_name, false) )
        {
            const std::string rel_name(it->first, norm_length, string::npos);
            m_current.insert( make_pair(rel_name, it->second) );
            ++it;
        }
    }

    std::string Registry::getFullName(const std::string& name) const
    {
        if (isAbsoluteName(name))
            return name;
        return m_namespace + name;
    }

    void Registry::addStandardTypes()
    {
        BOOST_STATIC_ASSERT((NamespaceMark == '/'));
        add(new Type("/char", sizeof(char), Type::SInt));
        add(new Type("/signed char", sizeof(char), Type::SInt));
        add(new Type("/unsigned char", sizeof(unsigned char), Type::UInt));
        add(new Type("/short", sizeof(short), Type::SInt));
        add(new Type("/signed short", sizeof(short), Type::SInt));
        add(new Type("/unsigned short", sizeof(unsigned short), Type::UInt));
        add(new Type("/int", sizeof(int), Type::SInt));
        add(new Type("/signed", sizeof(signed), Type::SInt));
        add(new Type("/signed int", sizeof(int), Type::SInt));
        add(new Type("/unsigned", sizeof(unsigned), Type::UInt));
        add(new Type("/unsigned int", sizeof(unsigned int), Type::UInt));
        add(new Type("/long", sizeof(long), Type::SInt));
        add(new Type("/unsigned long", sizeof(unsigned long), Type::UInt));

        add(new Type("/float", sizeof(float), Type::Float));
        add(new Type("/double", sizeof(double), Type::Float));
    }

    int Registry::getCount() const { return m_persistent.size(); }

    bool Registry::has(const std::string& name, bool build) const
    {
        if (m_current.find(name) != m_current.end())
            return true;

        if (! build) return false;

        const Type* base_type = TypeBuilder::getBaseType(this, getFullName(name));
        return base_type;
    }

    const Type* Registry::build(const std::string& name)
    {
        const Type* type = get(name);
        if (type)
            return type;

        return TypeBuilder::build(this, getFullName(name));
    }

    const Type* Registry::get(const std::string& name) const
    {
        TypeMap::const_iterator it = m_current.find(name);
        if (it != m_current.end()) 
            return it->second;
        return 0;
    }

    list<string> Registry::getTypeNames() const
    {
        list<string> ret;
        for (PersistentList::const_iterator it = m_persistent.begin(); it != m_persistent.end(); ++it)
            ret.push_back(it -> second -> getName());

        return ret;
    }

    void Registry::add(Type* new_type, const std::string& source_id)
    {
        std::string name(new_type -> getName());
        if (! isValidTypename(name, true))
            throw BadName(name);
            
        const Type* old_type = get(name);
        if (old_type == new_type) 
            return;

        if (old_type)
            throw AlreadyDefined(old_type, new_type);

        const Type::Category cat(new_type -> getCategory());
        if (cat != Type::Array && cat != Type::Pointer)
            m_persistent.push_back( make_pair(source_id, new_type) );

        m_global.insert(make_pair(name, new_type));
        m_current.insert(make_pair(name, new_type));
        m_current.insert(make_pair(getTypename(name), new_type));
        cout << "New type: " << name << " " << getTypename(name) << endl;
    }

    void Registry::clear()
    {
        for (TypeMap::iterator it = m_global.begin(); it != m_global.end(); ++it)
            delete it->second;
        m_current.clear();
        m_persistent.clear();
    }









    /******************************************************************************************
     *  Serialization support
     */

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

            checkNodeName<UnexpectedElement>      (field, "field");
            std::string name  = getStringAttribute(field, "name");
            std::string tname = getStringAttribute(field, "type");
            int offset        = getIntAttribute   (field, "offset");

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

    void Registry::dump(ostream& stream, int mode, const std::string& source_id) const
    {
        stream << "Types in registry";
        if (source_id == "")
            stream << " not defined in any type library";
        else if (source_id != "*")
            stream << " defined in " << source_id;
        stream << endl;

        for (PersistentList::const_iterator it = m_persistent.begin(); it != m_persistent.end(); ++it)
        {
            if (source_id != "*" && source_id != it -> first)
                continue;

            if (mode & WithSourceId)
            {
                string it_source_id = it->first;
                if (it_source_id.empty()) stream << "\t\t";
                else stream << it -> first << "\t";
            }

            const Type* type = it -> second;
            if (mode & AllType)
                stream << type -> toString(
                        (mode&WithSourceId ? "\t\t" : "\t"), mode & Registry::RecursiveTypeDump) 
                    << endl;
            else
                stream << "\t" << type -> getName() << endl;
        }
    }

}; // namespace Typelib

