#include "registry.h"

#include "typebuilder.h"

Registry* Registry::s_self = 0;
Registry* Registry::self() 
{
    if (! s_self)
    {
        s_self = new Registry(StringList());
        s_self -> add(new Type("char", sizeof(char), Type::SInt));
        s_self -> add(new Type("signed char", sizeof(char), Type::SInt));
        s_self -> add(new Type("unsigned char", sizeof(unsigned char), Type::UInt));
        s_self -> add(new Type("short", sizeof(short), Type::SInt));
        s_self -> add(new Type("signed short", sizeof(short), Type::SInt));
        s_self -> add(new Type("unsigned short", sizeof(unsigned short), Type::UInt));
        s_self -> add(new Type("int", sizeof(int), Type::SInt));
        s_self -> add(new Type("signed", sizeof(signed), Type::SInt));
        s_self -> add(new Type("signed int", sizeof(int), Type::SInt));
        s_self -> add(new Type("unsigned", sizeof(unsigned), Type::UInt));
        s_self -> add(new Type("unsigned int", sizeof(unsigned int), Type::UInt));
        s_self -> add(new Type("long", sizeof(long), Type::SInt));
        s_self -> add(new Type("unsigned long", sizeof(unsigned long), Type::UInt));

        s_self -> add(new Type("float", sizeof(float), Type::Float));
        s_self -> add(new Type("double", sizeof(double), Type::Float));
    }

    return s_self;
}

Registry::Registry(const StringList& path) { }

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

    const Type* base_type = TypeBuilder::getBaseType(name);
    return base_type;
}

const Type* Registry::get(const std::string& name, bool build)
{
    TypeMap::const_iterator it = find(name);
    if (it != findEnd()) return it->second;
    if (! build) return false;

    return TypeBuilder::build(name);
}

void Registry::add(Type* new_type)
{
    std::string name(new_type -> getName());
    const Type* old_type = get(name, false);
    if (old_type == new_type) return;

    if (old_type)
        throw AlreadyDefined(old_type, new_type);

    const Type::Category cat(new_type -> getCategory());
    if (cat == Type::Array || cat == Type::Pointer)
        m_temporary.insert( std::make_pair(name, new_type) );
    else
        m_persistent.insert( std::make_pair(name, new_type) );
}

