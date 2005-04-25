#ifndef TYPELIB_REGISTRY_H
#define TYPELIB_REGISTRY_H

#include "type.hh"
#include <libxml/parser.h>

namespace Typelib
{
    class RegistryException : public std::exception 
    {
    public:
        virtual std::string toString() const throw() 
        { return what(); }
    };

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

    class AlreadyDefined : public RegistryException
    {
        const Type* m_old;
        Type*       m_new;

    public:
        AlreadyDefined(const Type* old_type, Type* new_type)
            : m_old(old_type), m_new(new_type) {}
        ~AlreadyDefined() throw() { }

        const Type* getOld() const { return m_old; }
        Type*       getNew() const { return m_new; }

        std::string toString() const throw() 
        { return "Type " + m_old->getName() + " already defined in registry"; }
    };

    class AlreadyDefinedName : public RegistryException
    {
        const std::string name;

    public:
        AlreadyDefinedName(const std::string& name)
            : name(name) {}
        ~AlreadyDefinedName() throw() {}

        const std::string getName() const { return name; }
        std::string toString() const throw()
        { return "Type " + name + " already defined in registry"; }
    };

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

    class Registry
    {
    private:
        typedef std::pair<std::string, const Type*>     PersistentType;
        typedef std::list<PersistentType>               PersistentList;
        PersistentList m_persistent;

        typedef std::map
            <const std::string
            , Type *
            , bool (*) (const std::string&, const std::string&)
            >     TypeMap;

        typedef std::map<const std::string, Type *>  NameMap;
    

        TypeMap                 m_global;
        NameMap                 m_current;
        std::string             m_namespace;
        std::list<std::string>  m_nspace_list;

        void addStandardTypes();
        //void loadDefaultLibraries();
        void loadLibraryDir(const std::string& path);

    public:
        Registry();
        ~Registry();

        void importNamespace(const std::string& name);

        bool setDefaultNamespace(const std::string& name);
        std::string getDefaultNamespace() const;

       
        /** Checks for the availability of a particular type
         *
         * If \c name is a modified version (pointer or array) of a known
         * type, then the according object is created
         *
         * @arg name the type name
         * @return true if the Type exists or can be built, false otherwise
         */
        bool        has(const std::string& name, bool build = true) const;


        const Type* build(const std::string& name);

        /** Gets a Type object
         * @arg name the type name
         * @return the type object if it exists, 0 if not
         */
        const Type* get(const std::string& name) const;  

        /** Adds a new type
         * @return true on success, false if the type was already
         * defined in the registry
         */
        void        add(Type* type, const std::string& file = "");


        void        clear();
        int         getCount() const;
        std::list<std::string>  getTypeNames() const;

        std::string getFullName (const std::string& name) const;

    private:
        void getFields(xmlNodePtr xml, Type* type);

    public:
        /** Loads the type from the tlb file in \c path
         * Note that any type not defined in this registry shall already been defined
         * in the registry. In particular, there is no support for forward declarations
         */
        void load(const std::string& path);

        /** Saves the registry in a XML file in \c path
         * @path the path of the destination file
         * @save_all if true, all the registry will be saved. Otherwise, we save only types
         * not defined in other registries (as seen by load)
         */
        bool save(const std::string& path, bool save_all = false) const;

        std::string getDefinitionFile(const Type* type) const;


        enum DumpMode
        {
            NameOnly = 0, 
            AllType  = 1, 
            WithSourceId = 2,
            RecursiveTypeDump = 4
        };
        void dump(std::ostream& stream, int dumpmode = AllType, const std::string& file = "*") const;
    };


}; // Typelib namespace

#endif

