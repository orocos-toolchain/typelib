#ifndef __TYPELIB_TYPE_H__
#define __TYPELIB_TYPE_H__

#include <string>
#include <list>
#include <map>

#include "typename.h"
  
namespace Typelib
{
    class Type;

    /** A field in a struct or union */
    class Field
    {
        friend class Struct;
        friend class Union;

        std::string m_name;
        const Type* m_type;
        size_t m_offset;

    protected:
        void setOffset(size_t offset);

    public:
        Field(const std::string& name, const Type* base_type);

        std::string getName() const;
        const Type* getType() const;
        size_t getOffset() const;
    };

    class Type 
    {
    public:

        enum Category
        { 
            NullType = 0,
            Array   = 1, 
            Pointer = 2, 
            SInt    = 4, 
            UInt    = 8, 
            Float   = 16, 
            Struct  = 32, 
            Union   = 64
        };
        static const int ValidTypes = 63;
        
        typedef std::list<Field> FieldList;

    private:
        std::string m_name;

        size_t m_size;
        Category m_category;

        const Type* m_next_type;

        static bool isValidIdentifier(const std::string& identifier);
    protected:
        FieldList m_fields;

        void setName    (const std::string& name);
        void prependName(const std::string& prepend);

        void setSize(size_t size);
        void setNextType(const Type* type);
        virtual void fieldsChanged();

    public:
        // Creates a basic type from \c name, \c size and \c category
        Type(const std::string& name, size_t size, Category category);
        // Creates a type which is the same as \c from, but whose name differs
        Type(const std::string& name, const Type* from);

        virtual ~Type();

        std::string   getName() const;
        bool          isSimple() const;
        size_t        getSize() const;
        Category      getCategory() const;

        const FieldList&  getFields() const;
        const Field*      getField(const std::string& name) const;
        void              addField(const Field& field);
        void              addField(const std::string& name, const Type* type);

        bool          isIndirect() const;
        const Type*   getNextType() const;

        virtual std::string toString(const std::string& prefix = "", bool recursive = false) const;

        bool operator == (const Type& type) const;
        bool operator != (const Type& type) const;
    };

    class Struct : public Type
    {
    protected:
        void fieldsChanged();

    public:
        Struct(const std::string& name, bool deftype = false);
    };

    class Union : public Type
    {
    protected:
        void fieldsChanged();

    public:
        Union(const std::string& name, bool deftype = false);
    };

    class Enum : public Type
    {
    public:
        Enum(const std::string& name);
    };

    class Array : public Type
    {
        int m_dimension;
        std::string m_basename, m_dim_string;

    public:
        Array(const Type* of, int dimension);

        int getDimension() const;
        std::string getBasename() const;
        std::string getDimString() const;
    };

    class Pointer : public Type
    {
    public:
        Pointer(const Type* on);
    };







    class TypeException : public std::exception {};

    class BadCategory : public TypeException
    {
        Type::Category m_found;
        int m_expected;
    public:
        BadCategory(Type::Category found, int expected)
            : m_found(found), m_expected(expected) {}
    };

    class NullTypeFound : public BadCategory
    {
    public:
        NullTypeFound()
            : BadCategory(Type::NullType, Type::ValidTypes) {}
    };
};

#endif

