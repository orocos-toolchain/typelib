#ifndef TYPELIB_REGISTRY_H
#define TYPELIB_REGISTRY_H

#include "typemodel.hh"
#include <boost/shared_ptr.hpp>
#include <stdexcept>

namespace Typelib
{
    class RegistryIterator;
    
    /** Base for exceptions thrown by the registry */
    class RegistryException : public std::runtime_error 
    { 
    public:
        RegistryException(std::string const& what) : std::runtime_error(what) {}
    };

    /** A type has not been found while it was expected */
    class Undefined : public RegistryException
    {
        std::string m_name;

    public:
        Undefined(const std::string& name)
            : RegistryException("undefined type '" + name + "'")
            , m_name(name) {}
        ~Undefined() throw() {}

        std::string getName() const { return m_name; }
    };

    /** A type is already defined (duplicates definitions are
     * not allowed in type registries) */
    class AlreadyDefined : public RegistryException
    {
        std::string m_name;

    public:
        AlreadyDefined(std::string const& name)
            : RegistryException("type " + name + " already defined in registry")
            , m_name(name) {}
        ~AlreadyDefined() throw() {}

        std::string getName() const { return m_name; }
    };

    /** An attempt has been made of registering a type with an invalid name */
    class BadName : public RegistryException
    {
        const std::string m_name;

    public:
        BadName(const std::string& name)
            : RegistryException(name + " is not a valid type name")
            , m_name(name) {}
        ~BadName() throw() {}
    };

    /** We are trying to merge two registries where two types have the same
     * name but different definitions
     */
    class DefinitionMismatch : public RegistryException
    {
        const std::string m_name;

    public:
        DefinitionMismatch(const std::string& name)
            : RegistryException(name + " already defines a type in the registry, but with a different definition") {}
        ~DefinitionMismatch() throw() {}
    };


    /** Manipulation of a set of types
     * Any complex type in a Registry object can only reference types in the same
     * registry. No duplicates are allowed.
     *
     * Registry objects provide the following services:
     * <ul>
     *	<li> type aliases (Registry::alias)
     *	<li> namespace management: default namespace (Registry::setDefaultNamespace, Registry::getDefaultNamespace),
     *  namespace import (like <tt>using namespace</tt> in C++)
     *  <li> derived types (array, pointers) automatic building (Registry::build)
     * </ul>
     */
    class Registry
    {
        friend class RegistryIterator;
        
    private:
        struct RegistryType
        {
            Type*       type;
            bool        persistent;
            std::string source_id;
        };
        typedef std::map
            < const std::string
            , RegistryType
            , bool (*) (const std::string&, const std::string&)
            >     TypeMap;

        typedef std::map<const std::string, RegistryType>  NameMap;

        TypeMap                 m_global;
        NameMap                 m_current;
        std::string             m_namespace;

        void addStandardTypes();

        static char const* const s_stdsource;
        static bool isPersistent(std::string const& name, Type const& type, std::string const& source_id);
        void add(std::string const& name, Type* new_type, std::string const& source_id);

        /* Internal version to get a non-const Type object */
        Type* get_(const std::string& name);

    public:
	typedef RegistryIterator Iterator;

        Registry();
        ~Registry();

        /** Import the \c name namespace into the current namespace.
	 * @arg erase_existing controls the behaviour in case of name collision. If true,
	 * the type from the namespace being imported will have priority. If false, 
	 * an already imported type will have priority.
	 */
        void importNamespace(const std::string& name, bool erase_existing = false);

        /** Sets the default namespace. This removes all namespace import and
         * imports \c name in the current namespace
         */
        bool setDefaultNamespace(const std::string& name);
        /** Get the default namespace (if any). Returns '/' (i.e. the root)
         * if no default namespace have been set
         */
        std::string getDefaultNamespace() const;
       
        /** Checks for the availability of a particular type
         *
         * @arg name the type name
         * @arg build if true, the method returns true if \c name is 
	 * a derived version (pointer or array) of a known type
         *
         * @return true if the Type exists, or if it is a derived type and build is true, false otherwise
         */
        bool        has(const std::string& name, bool build = true) const;

        /** Get the source ID of \c type. Throws Undefined if \c type is not
         * a registered type
         */
        std::string source(Type const* type) const;

        /** Build a derived type from its canonical name */
        Type const* build(const std::string& name);

        /** Gets a Type object
         * @arg name the type name
         * @return the type object if it exists, 0 otherwise
         */
        Type const* get(const std::string& name) const;  

        /** Adds a new type
         * @return true on success, false if the type was already
         * defined in the registry
         */
        void        add(Type* type, std::string const& source_id = "");

        /** Creates an alias
         * While base can be a derived type, all aliases are considered persistent since
         * they can be referenced by their name in other types
         *
         * @arg base       the base type
         * @arg alias      the alias name
         * @arg source_id  a source ID for the newly created type
         */
        void        alias(std::string const& base, std::string const& alias, std::string const& source_id = "");

        /** Returns the count of types defined in this registry, including
         * non-persistent and aliases */
        size_t      size() const;
        /** Remove all types */
        void        clear();

        /** Get the absolute type name of \c name, 
         * taking the default namespace into account 
         * @see setDefaultNamespace getDefaultNamespace */
        std::string getFullName (const std::string& name) const;

	/** Iterator pointing on the first type of the registry */
        RegistryIterator begin() const;
	/** Past-the-end iterator on the registry */
        RegistryIterator end() const;
        
        /** Returns a null type */
        static Type const& null();

	/** Merges the content of \c registry into this object */
	void merge(Registry const& registry);

        /** Returns a registry containing the minimal set of types needed to
         * define the types that are in \c this and not in \c auto_types */
        Registry* minimal(Registry const& auto_types) const;

        /** Compares the two registries. Returns true if they contain the same
         * set of types, referenced under the same name
         *
         * Note that for now, if this contains a type 'A' aliased to 'B' and \c
         * other contains the \b same type but named 'B' and aliased to 'A',
         * the method will return false.
         */
        bool isSame(Registry const& other) const;

        /** Returns true if +type+ is a type included in this registry */
        bool isIncluded(Type const& type) const;
        
    public:
        enum DumpMode
        {
            NameOnly = 0, 
            AllType  = 1, 
            WithSourceId = 2,
            RecursiveTypeDump = 4
        };
        void dump(std::ostream& stream, int dumpmode = AllType, const std::string& source_filter = "*") const;
    };
}

#endif

