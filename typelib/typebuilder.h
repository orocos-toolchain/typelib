#ifndef __TYPEBUILDER_H__
#define __TYPEBUILDER_H__

#include <string>
#include <list>

#include "type.h"

class Registry;

class TypeBuilder
{
    std::string m_basename;
    const Type* m_type;
    
    struct Modifier
    {
        Type::Category category;
        int size; // Size of an array, deference count on multi-dim pointers
    };
    typedef std::list<Modifier> ModifierList;
    typedef std::pair<const Type*, ModifierList> TypeSpec;

    static TypeSpec parse(const Registry* registry, const std::string& full_name);
    static const Type* build(Registry* registry, const TypeSpec& spec);

    Registry* m_registry;

public:
    class NotFound
    {
        std::string m_name;
    public:
        NotFound(const std::string& name) : m_name(name) {}
        std::string toString() const { return "Type " + m_name + " not found"; }
    };

    TypeBuilder(Registry* registry, const std::list<std::string>& base);
    TypeBuilder(Registry* registry, const Type* base_type);

    void addPointer(int level);
    void addArray(int size);

    const Type* getType() const;

    static const Type* build(Registry* registry, const std::string& full_name);
    static const Type* getBaseType(const Registry* registry, const std::string& full_name);
};

#endif

