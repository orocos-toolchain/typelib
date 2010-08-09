#include "typemodel.hh"
#include "registry.hh"

#include <iostream>
#include <sstream>

#include <numeric>
#include <algorithm>
#include <cassert>
using namespace std;

#include <iostream>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

namespace Typelib
{
    Type::Type(std::string const& name, size_t size, Category category)
        : m_size(size)
        , m_category(category)
        { setName(name); }

    Type::~Type() {}

    std::string Type::getName() const { return m_name; }
    std::string Type::getBasename() const { return getTypename(m_name); }
    std::string Type::getNamespace() const { return Typelib::getNamespace(m_name); }
    void Type::setName(const std::string& name) { m_name = name; }
    Type::Category Type::getCategory() const { return m_category; }
    void Type::modifiedDependencyAliases(Registry& registry) const {}

    void   Type::setSize(size_t size) { m_size = size; }
    size_t Type::getSize() const { return m_size; }
    bool   Type::isNull() const { return m_category == NullType; }
    bool   Type::operator != (Type const& with) const { return (this != &with); }
    bool   Type::operator == (Type const& with) const { return (this == &with); }
    bool   Type::isSame(Type const& with) const
    { 
        if (this == &with)
            return true;

        RecursionStack stack;
        stack.insert(make_pair(this, &with));
        return do_compare(with, true, stack);
    }
    bool   Type::canCastTo(Type const& with) const
    {
        if (this == &with)
            return true;

        RecursionStack stack;
        stack.insert(make_pair(this, &with));
        return do_compare(with, false, stack);
    }

    unsigned int Type::getTrailingPadding() const
    { return 0; }

    bool Type::rec_compare(Type const& left, Type const& right, bool equality, RecursionStack& stack) const
    {
        if (&left == &right)
            return true;

        RecursionStack::const_iterator it = stack.find(&left);
        if (it != stack.end())
        {
            /** One type cannot be equal to two different types. So either we
             * are already comparing left and right and it is fine (return
             * true). Or we are comparing it with a different type and that
             * means the two types are different (return false)
             */
            return (&right == it->second);
        }

        RecursionStack::iterator new_it = stack.insert( make_pair(&left, &right) ).first;
        bool result = left.do_compare(right, equality, stack);
        stack.erase(new_it);
        return result;
    }
    bool Type::do_compare(Type const& other, bool equality, RecursionStack& stack) const
    { return (getSize() == other.getSize() && getCategory() == other.getCategory()); }

    Type const& Type::merge(Registry& registry) const
    {
        RecursionStack stack;
        return merge(registry, stack);
    }
    Type const* Type::try_merge(Registry& registry, RecursionStack& stack) const
    {
        RecursionStack::iterator it = stack.find(this);
        if (it != stack.end())
            return it->second;

	Type const* old_type = registry.get(getName());
	if (old_type)
	{
            if (old_type->isSame(*this))
		return old_type;
	    else
		throw DefinitionMismatch(getName());
	}
        return 0;
    }
    Type const& Type::merge(Registry& registry, RecursionStack& stack) const
    {
        Type const* old_type = try_merge(registry, stack);
        if (old_type)
            return *old_type;

	Type* new_type = do_merge(registry, stack);
	registry.add(new_type);
	return *new_type;
    }

    bool Type::resize(Registry& registry, std::map<std::string, size_t>& new_sizes)
    {
        if (do_resize(registry, new_sizes))
        {
            new_sizes.insert(make_pair(getName(), getSize()));
            return true;
        }
        else
            return false;
    }

    bool Type::do_resize(Registry& into, std::map<std::string, size_t>& new_sizes)
    {
        map<std::string, size_t>::const_iterator it =
            new_sizes.find(getName());
        if (it != new_sizes.end())
        {
            if (getSize() != it->second)
            {
                setSize(it->second);
                return true;
            }
        }
        return false;
    }

    bool OpaqueType::do_compare(Type const& other, bool equality, std::map<Type const*, Type const*>& stack) const
    { return Type::do_compare(other, equality, stack) && getName() == other.getName(); }

    Numeric::Numeric(std::string const& name, size_t size, NumericCategory category)
        : Type(name, size, Type::Numeric), m_category(category) {}
    Numeric::NumericCategory Numeric::getNumericCategory() const { return m_category; }
    bool Numeric::do_compare(Type const& type, bool equality, RecursionStack& stack) const 
    { return getSize() == type.getSize() && getCategory() == type.getCategory() && 
	m_category == static_cast<Numeric const&>(type).m_category; }
    Type* Numeric::do_merge(Registry& registry, RecursionStack& stack) const
    { return new Numeric(*this); }

    Indirect::Indirect(std::string const& name, size_t size, Category category, Type const& on)
        : Type(name, size, category)
        , m_indirection(on) {}
    Type const& Indirect::getIndirection() const { return m_indirection; }
    bool Indirect::do_compare(Type const& type, bool equality, RecursionStack& stack) const
    { return Type::do_compare(type, equality, stack) &&
        rec_compare(m_indirection, static_cast<Indirect const&>(type).m_indirection, equality, stack); }
    std::set<Type const*> Indirect::dependsOn() const
    {
	std::set<Type const*> result;
	result.insert(&getIndirection());
	return result;
    }
    Type const& Indirect::merge(Registry& registry, RecursionStack& stack) const
    {
        Type const* old_type = try_merge(registry, stack);
        if (old_type) return *old_type;
        // First, make sure the indirection is already merged and then merge
        // this type
        getIndirection().merge(registry, stack);

        return Type::merge(registry, stack);
    }

    void Indirect::modifiedDependencyAliases(Registry& registry) const
    {
        std::string full_name = getName();
        set<string> aliases = registry.getAliasesOf(getIndirection());
        for (set<string>::const_iterator alias_it = aliases.begin();
                alias_it != aliases.end(); ++alias_it)
        {
            std::string alias_name = getIndirectTypeName(*alias_it);
            if (!registry.has(alias_name, false))
                registry.alias(full_name, alias_name, false);
        }
    }

    Compound::Compound(std::string const& name)
        : Type(name, 0, Type::Compound) {}

    Compound::FieldList const&  Compound::getFields() const { return m_fields; }
    Field const* Compound::getField(const std::string& name) const 
    {
        for (FieldList::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (it -> getName() == name)
                return &(*it);
        }
        return 0;
    }
    unsigned int Compound::getTrailingPadding() const
    {
        if (m_fields.empty())
            return getSize();

        FieldList::const_iterator it = m_fields.begin();
        FieldList::const_iterator const end = m_fields.end();

        int max_offset = 0;
        for (; it != end; ++it)
        {
            int offset = it->getOffset() + it->getType().getSize();
            if (offset > max_offset)
                max_offset = offset;
        }
        return getSize() - max_offset;
    }
    bool Compound::do_compare(Type const& type, bool equality, RecursionStack& stack) const
    { 
        if (type.getCategory() != Type::Compound)
            return false;
        if (equality && !Type::do_compare(type, true, stack))
            return false;

        Compound const& right_type = static_cast<Compound const&>(type);
        if (m_fields.size() != right_type.getFields().size())
            return false;

        FieldList::const_iterator left_it = m_fields.begin(),
            left_end = m_fields.end(),
            right_it = right_type.getFields().begin(),
            right_end = right_type.getFields().end();

        while (left_it != left_end)
        {
            if (left_it->getName() != right_it->getName()
                    || left_it->m_offset != right_it->m_offset
                    || left_it->m_name != right_it->m_name)
                return false;

            if (!rec_compare(left_it->getType(), right_it->getType(), equality, stack))
                return false;
            ++left_it; ++right_it;
        }
        return true;
    }

    void Compound::addField(const std::string& name, Type const& type, size_t offset) 
    { addField( Field(name, type), offset ); }
    void Compound::addField(Field const& field, size_t offset)
    {
        m_fields.push_back(field);
        m_fields.back().setOffset(offset);
	size_t old_size = getSize();
	size_t new_size = offset + field.getType().getSize();
	if (old_size < new_size)
	    setSize(new_size);
    }
    Type* Compound::do_merge(Registry& registry, RecursionStack& stack) const
    {
	auto_ptr<Compound> result(new Compound(getName()));
        RecursionStack::iterator it = stack.insert(make_pair(this, result.get())).first;

	for (FieldList::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
	    result->addField(it->getName(), it->getType().merge(registry, stack), it->getOffset());

        stack.erase(it);
        result->setSize(getSize());
	return result.release();
    }
    std::set<Type const*> Compound::dependsOn() const
    {
	std::set<Type const*> result;
        for (FieldList::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
	    result.insert(&(it->getType()));
	return result;
    }

    bool Compound::do_resize(Registry& registry, std::map<std::string, size_t>& new_sizes)
    {
        size_t global_offset = 0;
        for (FieldList::iterator it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            it->setOffset(it->getOffset() + global_offset);

            Type& field_type = registry.get_(it->getType());
            size_t old_size = field_type.getSize();
            if (field_type.resize(registry, new_sizes))
                global_offset += field_type.getSize() - old_size;
        }
        if (global_offset != 0)
        {
            setSize(getSize() + global_offset);
            return true;
        }
        return false;
    }


    Enum::AlreadyExists::AlreadyExists(Type const& type, std::string const& name)
        : std::runtime_error("enumeration symbol " + name + " is already used in " + type.getName()) {}
    Enum::SymbolNotFound::SymbolNotFound(Type const& type, std::string const& name)
            : std::runtime_error("enumeration symbol " + name + " does not exist in " + type.getName()) {}
    Enum::ValueNotFound::ValueNotFound(Type const& type, int value)
            : std::runtime_error("no symbol associated with " + boost::lexical_cast<std::string>(value)  + " in " + type.getName()) {}

    namespace
    {
        enum FindSizeOfEnum { EnumField };
        BOOST_STATIC_ASSERT(( sizeof(FindSizeOfEnum) == sizeof(Enum::integral_type) ));
    }
    Enum::Enum(const std::string& name, Enum::integral_type initial_value)
        : Type (name, sizeof(FindSizeOfEnum), Type::Enum)
        , m_last_value(initial_value - 1) { }
    Enum::ValueMap const& Enum::values() const { return m_values; }
    bool Enum::do_compare(Type const& type, bool equality, RecursionStack& stack) const
    { return Type::do_compare(type, equality, stack) &&
	m_values == static_cast<Enum const&>(type).m_values; }

    Enum::integral_type Enum::getNextValue() const { return m_last_value + 1; }
    void Enum::add(std::string const& name, int value)
    { 
        std::pair<ValueMap::iterator, bool> inserted;
        inserted = m_values.insert( make_pair(name, value) );
        if (!inserted.second && inserted.first->second != value)
            throw AlreadyExists(*this, name);
        m_last_value = value;
    }
    Enum::integral_type  Enum::get(std::string const& name) const
    {
        ValueMap::const_iterator it = m_values.find(name);
        if (it == m_values.end())
            throw SymbolNotFound(*this, name);
        else
            return it->second;
    }
    std::string Enum::get(Enum::integral_type value) const
    {
        ValueMap::const_iterator it;
        for (it = m_values.begin(); it != m_values.end(); ++it)
        {
            if (it->second == value)
                return it->first;
        }
        throw ValueNotFound(*this, value);
    }
    std::list<std::string> Enum::names() const
    {
        list<string> ret;
        ValueMap::const_iterator it;
        for (it = m_values.begin(); it != m_values.end(); ++it)
            ret.push_back(it->first);
        return ret;
    }
    Type* Enum::do_merge(Registry& registry, RecursionStack& stack) const
    { return new Enum(*this); }

    Array::Array(Type const& of, size_t new_dim)
        : Indirect(getArrayName(of.getName(), new_dim), new_dim * of.getSize(), Type::Array, of)
        , m_dimension(new_dim) { }

    std::string Array::getIndirectTypeName(std::string const& inside_type_name) const
    {
        return Array::getArrayName(inside_type_name, getDimension());
    }

    std::string Array::getArrayName(std::string const& name, size_t dim)
    { 
        std::ostringstream stream;
        stream << name << '[' << dim << ']';
        return stream.str();
    }
    size_t  Array::getDimension() const { return m_dimension; }
    bool Array::do_compare(Type const& type, bool equality, RecursionStack& stack) const
    {
	if (!Type::do_compare(type, equality, stack))
	    return false;

	Array const& array = static_cast<Array const&>(type);
	return (m_dimension == array.m_dimension) && Indirect::do_compare(type, true, stack);
    }
    Type* Array::do_merge(Registry& registry, RecursionStack& stack) const
    {
        // The pointed-to type has already been inserted in the repository by
        // Indirect::merge, so we don't have to worry.
        Type const& indirect_type = getIndirection().merge(registry, stack);
        return new Array(indirect_type, m_dimension);
    }

    bool Array::do_resize(Registry& registry, std::map<std::string, size_t>& new_sizes)
    {
        if (!registry.get_(getIndirection()).resize(registry, new_sizes))
            return false;

        setSize(getDimension() * getIndirection().getSize());
        return true;
    }

    Pointer::Pointer(const Type& on)
        : Indirect( getPointerName(on.getName()), sizeof(int *), Type::Pointer, on) {}
    std::string Pointer::getPointerName(std::string const& base)
    { return base + '*'; }
    std::string Pointer::getIndirectTypeName(std::string const& inside_type_name) const
    {
        return Pointer::getPointerName(inside_type_name);
    }

    Type* Pointer::do_merge(Registry& registry, RecursionStack& stack) const
    {
        // The pointed-to type has already been inserted in the repository by
        // Indirect::merge, so we don't have to worry.
        Type const& indirect_type = getIndirection().merge(registry, stack);
        return new Pointer(indirect_type);
    }

    Container::Container(std::string const& kind, std::string const& name, size_t size, Type const& of)
        : Indirect( name, size, Type::Container, of )
        , m_kind(kind) {}

    std::string Container::kind() const { return m_kind; }

    Container::AvailableContainers* Container::s_available_containers;
    Container::AvailableContainers Container::availableContainers() {
        if (!s_available_containers)
            return Container::AvailableContainers();
        return *s_available_containers;
    }

    void Container::registerContainer(std::string const& name, ContainerFactory factory)
    {
        if (s_available_containers == 0)
            s_available_containers = new Container::AvailableContainers;

        (*s_available_containers)[name] = factory;
    }
    Container const& Container::createContainer(Registry& r, std::string const& name, Type const& on)
    {
        std::list<Type const*> temp;
        temp.push_back(&on);
        return createContainer(r, name, temp);
    }
    Container const& Container::createContainer(Registry& r, std::string const& name, std::list<Type const*> const& on)
    {
        AvailableContainers::const_iterator it = s_available_containers->find(name);
        if (it == s_available_containers->end())
            throw UnknownContainer(name);
        return (*it->second)(r, on);
    }

    bool Container::do_compare(Type const& other, bool equality, RecursionStack& stack) const
    {
	if (!Type::do_compare(other, true, stack))
	    return false;

	Container const& container = static_cast<Container const&>(other);
	return (getFactory() == container.getFactory()) && Indirect::do_compare(other, true, stack);
    }

    Type* Container::do_merge(Registry& registry, RecursionStack& stack) const
    {
        // The pointed-to type has already been inserted in the repository by
        // Indirect::merge, so we don't have to worry.
        Type const& indirect_type = getIndirection().merge(registry, stack);
        std::list<Type const*> on;
        on.push_back(&indirect_type);
        Container* result = const_cast<Container*>(&(*getFactory())(registry, on));
        result->setSize(getSize());
        return result;
    }


    Field::Field(const std::string& name, const Type& type)
        : m_name(name), m_type(type), m_offset(0) {}

    std::string Field::getName() const { return m_name; }
    Type const& Field::getType() const { return m_type; }
    size_t  Field::getOffset() const { return m_offset; }
    void    Field::setOffset(size_t offset) { m_offset = offset; }
    bool Field::operator == (Field const& field) const
    { return m_offset == field.m_offset && m_name == field.m_name && m_type.isSame(field.m_type); }


    BadCategory::BadCategory(Type::Category found, int expected)
        : TypeException("bad category: found " + lexical_cast<string>(found) + " expecting " + lexical_cast<string>(expected))
        , found(found), expected(expected) {}
    NullTypeFound::NullTypeFound()
        : TypeException("null type found") { }
};

