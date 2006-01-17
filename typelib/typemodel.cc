#include "typemodel.hh"

#include <iostream>
#include <sstream>

#include <numeric>
#include <algorithm>
#include <cassert>
using namespace std;

#include <iostream>

namespace Typelib
{
    Type::Type(string const& name, size_t size, Category category)
        : m_size(size)
        , m_category(category)
        { setName(name); }

    Type::~Type() {}

    string Type::getName() const { return m_name; }
    void Type::setName(const string& name) { m_name = name; }
    Type::Category Type::getCategory() const { return m_category; }

    void   Type::setSize(size_t size) { m_size = size; }
    size_t Type::getSize() const { return m_size; }



    Numeric::Numeric(string const& name, size_t size, NumericCategory category)
        : Type(name, size, Type::Numeric), m_category(category) {}
    Numeric::NumericCategory Numeric::getNumericCategory() const { return m_category; }


    Indirect::Indirect(string const& name, size_t size, Category category, Type const& on)
        : Type(name, size, category)
        , m_indirection(on) {}
    Type const& Indirect::getIndirection() const { return m_indirection; }


    Compound::Compound(std::string const& name)
        : Type(name, 0, Type::Compound) {}

    Compound::FieldList const&  Compound::getFields() const { return m_fields; }
    Field const* Compound::getField(const string& name) const 
    {
        for (FieldList::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (it -> getName() == name)
                return &(*it);
        }
        return 0;
    }

    void Compound::addField(const string& name, Type const& type, size_t offset) 
    { addField( Field(name, type), offset ); }
    void Compound::addField(Field const& field, size_t offset)
    {
        m_fields.push_back(field);
        m_fields.back().setOffset(offset);
        setSize( offset + field.getType().getSize() );
    }

    /*
    bool Type::operator != (const Type& with) const { return ! ((*this) == with); }
    bool Type::operator == (const Type& with) const
    {
        if (getCategory() != with.getCategory()) return false;
        if (getSize() != with.getSize()) return false;

        if (isSimple())
        {
            // simple type with same size and category
            if (isIndirect())
                return (*getNextType()) == (*with.getNextType());
            return true;
        }

        if (m_fields.size() != with.m_fields.size())
            return false;

        FieldList::const_iterator differ 
            = mismatch(m_fields.begin(), m_fields.end(), with.m_fields.begin(), field_compare_types).first;

        return differ == m_fields.end();
    }
    */




    /*
    Struct::Struct(const string& name, bool deftype)
        : Compound("", 0, Type::Struct) 
    { 
        if (deftype) setName(name);
        else 
            setName(getNamespace(name) + "struct " + getTypename(name));
    }
    void Struct::fieldsChanged()
    {
        FieldList& fields(getFields());
        if (fields.empty()) return;

        FieldList::iterator field = fields.begin();
        field -> setOffset(0);

        FieldList::const_iterator last_field = field;
        for (++field; field != fields.end(); ++field)
        {
            int offset = Packing::getOffsetOf(*last_field, *field);
            field -> setOffset(offset);
            last_field = field;
        }

        setSize(last_field->getOffset() + last_field->getType().getSize());
    }


    Union::Union(const string& name, bool deftype)
        : Compound("", 0, Type::Union) 
    { 
        if (deftype) setName(name);
        else 
            setName(getNamespace(name) + "union " + getTypename(name));
    }

    void Union::fieldsChanged()
    {
        size_t size = 0;
        FieldList& fields(getFields());
        for (FieldList::iterator it = fields.begin(); it != fields.end(); ++it)
        {
            const Type& field_type(it -> getType());
            it -> setOffset(0);
            size = std::max(size, field_type.getSize());
        }
        setSize(size);
    }
    */



    namespace
    {
        enum FindSizeOfEnum { EnumField };
        BOOST_STATIC_ASSERT(( sizeof(FindSizeOfEnum) == sizeof(int) ));
    }
    Enum::Enum(const string& name)
        : Type (name, sizeof(FindSizeOfEnum), Type::Enum) { }
    Enum::ValueMap const& Enum::values() const { return m_values; }
    void Enum::add(std::string const& name, int value)
    { 
        std::pair<ValueMap::iterator, bool> inserted;
        inserted = m_values.insert( make_pair(name, value) );
        if (!inserted.second && inserted.first->second != value)
            throw AlreadyExists();
    }
    int  Enum::get(std::string const& name)
    {
        ValueMap::iterator it = m_values.find(name);
        if (it == m_values.end())
            throw DoesNotExist();
        else
            return it->second;
    }


    Array::Array(Type const& of, size_t new_dim)
        : Indirect(getArrayName(of.getName(), new_dim), new_dim * of.getSize(), Type::Array, of)
        , m_dimension(new_dim) { }

    string Array::getArrayName(string const& name, size_t dim)
    { 
        std::ostringstream stream;
        stream << name << '[' << dim << ']';
        return stream.str();
    }
    size_t  Array::getDimension() const { return m_dimension; }



    Pointer::Pointer(const Type& on)
        : Indirect( getPointerName(on.getName()), sizeof(int *), Type::Pointer, on)
    {}
    string Pointer::getPointerName(string const& base)
    { return base + '*'; }




    Field::Field(const string& name, const Type& type)
        : m_name(name), m_type(type), m_offset(0) {}

    string Field::getName() const { return m_name; }
    Type const& Field::getType() const { return m_type; }
    size_t  Field::getOffset() const { return m_offset; }
    void    Field::setOffset(size_t offset) { m_offset = offset; }
};

