#include "typebuilder.hh"
#include "registry.hh"
#include "typemodel.hh"

#include <numeric>
#include <sstream>

using std::string;
using std::list;

namespace {
static string join(const string &acc, const string &add) {
    if (acc.empty())
        return add;
    return acc + ' ' + add;
}
}

namespace Typelib {
TypeBuilder::TypeBuilder(Registry &registry, const std::list<std::string> &name)
    : m_registry(registry) {
    string basename = accumulate(name.begin(), name.end(), string(), join);

    m_type = m_registry.get_(basename);
    if (!m_type)
        throw Undefined(basename);
}
TypeBuilder::TypeBuilder(Registry &registry, const Type *base)
    : m_type(const_cast<Type *>(base)), m_registry(registry) {}

const Type &TypeBuilder::getType() const { return *m_type; }
void TypeBuilder::addPointer(int level) {
    for (; level; --level) {
        // Try to get the type object in the registry
        Type *base_type =
            m_registry.get_(Pointer::getPointerName(m_type->getName()));
        if (base_type)
            m_type = base_type;
        else {
            Type *new_type = new Pointer(*m_type);
            m_registry.add(new_type);
            m_type = new_type;
        }
    }
}

void TypeBuilder::addArrayMinor(int new_dim) {
    // build the list of array dimensions
    std::vector<int> dims;

    while (m_type->getCategory() == Type::Array) {
        Array *array = dynamic_cast<Array *>(m_type);
        dims.push_back(array->getDimension());
        m_type = const_cast<Type *>(&array->getIndirection());
    }

    addArrayMajor(new_dim);
    for (std::vector<int>::reverse_iterator it = dims.rbegin();
         it != dims.rend(); ++it)
        addArrayMajor(*it);
}

void TypeBuilder::addArrayMajor(int new_dim) {
    Type *base_type =
        m_registry.get_(Array::getArrayName(m_type->getName(), new_dim));
    if (base_type)
        m_type = base_type;
    else {
        Type *new_type = new Array(*m_type, new_dim);
        m_registry.add(new_type);
        m_type = new_type;
    }
}

void TypeBuilder::setSize(int size) { m_type->setSize(size); }

const Type &TypeBuilder::build(Registry &registry, const TypeSpec &spec,
                               int size) {
    const Type *base = spec.first;
    const ModifierList &stack(spec.second);

    TypeBuilder builder(registry, base);
    for (ModifierList::const_iterator it = stack.begin(); it != stack.end();
         ++it) {
        Type::Category category = it->category;
        int size = it->size;

        if (category == Type::Pointer)
            builder.addPointer(size);
        else if (category == Type::Array)
            builder.addArrayMajor(size);
    }

    if (size != 0)
        builder.setSize(size);
    return builder.getType();
}

struct InvalidIndirectName : public std::runtime_error {
    InvalidIndirectName(std::string const &what) : std::runtime_error(what) {}
};

TypeBuilder::TypeSpec TypeBuilder::parse(const Registry &registry,
                                         const std::string &full_name) {
    static const char *first_chars = "*[";

    TypeSpec spec;

    size_t end = full_name.find_first_of(first_chars);
    string base_name = full_name.substr(0, end);
    spec.first = registry.get(base_name);
    if (!spec.first)
        throw Undefined(base_name);

    size_t full_length(full_name.length());
    ModifierList &modlist(spec.second);
    while (end < full_length) {
        Modifier new_mod;
        if (full_name[end] == '[') {
            new_mod.category = Type::Array;
            new_mod.size = atoi(&full_name[end] + 1);
            end = full_name.find(']', end) + 1;
        } else if (full_name[end] == '*') {
            new_mod.category = Type::Pointer;
            new_mod.size = 1;
            ++end;
        } else
            throw InvalidIndirectName(full_name + " is not a valid type name");
        modlist.push_back(new_mod);
    }

    return spec;
}

const Type *TypeBuilder::build(Registry &registry, const std::string &full_name,
                               int size) {
    TypeSpec spec;
    try {
        spec = parse(registry, full_name);
    } catch (Undefined) {
        return 0;
    }

    return &build(registry, spec, size);
}

const Type *TypeBuilder::getBaseType(const Registry &registry,
                                     const std::string &full_name) {
    TypeSpec spec;
    try {
        spec = parse(registry, full_name);
    } catch (Undefined) {
        return 0;
    }
    return spec.first;
}

std::string TypeBuilder::getBaseTypename(const std::string &full_name) {
    static const char *first_chars = "*[";

    size_t end = full_name.find_first_of(first_chars);
    return string(full_name, 0, end);
}
};
