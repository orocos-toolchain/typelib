#include "registry.h"

#include "typebuilder.h"

Registry::Registry() 
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

//void Registry::load(const StringList& registry_path) {}

Registry::TypeMap::const_iterator Registry::findEnd() const { return m_temporary.end(); }
Registry::TypeMap::const_iterator Registry::find(const std::string& name) const
{
    TypeMap::const_iterator main = m_persistent.find(name);
    if (main != m_persistent.end()) return main;

    TypeMap::const_iterator temp = m_temporary.find(name);
    if (temp != m_temporary.end()) return temp;

    return m_temporary.end();
}

bool Registry::has(const std::string& name, bool build) const
{
    TypeMap::const_iterator it = find(name);
    if (it != findEnd()) return true;
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
    TypeMap::const_iterator it = find(name);
    if (it != findEnd()) return it->second;
    return 0;
}

void Registry::add(Type* new_type)
{
    std::string name(new_type -> getName());
    const Type* old_type = get(name);
    if (old_type == new_type) return;

    if (old_type)
        throw AlreadyDefined(old_type, new_type);

    const Type::Category cat(new_type -> getCategory());
    if (cat == Type::Array || cat == Type::Pointer)
        m_temporary.insert( std::make_pair(name, new_type) );
    else
        m_persistent.insert( std::make_pair(name, new_type) );
}

