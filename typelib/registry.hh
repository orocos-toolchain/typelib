#ifndef TYPELIB_REGISTRY_H
#define TYPELIB_REGISTRY_H

#include "typemodel.hh"
#include <boost/shared_ptr.hpp>

namespace Typelib
{
    class RegistryIterator;
    
    /** Base for exceptions thrown by the registry */
    class RegistryException : public std::exception 
    {
    public:
        virtual std::string toString() const throw() 
        { return what(); }
    };

    /** A type has not been found while it was expected */
    class Undefined : public RegistryException
    {
        std::string m_name;

    public:
        std::string getName() const { return m_name; }
        Undefined(const std::string& name)
            : m_name(name) {}
        ~Undefined() throw() {}

        std::string toString() const throw() 
        { return "Undefined type " + m_name; }
    };

    /** A type is already defined */
    class AlreadyDefined : public RegistryException
    {
        std::string m_name;
    public:
        AlreadyDefined(std::string const& name)
            : m_name(name) {}
        ~AlreadyDefined() throw() { }

        std::string getName() const { return m_name; }
        std::string toString() const throw() 
        { return "Type " + m_name + " already defined in registry"; }
    };

    /** An attempt has been made of registering a type with an invalid name */
    class BadName : public RegistryException
    {
        const std::string m_name;

    public:
        BadName(const std::string& name)
            : m_name(name) {}
        ~BadName() throw() {}

        std::string toString() const throw()
        { return "Type name " + m_name + " is invalid"; }
    };


    /** Manipulation of a set of types
     * This class is a container for types. It helps manipulating a set of types. 
     * Any type in a Registry object can only reference types in the same
     * registry
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
        Registry();
        ~Registry();

        /** Import the \c name namespace into the current namespace */
        void importNamespace(const std::string& name);

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
         * If \c name is a modified version (pointer or array) of a known
         * type, then the according object can be created
         *
         * @arg name the type name
         * @return true if the Type exists or can be built, false otherwise
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


}; // Typelib namespace

#endif

