#ifndef REGISTRY_H
#define REGISTRY_H

#include "type.h"
#include <libxml/parser.h>

class Registry
{
public:
    typedef std::list<std::string>                  StringList;

private:
    typedef std::pair<std::string, const Type*>     PersistentType;
    typedef std::list<PersistentType>               PersistentList;
    typedef std::map<const std::string, Type *>     TypeMap;

    PersistentList m_persistent;
    TypeMap  m_typemap;
    TypeMap::const_iterator findEnd() const;
    TypeMap::const_iterator find(const std::string& name) const;
    
    void addStandardTypes();

public:
    Registry();
    ~Registry();
    
    struct Undefined
    {
        std::string m_typename;
    public:
        std::string getTypename() const
        { return m_typename; }
        Undefined(const std::string& name)
            : m_typename(name) {}
    };
    struct AlreadyDefined
    {
        const Type* m_old;
        Type*       m_new;
    public:
        AlreadyDefined(const Type* old_type, Type* new_type)
            : m_old(old_type), m_new(new_type) {}
        ~AlreadyDefined() { delete m_new; }

        const Type* getOld() const { return m_old; }
        Type*       getNew() const { return m_new; }

        std::string toString() const { return "Type " + m_old->getName() + " was already defined"; }
    };

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
    void        add(Type* type);


    void        clear();

private:
    void getFields(xmlNodePtr xml, Type* type);
    
public:
    /** Loads the type from the tlb file in \c path
     * Note that any type not defined in this registry shall already been defined
     * in the registry. In particular, there is no support for forward declarations
     */
    void load(const std::string& path);

    /** Saves the registry in as XML in \c path
     * @path the path of the destination file
     * @save_all if true, all the registry will be saved. Otherwise, we save only types
     * not defined in other registries (as seen by load)
     */
    bool save(const std::string& path, bool save_all = false) const;

    std::string getDefinitionFile(const Type* type) const;

    void dump(bool verbose = false) const;
};

#endif

