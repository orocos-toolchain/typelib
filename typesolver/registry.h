#ifndef REGISTRY_H
#define REGISTRY_H

#include "type.h"

class Registry
{
    typedef std::list<std::string>  StringList;

    typedef std::map<const std::string, Type *>      TypeMap;

    TypeMap m_persistent, m_temporary;
    TypeMap::const_iterator findEnd() const;
    TypeMap::const_iterator find(const std::string& name) const;
    
    static Registry* s_self;
    Registry(const StringList& registry_path);
    
public:
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
    
    static Registry* self();

    /** Checks for the availability of a particular type
     *
     * If \c name is a modified version (pointer or array) of a known
     * type, then the according object is created
     *
     * @arg name the type name
     * @return true if the Type exists or can be built, false otherwise
     */
    bool        has(const std::string& name, bool build = true) const;

    /** Gets a Type object
     * @arg name the type name
     * @arg if we shall build a new type if \c name is derived from 
     * a known type (pointer or array)
     * @return the type object if it exists or can be built, 0 if not
     */
    const Type* get(const std::string& name, bool build = true);  

    /** Adds a new type
     * @return true on success, false if the type was already
     * defined in the registry
     */
    void        add(Type* type);
};

#endif

