#include "typebuilder.h"
#include "registry.h"
#include "type.h"

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
    TypeBuilder::TypeBuilder(Registry* registry, const list<string>& name)
        : m_registry(registry) 
        {
            string basename = accumulate(name.begin(), name.end(), string(), join);
            
            m_type = m_registry -> build(basename);
            if (!m_type) throw Undefined(basename);
        }
    TypeBuilder::TypeBuilder(Registry* registry, const Type* base)
        : m_type(base), m_registry(registry) { }



    const Type* TypeBuilder::getType() const { return m_type; }

    void TypeBuilder::addPointer(int level) 
    {
        while (level)
        {
            // Try to get the type object in the registry
            const Type* base_type = m_registry -> get(m_type -> getName() + "*");
            if (! base_type)
            {
                Type* new_type = new Pointer(m_type);
                m_registry -> add(new_type);
                m_type = new_type;
            }
            else
                m_type = base_type;

            --level;
        }
    }

    void TypeBuilder::addArray(int new_dim)
    {
        // May seem weird, but we have to handle
        // int test[4][5] as a 4 item array of a 5 item array of int
        typedef list<const Array *> ArrayList;
        ArrayList array_chain;

        const Type* base_type;
        for (base_type = m_type; base_type -> getCategory() == Type::Array; base_type = base_type -> getNextType())
        {
            const Array* array = static_cast<const Array*>(base_type);
            array_chain.push_front(array);
        }

        string basename;
        // Try to get the first array level in the registry
        {   
            std::ostringstream stream;
            stream << base_type -> getName() << "[" << new_dim << "]";
            basename = stream.str();

            const Type* new_base = m_registry -> get(basename);
            if (new_base)
                base_type = new_base;
            else base_type = new Array(base_type, new_dim); 
        }

        // Find as much already-built object as possible
        ArrayList::const_iterator cur_dim;
        for (cur_dim = array_chain.begin(); cur_dim != array_chain.end(); ++cur_dim)
        {
            const Array* array = *cur_dim;
            const Type* new_type = m_registry -> get(basename + array -> getDimString());
            if (! new_type) break;
        }

        // Build the remaining ones
        for (; cur_dim != array_chain.end(); ++cur_dim)
        {
            const Array* array = *cur_dim;

            Type* new_type = new Array(base_type, array -> getDimension());
            m_registry -> add(new_type);
            base_type = new_type;
        }

        m_type = base_type;
    }

    const Type* TypeBuilder::build(Registry* registry, const TypeSpec& spec)
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

    TypeBuilder::TypeSpec TypeBuilder::parse(const Registry* registry, const string& full_name)
    {
        static const char* first_chars = "*[";

        TypeSpec spec;

        int end = full_name.find_first_of(first_chars);
        string base_name = full_name.substr(0, end);
        spec.first = registry -> get(base_name);
        if (! spec.first) throw Undefined(base_name);

        int full_length(full_name.length());
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

    const Type* TypeBuilder::build(Registry* registry, const string& full_name)
    {
        TypeSpec spec;
        try { spec = parse(registry, full_name); }
        catch(Undefined) { return 0; }

        return build(registry, spec);
    }

    const Type* TypeBuilder::getBaseType(const Registry* registry, const string& full_name)
    {
        TypeSpec spec;
        try { spec = parse(registry, full_name); }
        catch(...) { return 0; }
        return spec.first;
    }
};

