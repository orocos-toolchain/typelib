#include "typebuilder.hh"
#include "registry.hh"
#include "typemodel.hh"

#include <numeric>
#include <sstream>

using std::string;
using std::list;

namespace
{
    static string join(const string& acc, const string& add)
    {
        if (acc.empty()) return add;
        return acc + ' ' + add;
    }
}

namespace Typelib
{
    TypeBuilder::TypeBuilder(Registry& registry, const list<string>& name)
        : m_registry(registry) 
    {
        string basename = accumulate(name.begin(), name.end(), string(), join);
        
        m_type = m_registry.build(basename);
        if (!m_type) 
            throw Undefined(basename);
    }
    TypeBuilder::TypeBuilder(Registry& registry, const Type* base)
        : m_type(base), m_registry(registry) { }



    const Type& TypeBuilder::getType() const { return *m_type; }
    void TypeBuilder::addPointer(int level) 
    {
        for (; level; --level)
        {
            // Try to get the type object in the registry
            const Type* base_type = m_registry.get(Pointer::getPointerName(m_type->getName()));
            if (base_type)
                m_type = base_type;
            else
            {
                Type* new_type = new Pointer(*m_type);
                m_registry.add(new_type);
                m_type = new_type;
            }
        }
    }

    void TypeBuilder::addArray(int new_dim)
    {
        const Type* base_type = m_registry.get(Array::getArrayName(m_type->getName(), new_dim));
        if (base_type)
            m_type = base_type;
        else
        {
            Type* new_type = new Array(*m_type, new_dim);
            m_registry.add(new_type);
            m_type = new_type;
        }
    }

    const Type& TypeBuilder::build(Registry& registry, const TypeSpec& spec)
    {
        const Type* base = spec.first;
        const ModifierList& stack(spec.second);

        TypeBuilder builder(registry, base);
        for (ModifierList::const_iterator it = stack.begin(); it != stack.end(); ++it)
        {
            Type::Category category = it -> category;
            int size = it -> size;

            if (category == Type::Pointer)
                builder.addPointer(size);
            else if (category == Type::Array)
                builder.addArray(size);
        }

        return builder.getType();
    }

    TypeBuilder::TypeSpec TypeBuilder::parse(const Registry& registry, const string& full_name)
    {
        static const char* first_chars = "*[";

        TypeSpec spec;

        size_t end = full_name.find_first_of(first_chars);
        string base_name = full_name.substr(0, end);
        spec.first = registry.get(base_name);
        if (! spec.first) throw Undefined(base_name);

        size_t full_length(full_name.length());
        ModifierList& modlist(spec.second);
        while (end < full_length)
        {
            Modifier new_mod;
            if (full_name[end] == '[')
            {
                new_mod.category = Type::Array;
                new_mod.size = atoi(&full_name[end] + 1);
                end = full_name.find(']', end) + 1;
            }
            else if (full_name[end] == '*')
            {
                new_mod.category = Type::Pointer;
                new_mod.size = 1;
                ++end;
            } 
            modlist.push_back(new_mod);
        }

        return spec;
    }

    const Type* TypeBuilder::build(Registry& registry, const string& full_name)
    {
        TypeSpec spec;
        try { spec = parse(registry, full_name); }
        catch(Undefined) { return 0; }

        return &build(registry, spec);
    }

    const Type* TypeBuilder::getBaseType(const Registry& registry, const string& full_name)
    {
        TypeSpec spec;
        try { spec = parse(registry, full_name); }
        catch(Undefined) { return 0; }
        return spec.first;
    }

    std::string TypeBuilder::getBaseTypename(const std::string& full_name)
    {
        static const char* first_chars = "*[";

        size_t end = full_name.find_first_of(first_chars);
        return string(full_name, 0, end);
    }
};

