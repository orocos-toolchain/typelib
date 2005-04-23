#include "type.hh"

#include <iostream>
#include <sstream>

#include <numeric>
#include <algorithm>
#include <cassert>

#include "packing.hh"
using namespace std;

#include <iostream>

namespace Typelib
{
    Type::Type(const std::string& name, size_t size, Category category)
        : m_size(size), m_category(category), m_next_type(0)
        {
            if (!name.empty())
                setName(name);
        }
    Type::Type(const std::string& name, const Type* from)
        : m_size(from->m_size), m_category(from->m_category), 
        m_next_type(from->m_next_type), m_fields(from->m_fields)
        {
            if (!name.empty())
                setName(name);
        }
    Type::~Type() {}

    std::string Type::getName() const { return m_name; }
    void Type::setName(const std::string& name) { m_name = name; }
    void Type::prependName(const std::string& prepend)
    {
        const std::string name(getName());
    }
    Type::Category Type::getCategory() const { return m_category; }

    bool Type::isSimple() const
    {
        switch(m_category)
        {
            case Pointer:
            case SInt:
            case UInt:
            case Float:
            case Array:
                return true;
            case Struct:
            case Union:
                return false;
            case NullType:
                throw NullTypeFound();
        };

        // never reached
        return false;
    };

    const Type* Type::getNextType() const { return m_next_type; }
    bool Type::isIndirect() const
    {
        switch(m_category)
        {
            case Pointer:
            case Array:
                return true;
            case UInt:
            case SInt:
            case Float:
            case Struct:
            case Union:
                return false;
            case NullType:
                throw NullTypeFound();
        };


        // never reached
        return false;
    }

    void   Type::setSize(size_t size) { m_size = size; }
    size_t Type::getSize() const { return m_size; }

    const Type::FieldList&  Type::getFields() const { return m_fields; }
    const Field*            Type::getField(const std::string& name) const 
    {
        for (FieldList::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (it -> getName() == name)
                return &(*it);
        }
        return 0;
    }
    void Type::fieldsChanged() {}

    void Type::addField(const std::string& name, const Type* type) 
    { addField( Field(name, type) ); }
    void Type::addField(const Field& field)
    {
        if (isSimple())
        {
            std::cerr << "ERROR: addField called on a simple type" << std::endl;
            return;
        }

        m_fields.push_back(field);
        fieldsChanged();
    }

    void Type::setNextType(const Type* type) { m_next_type = type; }




    std::string Type::toString(const std::string& prefix, bool recursive) const
    {
        using namespace std;

        ostringstream output;
        output << prefix << getName();

        if (isSimple()) 
        {
            output << std::endl << prefix << "\t";
            Category cat = getCategory();
            if (isIndirect())
            {
                output << getSize() << " bytes "
                    << ((cat == Array) ? " array of " : " pointer on ") << endl;

                const Type* next_type = getNextType();
                if (recursive || next_type -> isIndirect())
                    output << next_type -> toString(prefix + "\t", recursive);
                else
                    output << prefix << "\t" << next_type -> getName();
            }
            else
            {
                output << getSize() * 8 << " bits ";
                switch (getCategory())
                {
                    case SInt:
                        output << "signed integer ";
                        break;
                    case UInt:
                        output << "unsigned integer ";
                        break;
                    case Float:
                        output << "floating point number ";
                        break;
                    default: break;
                };
            }
        }
        else
        {
            output << " {" << std::endl;
            for (FieldList::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
            {
                output 
                    << prefix << "\t"
                    << "(+" << it -> getOffset() << ") "
                    << it -> getName() << " ";
                if (recursive)
                    output << it -> getType() -> toString(prefix + "\t");
                else
                    output << it -> getType() -> getName();
                output << " " << std::endl;
            }
            output << prefix << "}";
        }

        return output.str();
    }

    static bool field_compare_types(const Field& field1, const Field& field2)
    { return *(field1.getType()) == *(field2.getType()); }

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




    Struct::Struct(const std::string& name, bool deftype)
        : Type("", 0, Type::Struct) 
    { 
        if (deftype) setName(name);
        else 
            setName(getNamespace(name) + "struct " + getTypename(name));
    }
    void Struct::fieldsChanged()
    {
        if (m_fields.empty()) return;

        FieldList::iterator field = m_fields.begin();
        field -> setOffset(0);

        FieldList::const_iterator last_field = field;
        for (++field; field != m_fields.end(); ++field)
        {
            int offset = PackingInfo::getOffsetOf(*last_field, *field);
            field -> setOffset(offset);
            last_field = field;
        }

        setSize(last_field->getOffset() + last_field->getType()->getSize());
    }


    Union::Union(const std::string& name, bool deftype)
        : Type("", 0, Type::Union) 
    { 
        if (deftype) setName(name);
        else 
            setName(getNamespace(name) + "union " + getTypename(name));
    }

    void Union::fieldsChanged()
    {
        size_t size = 0;
        for (FieldList::iterator it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            const Type* field_type(it -> getType());
            it -> setOffset(0);
            size = std::max(size, field_type -> getSize());
        }
        setSize(size);
    }




    namespace
    {
        enum FindSizeOfEnum { EnumField };
    }
    Enum::Enum(const std::string& name)
        : Type (name, sizeof(FindSizeOfEnum), Type::SInt) { }

    Array::Array(const Type* of, int new_dim)
        : Type("", 0, Type::Array), m_dimension(new_dim)
    {
        if (of -> getCategory() == Type::Array)
        {
            const Array* base_array = static_cast<const Array *>(of);
            m_basename = base_array -> getBasename();
            m_dim_string = base_array -> getDimString();
        }
        else
        {
            m_basename = of -> getName();
        }

        {
            std::ostringstream dimensions;
            dimensions << '[' << new_dim << ']' << m_dim_string;
            m_dim_string = dimensions.str();
        }

        setName(m_basename + m_dim_string);
        setSize(new_dim * of -> getSize());
        setNextType(of);
    }

    int Array::getDimension() const { return m_dimension; }
    std::string Array::getBasename() const { return m_basename; }
    std::string Array::getDimString() const { return m_dim_string; }

    Pointer::Pointer(const Type* on)
        : Type(on -> getName() + "*", sizeof(int *), Type::Pointer)
        { setNextType(on); }




    Field::Field(const std::string& name, const Type* type)
        : m_name(name), m_type(type), m_offset(0) {}

    std::string Field::getName() const { return m_name; }
    const Type* Field::getType() const { return m_type; }
    size_t  Field::getOffset() const { return m_offset; }
    void    Field::setOffset(size_t offset) { m_offset = offset; }
};

