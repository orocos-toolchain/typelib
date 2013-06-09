#include "typemodel.hh"
#include "value.hh"
#include "registry.hh"

#include "typename.hh"
#include <boost/tuple/tuple.hpp>
  
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
        , m_metadata(new MetaData)
    {
        setName(name);
    }

    Type::Type(Type const& type)
        : m_name(type.m_name)
        , m_size(type.m_size)
        , m_category(type.m_category)
        , m_metadata(new MetaData(*type.m_metadata))
    {
    }

    Type::~Type()
    {
        delete m_metadata;
    }

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
        try {
            return left.do_compare(right, equality, stack);
        }
        catch(...) {
            stack.erase(new_it);
            throw;
        }
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
        stack.insert(make_pair(this, new_type));
	registry.add(new_type);
	return *new_type;
    }

    bool Type::resize(Registry& registry, std::map<std::string, std::pair<size_t, size_t> >& new_sizes)
    {
        size_t old_size = getSize();
        if (new_sizes.count(getName()))
            return true;
        else if (do_resize(registry, new_sizes))
        {
            new_sizes.insert(make_pair(getName(), make_pair(old_size, getSize()) ));
            return true;
        }
        else
            return false;
    }

    bool Type::do_resize(Registry& into, std::map<std::string, std::pair<size_t, size_t> >& new_sizes)
    {
        map<std::string, std::pair<size_t, size_t> >::const_iterator it =
            new_sizes.find(getName());
        if (it != new_sizes.end())
        {
            size_t new_size = it->second.second;
            if (getSize() != new_size)
            {
                setSize(new_size);
                return true;
            }
        }
        return false;
    }

    MetaData& Type::getMetaData() const
    {
        return *m_metadata;
    }

    MetaData::Values Type::getMetaData(std::string const& key) const
    {
        return m_metadata->get(key);
    }

    void Type::mergeMetaData(Type const& other) const
    {
        m_metadata->merge(other.getMetaData());
    }

    MetaData::Map const& MetaData::get() const
    {
        return m_values;
    }

    MetaData::Values MetaData::get(std::string const& key) const
    {
        Map::const_iterator it = m_values.find(key);
        if (it == m_values.end())
            return Values();
        return it->second;
    }

    void MetaData::set(std::string const& key, std::string const& value)
    {
        clear(key);
        add(key, value);
    }

    void MetaData::add(std::string const& key, std::string const& value)
    {
        m_values[key].insert(value);
    }

    void MetaData::clear(std::string const& key)
    {
        m_values.erase(key);
    }

    void MetaData::clear()
    {
        m_values.clear();
    }

    void MetaData::merge(MetaData const& metadata)
    {
        Map const& values = metadata.m_values;
        for (Map::const_iterator it = values.begin(); it != values.end(); ++it)
            m_values[it->first].insert(it->second.begin(), it->second.end());
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

    bool Indirect::do_resize(Registry& registry, std::map<std::string, std::pair<size_t, size_t> >& new_sizes)
    {
        bool result = Type::do_resize(registry, new_sizes);
        return registry.get_(getIndirection()).resize(registry, new_sizes) || result;
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

        try  {
            for (FieldList::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
                result->addField(it->getName(), it->getType().merge(registry, stack), it->getOffset());
        }
        catch(...) {
            stack.erase(it);
            throw;
        }

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

    bool Compound::do_resize(Registry& registry, std::map<std::string, std::pair<size_t, size_t> >& new_sizes)
    {
        size_t global_offset = 0;
        for (FieldList::iterator it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            it->setOffset(it->getOffset() + global_offset);

            Type& field_type = registry.get_(it->getType());
            if (field_type.resize(registry, new_sizes))
            {
                size_t old_size = new_sizes.find(field_type.getName())->second.first;
                global_offset += field_type.getSize() - old_size;
            }
        }
        if (global_offset != 0)
        {
            setSize(getSize() + global_offset);
            return true;
        }
        return false;
    }

    void Compound::mergeMetaData(Type const& other) const
    {
        Type::mergeMetaData(other);
        Compound const* other_compound = dynamic_cast<Compound const*>(&other);
        if (other_compound)
        {
            FieldList::const_iterator other_end = other_compound->m_fields.end();
            for (FieldList::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
            {
                Field const* other_field = other_compound->getField(it->getName());
                if (other_field)
                    it->mergeMetaData(*other_field);
            }
        }
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
    {
        Enum const& other_type = static_cast<Enum const&>(type);
        if (!Type::do_compare(type, equality, stack))
            return true;
        if (equality)
            return (m_values == other_type.m_values);
        else
        {
            ValueMap const& other_values = other_type.m_values;
            // returns true if +self+ is a subset of +type+
            if (m_values.size() > other_values.size())
                return false;
            for (ValueMap::const_iterator it = m_values.begin(); it != m_values.end(); ++it)
            {
                ValueMap::const_iterator other_it = other_values.find(it->first);
                if (other_it == other_values.end())
                    return false;
                if (other_it->second != it->second)
                    return false;
            }
            return true;
        }
    }

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

    bool Array::do_resize(Registry& registry, std::map<std::string, std::pair<size_t, size_t> >& new_sizes)
    {
        if (!Indirect::do_resize(registry, new_sizes))
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

    bool Container::isRandomAccess() const { return false; }
    void Container::setElement(void* ptr, int idx, Typelib::Value value) const
    { throw std::logic_error("trying to use setElement on a container that is not random-access"); }
    Typelib::Value Container::getElement(void* ptr, int idx) const
    { throw std::logic_error("trying to use getElement on a container that is not random-access"); }

    Field::Field(const std::string& name, const Type& type)
        : m_name(name), m_type(type), m_offset(0), m_metadata(new MetaData) {}
    Field::~Field()
    {
        delete m_metadata;
    }
    Field::Field(Field const& field)
        : m_name(field.m_name)
        , m_type(field.m_type)
        , m_offset(field.m_offset)
        , m_metadata(new MetaData(*field.m_metadata))
    {
    }

    std::string Field::getName() const { return m_name; }
    Type const& Field::getType() const { return m_type; }
    size_t  Field::getOffset() const { return m_offset; }
    void    Field::setOffset(size_t offset) { m_offset = offset; }
    bool Field::operator == (Field const& field) const
    { return m_offset == field.m_offset && m_name == field.m_name && m_type.isSame(field.m_type); }
    MetaData& Field::getMetaData() const
    { return *m_metadata; }
    MetaData::Values Field::getMetaData(std::string const& key) const
    { return m_metadata->get(key); }
    void Field::mergeMetaData(Field const& field) const
    { return m_metadata->merge(field.getMetaData()); }


    BadCategory::BadCategory(Type::Category found, int expected)
        : TypeException("bad category: found " + lexical_cast<string>(found) + " expecting " + lexical_cast<string>(expected))
        , found(found), expected(expected) {}
    NullTypeFound::NullTypeFound()
        : TypeException("null type found") { }
};

