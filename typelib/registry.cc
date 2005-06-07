#include "registry.hh"

#include "typebuilder.hh"
#include "typedisplay.hh"
#include <memory>
#include <iostream>
#include <set>
using namespace std;

#include <ctype.h>

#include <boost/static_assert.hpp>

#include <iostream>
#include "registryiterator.hh"

using namespace Typelib;

namespace 
{   
    bool sort_names ( const std::string& name1, const std::string& name2 )
    {
        NameTokenizer tok1(name1);
        NameTokenizer tok2(name2);
        NameTokenizer::const_iterator it1 = tok1.begin();
        NameTokenizer::const_iterator it2 = tok2.begin();

        std::string ns1, ns2;
        // we sort /A/B/C/ before /A/B/C/class
        // and /A/B/C/class2 after /A/B/class1
        for (; it1 != tok1.end() && it2 != tok2.end(); ++it1, ++it2)
        {
            ns1 = *it1;
            ns2 = *it2;

            int value = ns1.compare(ns2);
            if (value) return value < 0;
            //cout << ns1 << " " << ns2 << endl;
        }

        if (it1 == tok1.end()) 
            return true;  // there is namespace names left in name1, and not in name2
        if (it2 == tok2.end()) 
            return false; // there is namespace names left in name2, and not in name1
        int value = ns1.compare(ns2);
        if (value) 
            return value < 0;

        return false;
    }
}

namespace Typelib
{
    char const* const Registry::s_stdsource = "__stdtypes__";
    
    Registry::Registry()
        : m_global(sort_names)
    { 
        addStandardTypes();
        setDefaultNamespace("/");
    }
    Registry::~Registry() { clear(); }

    std::string Registry::getDefaultNamespace() const { return m_namespace; }
    bool Registry::setDefaultNamespace(const string& space)
    {
        if (! isValidNamespace(space, true))
            return false;

        m_namespace = getNormalizedNamespace(space);
        //cout << "Setting default namespace to " << m_namespace << endl;

        for (TypeMap::iterator it = m_global.begin(); it != m_global.end(); ++it)
            m_current.insert( make_pair(it->first, it->second) );

        NameTokenizer tokens( m_namespace );
        NameTokenizer::const_iterator ns_it = tokens.begin();

        //cout << "Importing " << NamespaceMarkString << endl;
        importNamespace(NamespaceMarkString);

        std::string cur_space = NamespaceMarkString;
        for (; ns_it != tokens.end(); ++ns_it)
        {
            cur_space += *ns_it + NamespaceMarkString;

            //cout << "Importing " << cur_space << endl;
            importNamespace(cur_space);
        }

        //cout << "Types available: " << endl;
        //for (NameMap::const_iterator it = m_current.begin(); it != m_current.end(); ++it)
        //    std::cout << it->first << std::endl;
        
        return true;
    }

    void Registry::importNamespace(const std::string& name)
    {
        const std::string norm_name(getNormalizedNamespace(name));
        const int         norm_length(norm_name.length());
            
        TypeMap::const_iterator it = m_global.lower_bound(norm_name);

        while ( it != m_global.end() && isInNamespace(it->first, norm_name, false) )
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
        add(new Numeric("/char",            sizeof(char),           Numeric::SInt), s_stdsource);
        add(new Numeric("/signed char",     sizeof(char),           Numeric::SInt), s_stdsource);
        add(new Numeric("/unsigned char",   sizeof(unsigned char),  Numeric::UInt), s_stdsource);
        add(new Numeric("/short",           sizeof(short),          Numeric::SInt), s_stdsource);
        add(new Numeric("/signed short",    sizeof(short),          Numeric::SInt), s_stdsource);
        add(new Numeric("/unsigned short",  sizeof(unsigned short), Numeric::UInt), s_stdsource);
        add(new Numeric("/int",             sizeof(int),            Numeric::SInt), s_stdsource);
        add(new Numeric("/signed",          sizeof(signed),         Numeric::SInt), s_stdsource);
        add(new Numeric("/signed int",      sizeof(int),            Numeric::SInt), s_stdsource);
        add(new Numeric("/unsigned",        sizeof(unsigned),       Numeric::UInt), s_stdsource);
        add(new Numeric("/unsigned int",    sizeof(unsigned int),   Numeric::UInt), s_stdsource);
        add(new Numeric("/long",            sizeof(long),           Numeric::SInt), s_stdsource);
        add(new Numeric("/unsigned long",   sizeof(unsigned long),  Numeric::UInt), s_stdsource);

        add(new Numeric("/float",           sizeof(float),          Numeric::Float), s_stdsource);
        add(new Numeric("/double",          sizeof(double),         Numeric::Float), s_stdsource);
    }

    bool Registry::has(const std::string& name, bool build) const
    {
        if (m_current.find(name) != m_current.end())
            return true;

        if (! build)
            return false;

        const Type* base_type = TypeBuilder::getBaseType(*this, getFullName(name));
        return base_type;
    }

    const Type* Registry::build(const std::string& name)
    {
        const Type* type = get(name);
        if (type)
            return type;

        return TypeBuilder::build(*this, getFullName(name));
    }

    Type* Registry::get_(const std::string& name)
    {
        NameMap::const_iterator it = m_current.find(name);
        if (it != m_current.end()) 
            return it->second.type;
        return 0;
    }

    const Type* Registry::get(const std::string& name) const
    {
        NameMap::const_iterator it = m_current.find(name);
        if (it != m_current.end()) 
            return it->second.type;
        return 0;
    }

    bool Registry::isPersistent(std::string const& name, Type const& type, std::string const& source_id)
    {
        if (source_id == s_stdsource)
            return false;
        if (type.getName() != name)
            return true;

        switch(type.getCategory())
        {
            case Type::Pointer:
            case Type::Array:
                return false;
            default:
                return true;
        };
    }

    void Registry::add(std::string const& name, Type* new_type, std::string const& source_id)
    {
        const Type* old_type = get(name);
        if (old_type == new_type) 
            return; 
        else if (old_type)
            throw AlreadyDefined(name);

        RegistryType regtype = 
            { new_type
            , isPersistent(name, *new_type, source_id)
            , source_id };
            
        m_global.insert (make_pair(name, regtype));
        m_current.insert(make_pair(name, regtype));
        std::string current_name = getTypename(name);
        if (current_name != name)
            m_current.insert(make_pair(current_name, regtype));
    }

    void Registry::add(Type* new_type, const std::string& source_id)
    {
        std::string name(new_type -> getName());
        if (! isValidTypename(name, true))
            throw BadName(name);
            
        add(name, new_type, source_id);
    }

    void Registry::alias(std::string const& base, std::string const& alias, std::string const& source_id)
    {
        if (! isValidTypename(alias, true))
            throw BadName(alias);

        Type* base_type = get_(base);
        if (! base_type) 
            throw Undefined(base);

        add(alias, base_type, source_id);
    }

    void Registry::clear()
    {
        for (TypeMap::iterator it = m_global.begin(); it != m_global.end(); ++it)
        {
            Type* type = it->second.type;
            if (type->getName() != it->first)
            {
                // This is an alias
                continue;
            }
            delete type;
        }
            
        m_global.clear();
        m_current.clear();
    }

    namespace
    {
        bool filter_source(std::string const& filter, std::string const& source)
        {
            if (filter == "*")
                return true;
            return filter == source;
        }
    }

    void Registry::dump(ostream& stream, int mode, const std::string& source_filter) const
    {
        stream << "Types in registry";
        if (source_filter == "")
            stream << " not defined in any type library";
        else if (source_filter != "*")
            stream << " defined in " << source_filter;
        stream << endl;

        TypeDisplayVisitor display(cout, "");
        for (TypeMap::const_iterator it = m_global.begin(); it != m_global.end(); ++it)
        {
            RegistryType regtype(it->second);
            if (! regtype.persistent)
                continue;
            if (filter_source(source_filter, regtype.source_id))
                continue;

            if (mode & WithSourceId)
            {
                string it_source_id = regtype.source_id;
                if (it_source_id.empty()) stream << "\t\t";
                else stream << it_source_id << "\t";
            }

            Type const* type = regtype.type;
            if (mode & AllType)
            {
                if (type->getName() == it->first)
                {
                    display.apply(*type);
                    stream << "\n";
                }
                else
                    stream << it->first << " is an alias for " << type->getName() << "\n";
            }
            else
                stream << "\t" << type->getName() << endl;
        }
    }


    RegistryIterator Registry::begin() const { return RegistryIterator(m_global.begin()); }
    RegistryIterator Registry::end() const { return RegistryIterator(m_global.end()); }


#if 0
    /******************************************************************************************
     *  Serialization support
     */

    //void Registry::loadDefaultLibraries()
    //{
    //    string home_dir = getenv("HOME") + string("/.genom/typelib/");
    //    loadLibraryDir(home_dir);
    //}

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

    /*
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
            // int offset        = getIntAttribute   (field, "offset");
            // TODO: check alignment 

            // TODO allow recursive types    
            const Type* type_object = build(tname);
            if (! type_object)
                throw Undefined(tname);

            type -> addField(name, type_object);
        }
    }
    */

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
            if (! it->first.empty() && ! save_all)
                continue;

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
#endif

}; // namespace Typelib

