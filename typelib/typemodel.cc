#include "typemodel.hh"
#include "registry.hh"

#include <iostream>
#include <sstream>

#include <numeric>
#include <algorithm>
#include <cassert>
using namespace std;

#include <iostream>

namespace Typelib
{
    class InternalError : public std::exception
    {
	std::string const m_message;
    public:
	InternalError(std::string const& message)
	    : m_message(message) {}
	~InternalError() throw() {}
	char const* what() const throw() { return m_message.c_str(); }
    };
    Type::Type(std::string const& name, size_t size, Category category)
        : m_size(size)
        , m_category(category)
        { setName(name); }

    Type::~Type() {}

    std::string Type::getName() const { return m_name; }
    void Type::setName(const std::string& name) { m_name = name; }
    Type::Category Type::getCategory() const { return m_category; }

    void   Type::setSize(size_t size) { m_size = size; }
    size_t Type::getSize() const { return m_size; }
    bool   Type::isNull() const { return m_category == NullType; }
    bool   Type::operator != (Type const& with) const { return (this != &with); }
    bool   Type::operator == (Type const& with) const { return (this == &with); }
    bool   Type::isSame(Type const& with) const
    { return (getSize() == with.getSize() && getCategory() == with.getCategory() && getName() == with.getName()); }
    Type const& Type::merge(Registry& registry) const
    {
	Type const* old_type = registry.get(getName());
	if (old_type)
	{
	    if (old_type->isSame(*this))
		return *old_type;
	    else
		throw DefinitionMismatch(getName());
	}
	Type* new_type = do_merge(registry);
	registry.add(new_type, "");
	return *new_type;
    }


    Numeric::Numeric(std::string const& name, size_t size, NumericCategory category)
        : Type(name, size, Type::Numeric), m_category(category) {}
    Numeric::NumericCategory Numeric::getNumericCategory() const { return m_category; }
    bool Numeric::isSame(Type const& type) const 
    { return Type::isSame(type) && 
	m_category == static_cast<Numeric const&>(type).m_category; }
    Type* Numeric::do_merge(Registry& registry) const
    { return new Numeric(*this); }

    Indirect::Indirect(std::string const& name, size_t size, Category category, Type const& on)
        : Type(name, size, category)
        , m_indirection(on) {}
    Type const& Indirect::getIndirection() const { return m_indirection; }
    bool Indirect::isSame(Type const& type) const
    { return Type::isSame(type) && 
	m_indirection.isSame(static_cast<Indirect const&>(type).m_indirection); }


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
    bool Compound::isSame(Type const& type) const
    { return Type::isSame(type) &&
	    m_fields == static_cast<Compound const&>(type).m_fields; }

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
    Type* Compound::do_merge(Registry& registry) const
    {
	auto_ptr<Compound> result(new Compound(getName()));
	for (FieldList::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
	    result->addField(it->getName(), it->getType().merge(registry), it->getOffset());
	return result.release();
    }

    namespace
    {
        enum FindSizeOfEnum { EnumField };
        BOOST_STATIC_ASSERT(( sizeof(FindSizeOfEnum) == sizeof(Enum::integral_type) ));
    }
    Enum::Enum(const std::string& name)
        : Type (name, sizeof(FindSizeOfEnum), Type::Enum) { }
    Enum::ValueMap const& Enum::values() const { return m_values; }
    bool Enum::isSame(Type const& type) const
    { return Type::isSame(type) &&
	m_values == static_cast<Enum const&>(type).m_values; }

    void Enum::add(std::string const& name, int value)
    { 
        std::pair<ValueMap::iterator, bool> inserted;
        inserted = m_values.insert( make_pair(name, value) );
        if (!inserted.second && inserted.first->second != value)
            throw AlreadyExists();
    }
    Enum::integral_type  Enum::get(std::string const& name) const
    {
        ValueMap::const_iterator it = m_values.find(name);
        if (it == m_values.end())
            throw SymbolNotFound();
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
        throw ValueNotFound();
    }
    std::list<std::string> Enum::names() const
    {
        list<string> ret;
        ValueMap::const_iterator it;
        for (it = m_values.begin(); it != m_values.end(); ++it)
            ret.push_back(it->first);
        return ret;
    }
    Type* Enum::do_merge(Registry& registry) const
    { return new Enum(*this); }

    Array::Array(Type const& of, size_t new_dim)
        : Indirect(getArrayName(of.getName(), new_dim), new_dim * of.getSize(), Type::Array, of)
        , m_dimension(new_dim) { }

    std::string Array::getArrayName(std::string const& name, size_t dim)
    { 
        std::ostringstream stream;
        stream << name << '[' << dim << ']';
        return stream.str();
    }
    size_t  Array::getDimension() const { return m_dimension; }
    bool Array::isSame(Type const& type) const
    {
	if (!Type::isSame(type))
	    return false;

	Array const& array = static_cast<Array const&>(type);
	return (m_dimension == array.m_dimension) && Indirect::isSame(type);
    }
    Type* Array::do_merge(Registry& registry) const
    { return new Array(getIndirection().merge(registry), m_dimension); }

    Pointer::Pointer(const Type& on)
        : Indirect( getPointerName(on.getName()), sizeof(int *), Type::Pointer, on) {}
    std::string Pointer::getPointerName(std::string const& base)
    { return base + '*'; }
    Type* Pointer::do_merge(Registry& registry) const
    { return new Pointer(getIndirection().merge(registry)); }




    Field::Field(const std::string& name, const Type& type)
        : m_name(name), m_type(type), m_offset(0) {}

    std::string Field::getName() const { return m_name; }
    Type const& Field::getType() const { return m_type; }
    size_t  Field::getOffset() const { return m_offset; }
    void    Field::setOffset(size_t offset) { m_offset = offset; }
    bool Field::operator == (Field const& field) const
    { return m_offset == field.m_offset && m_name == field.m_name && m_type.isSame(field.m_type); }
};

