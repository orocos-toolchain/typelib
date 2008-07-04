#include "registry.hh"
#include "registryiterator.hh"
#include "typebuilder.hh"
#include "typedisplay.hh"

#include <memory>
#include <iostream>
#include <set>
using namespace std;

#include <ctype.h>

#include <boost/static_assert.hpp>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

using namespace Typelib;

namespace 
{   
    /* Returns true if \c name1 is either in a more in-depth namespace than
     * name2 (i.e. name2 == /A/B/class and name1 == /A/B/C/class2 or if 
     * name2 < name1 (lexicographic sort). Otherwise, returns false
     */
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
    Type const& Registry::null() { 
        static NullType const null_type("/nil");
        return null_type; 
    }
    
    Registry::Registry()
        : m_global(sort_names)
    { 
        addStandardTypes();
        setDefaultNamespace("/");
    }
    Registry::~Registry() { clear(); }

    std::string Registry::getDefaultNamespace() const { return m_namespace; }
    bool Registry::setDefaultNamespace(const std::string& space)
    {
        if (! isValidNamespace(space, true))
            return false;

	m_current.clear();
        m_namespace = getNormalizedNamespace(space);

        for (TypeMap::iterator it = m_global.begin(); it != m_global.end(); ++it)
            m_current.insert( make_pair(it->first, it->second) );

        importNamespace(NamespaceMarkString);
        std::string cur_space = NamespaceMarkString;
        NameTokenizer tokens( m_namespace );
        NameTokenizer::const_iterator ns_it = tokens.begin();
        for (; ns_it != tokens.end(); ++ns_it)
        {
            cur_space += *ns_it + NamespaceMarkString;
            importNamespace(cur_space, true);
        }
        return true;
    }

    void Registry::importNamespace(const std::string& name, bool erase_existing)
    {
        const std::string norm_name(getNormalizedNamespace(name));
        const int         norm_length(norm_name.length());
            
        TypeMap::const_iterator it = m_global.lower_bound(norm_name);

        while ( it != m_global.end() && isInNamespace(it->first, norm_name, true) )
        {
            const std::string rel_name(it->first, norm_length, string::npos);
	    if (erase_existing)
		m_current.erase(rel_name);
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

    namespace {
        template<typename T>
        Numeric* make_std_numeric(char const* name)
        {
            typedef numeric_limits<T>   limits;
            Numeric::NumericCategory category = limits::is_signed ? Numeric::SInt : Numeric::UInt;
            unsigned int size = (limits::digits + 1) / 8;
            return new Numeric(name, size, category);
        }
    }

#   define DECLARE_STD_TYPE(name) add(make_std_numeric<name>("/" #name), s_stdsource)
    void Registry::addStandardTypes()
    {
        BOOST_STATIC_ASSERT((NamespaceMark == '/'));

        add(new NullType("/nil"), s_stdsource);
        alias("/nil", "/void", s_stdsource);
        
        DECLARE_STD_TYPE(char);
        DECLARE_STD_TYPE(signed char);
        DECLARE_STD_TYPE(unsigned char);

        DECLARE_STD_TYPE(short);
        DECLARE_STD_TYPE(short int);
        DECLARE_STD_TYPE(signed short);
        DECLARE_STD_TYPE(signed short int);
        DECLARE_STD_TYPE(unsigned short);
        DECLARE_STD_TYPE(unsigned short int);

        DECLARE_STD_TYPE(long);
        DECLARE_STD_TYPE(long int);
        DECLARE_STD_TYPE(signed long);
        DECLARE_STD_TYPE(signed long int);
        DECLARE_STD_TYPE(unsigned long);
        DECLARE_STD_TYPE(unsigned long int);

        DECLARE_STD_TYPE(long long);
        DECLARE_STD_TYPE(long long int);
        DECLARE_STD_TYPE(signed long long);
        DECLARE_STD_TYPE(signed long long int);
        DECLARE_STD_TYPE(unsigned long long);
        DECLARE_STD_TYPE(unsigned long long int);

        DECLARE_STD_TYPE(int);
        DECLARE_STD_TYPE(signed int);
        DECLARE_STD_TYPE(unsigned int);

        BOOST_STATIC_ASSERT(( sizeof(float) == sizeof(int32_t) ));
        BOOST_STATIC_ASSERT(( sizeof(double) == sizeof(int64_t) ));
        add(new Numeric("/float",  4, Numeric::Float), s_stdsource);
        add(new Numeric("/double", 8, Numeric::Float), s_stdsource);

        // Add standard sized integers
        static const int sizes[] = { 1, 2, 4, 8 };
        for (int i = 0; i < 4; ++i)
        {
            string suffix = "int" + lexical_cast<string>(sizes[i] * 8) + "_t";
            add(new Numeric("/"  + suffix, sizes[i], Numeric::SInt));
            add(new Numeric("/u" + suffix, sizes[i], Numeric::UInt));
        }
    }

    void Registry::merge(Registry const& registry)
    {
	// Merge non aliased types. Aliases are never used by type model classes
	// so we can safely call merge()
	for(Iterator it = registry.begin(); it != registry.end(); ++it)
	{
	    if (it.isAlias()) continue;
	    it->merge(*this);
	}

	for (Iterator it = registry.begin(); it != registry.end(); ++it)
	{
	    // Either the alias already points to a concrete type, and
	    // we must check that it is the same concrete type, or
	    // we have to add the alias
	    if (!it.isAlias()) continue;

	    Type const* old_type = get(it.getName());
	    if (old_type)
	    {
		if (!old_type->isSame(*it))
		    throw DefinitionMismatch(it.getName());
	    }
	    else
	    {
		// we are sure the concrete type we are pointing to is 
		// already in the target registry
		alias(it->getName(), it.getName(), "");
	    }
	}
    }


    Registry* Registry::minimal(Registry const& auto_types) const
    {
        auto_ptr<Registry> result(new Registry);
 
	// Merge non aliased types. Aliases are never used by type model classes
	// so we can safely call merge()
	for(Iterator it = begin(); it != end(); ++it)
	{
	    if (it.isAlias()) continue;
            if (!auto_types.has(it->getName()))
                it->merge(*result);
	}

	for (Iterator it = begin(); it != end(); ++it)
	{
	    // Either the alias already points to a concrete type, and
	    // we must check that it is the same concrete type, or
	    // we have to add the alias
	    if (!it.isAlias()) continue;
            if (auto_types.has(it.getName())) continue;

	    Type const* old_type = get(it.getName());
	    if (old_type)
	    {
		if (!old_type->isSame(*it))
		    throw DefinitionMismatch(it.getName());
	    }
	    else
	    {
		// we are sure the concrete type we are pointing to is 
		// already in the target registry
		result->alias(it->getName(), it.getName(), "");
	    }
	}

        return result.release();
    }

    bool Registry::has(const std::string& name, bool build_if_missing) const
    {
        if (m_current.find(name) != m_current.end())
            return true;

        if (! build_if_missing)
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
        if (! isValidTypename(name, true))
            throw BadName(name);

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

        NameTokenizer tokens( m_namespace );
        NameTokenizer::const_iterator ns_it = tokens.begin();
        std::string cur_space = NamespaceMarkString;
	while(true)
        {
	    string cur_name  = getRelativeName(name, cur_space);

	    // Check if there is already a type with the same relative name
	    TypeMap::iterator it = m_current.find(cur_name);
	    if (it == m_current.end() || !sort_names(it->second.type->getName(), name))
	    {
		m_current.erase(cur_name);
		m_current.insert(make_pair(cur_name, regtype));
	    }

	    if (ns_it == tokens.end())
		break;
            cur_space += *ns_it + NamespaceMarkString;
	    ++ns_it;
        }
    }

    void Registry::add(Type* new_type, const std::string& source_id)
    { add(new_type->getName(), new_type, source_id); }

    void Registry::alias(std::string const& base, std::string const& newname, std::string const& source_id)
    {
        if (! isValidTypename(newname, true))
            throw BadName(newname);

        Type* base_type = get_(base);
        if (! base_type) 
            throw Undefined(base);

        add(newname, base_type, source_id);
    }

    size_t Registry::size() const { return m_global.size(); }
    void Registry::clear()
    {
	// Set the type pointer to 0 on aliases
	for (TypeMap::iterator it = m_global.begin(); it != m_global.end(); ++it)
	{
	    if (it->first != it->second.type->getName())
		it->second.type = 0;
	}
	    
	// ... then delete the objects
        for (TypeMap::iterator it = m_global.begin(); it != m_global.end(); ++it)
            delete it->second.type;
            
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

    void Registry::dump(std::ostream& stream, int mode, const std::string& source_filter) const
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

    RegistryIterator Registry::begin() const { return RegistryIterator(*this, m_global.begin()); }
    RegistryIterator Registry::end() const { return RegistryIterator(*this, m_global.end()); }

    bool Registry::isSame(Registry const& other) const
    {
        if (m_global.size() != other.m_global.size())
            return false;

        TypeMap::const_iterator
            left_it   = m_global.begin(),
            left_end  = m_global.end(),
            right_it  = other.m_global.begin();

        while (left_it != left_end)
        {
            if (!left_it->second.type->isSame(*right_it->second.type))
            {
                throw;
                return false;
            }
            left_it++; left_end++;
        }
        return true;
    }

}; // namespace Typelib

