#include "type.h"

#include <iostream>
#include <sstream>

#include <numeric>
#include <algorithm>
#include <cassert>
using namespace std;

bool          Type::s_init = false;
Type::TypeMap Type::s_typemap;

// WARNING: not mt safe for now
const Type* Type::fromName(const std::string& name)
{
    if (! s_init)
    {
        new Type("char", sizeof(char), SInt);
        new Type("signed char", sizeof(char), SInt);
        new Type("unsigned char", sizeof(unsigned char), UInt);
        new Type("short", sizeof(short), SInt);
        new Type("signed short", sizeof(short), SInt);
        new Type("unsigned short", sizeof(unsigned short), UInt);
        new Type("int", sizeof(int), SInt);
        new Type("signed", sizeof(signed), SInt);
        new Type("signed int", sizeof(int), SInt);
        new Type("unsigned", sizeof(unsigned), UInt);
        new Type("unsigned int", sizeof(unsigned int), UInt);
        new Type("long", sizeof(long), SInt);
        new Type("unsigned long", sizeof(unsigned long), UInt);

        new Type("float", sizeof(float), Float);
        new Type("double", sizeof(double), Float);
        s_init = true;
    }

    TypeMap::iterator it = s_typemap.find(name);
    if (it == s_typemap.end()) return 0;

    return it -> second;

    // the type is not known
    // we try to build a Type object from the known types
    
}


Type::Type(const std::string& name, int size, Category category)
    : m_size(size), m_category(category), m_next_type(0)
{
    if (!name.empty())
        setName(name);
}
Type::Type(const std::string& name, const Type* from)
    : m_size(from->m_size), m_category(from->m_category),
    m_next_type(0), m_fields(from->m_fields)
{
    if (!name.empty())
        setName(name);
}
Type::~Type() {}

std::string Type::getName() const { return m_name; }
void Type::setName(const std::string& name) 
{ 
    if (name == m_name) return;
    if (! m_name.empty())
        s_typemap.erase(m_name);
        
    m_name = name;
    s_typemap[name] = this; 
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
    };


    // never reached
    return false;
}

void Type::setSize(int size) { m_size = size; }
int Type::getSize() const { return m_size; }

Type::FieldList Type::getFields() { return m_fields; }
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



std::string Type::toString(const std::string& prefix) const
{
    std::ostringstream output;
    output << prefix << getName();

    if (isSimple()) 
    {
        output << std::endl << prefix << "\t";
        Category cat = getCategory();
        if (isIndirect())
        {
            output << getSize() << " bytes "
                << ((cat == Array) ? " array of " : " pointer on ") << std::endl
                << getNextType() -> toString(prefix + "\t");
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
                << it -> getName() << " " << it -> getType() -> toString(prefix + "\t") << " "
                << std::endl;
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
    else setName("struct " + name);
}
void Struct::fieldsChanged()
{
    FieldList::iterator it = m_fields.begin();

    int offset = 0;
    while (it != m_fields.end())
    {
        const Type* field_type(it -> getType());
        it -> setOffset(offset);
        
        // TODO: take alignment issues into account
        offset += field_type -> getSize();
        ++it;
    }

    setSize(offset);
}


Union::Union(const std::string& name, bool deftype)
    : Type("", 0, Type::Union) 
{ 
    if (deftype) setName(name);
    else setName("union " + name);
}

void Union::fieldsChanged()
{
    int size = 0;
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
{
    setNextType(on);
}



    
Field::Field(const std::string& name, const Type* type)
    : m_name(name), m_type(type), m_offset(0) {}

std::string Field::getName() const { return m_name; }
const Type* Field::getType() const { return m_type; }
int Field::getOffset() const { return m_offset; }
void Field::setOffset(int offset) { m_offset = offset; }

