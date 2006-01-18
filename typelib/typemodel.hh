#ifndef __TYPELIB_TYPEMODEL_HH__
#define __TYPELIB_TYPEMODEL_HH__

#include <string>
#include <list>
#include <map>

#include "typename.hh"
  
namespace Typelib
{
    class Type 
    {
    public:

        enum Category
        { 
            NullType = 0,
            Array  , 
            Pointer, 
            Numeric, 
            Enum   ,
            Compound 
        };
        static const int ValidCategories = Compound + 1;
        
    private:
        std::string m_name;

        size_t      m_size;
        Category    m_category;

        static bool isValidIdentifier(const std::string& identifier);

    protected:
        void setName    (const std::string& name);
        void setSize    (size_t size);

        // Creates a basic type from \c name, \c size and \c category
        Type(const std::string& name, size_t size, Category category);

    public:
        virtual ~Type();

        std::string   getName() const;
        size_t        getSize() const;
        Category      getCategory() const;
        bool          isNull() const;

        bool operator == (Type const& with) const;
        bool operator != (Type const& with) const;
    };

    class NullType : public Type
    {
    public:
        NullType() : Type("nil", 0, Type::NullType ) {}
    };

    class Numeric : public Type
    {
    public:
        enum NumericCategory
        { SInt = Type::ValidCategories, UInt, Float };
        static const int ValidCategories = Float + 1;

        NumericCategory getNumericCategory() const;

    public:
        // Creates a basic type from \c name, \c size and \c category
        Numeric(const std::string& name, size_t size, NumericCategory category);

    private:
        NumericCategory m_category;
    };

    class Enum : public Type
    {
    public:
        typedef int integral_type;
        typedef std::map<std::string, int> ValueMap;

        class AlreadyExists  {};
        class SymbolNotFound {};
        class ValueNotFound  {};
        
        Enum(const std::string& name);
        void            add(std::string const& name, int value);
        integral_type   get(std::string const& name) const;
        std::string     get(integral_type value) const;
        
        std::list<std::string> names() const;
        ValueMap const& values() const;

    private:
        ValueMap m_values;
    };

    /** A field in a Compound type */
    class Field
    {
        friend class Compound;

        std::string m_name;
        const Type& m_type;
        size_t m_offset;

    protected:
        void setOffset(size_t offset);

    public:
        Field(const std::string& name, Type const& base_type);

        std::string getName() const;
        const Type& getType() const;
        size_t getOffset() const;
    };

    /* Compound types (struct, enums) */
    class Compound : public Type
    {
    public:
        typedef std::list<Field> FieldList;

    public:
        Compound(std::string const& name);

        FieldList const&  getFields() const;
        Field const*      getField(const std::string& name) const;
        void              addField(const Field& field, size_t offset);
        void              addField(const std::string& name, const Type& type, size_t offset);

    private:
        FieldList m_fields;
    };



    /** Type with indirection (pointer, array) */
    class Indirect : public Type
    {
    public:
        static const int ValidIDs = Type::Pointer | Type::Array;

    private:
        Type const& m_indirection;

    public:
        Indirect(std::string const& name, size_t size, Category category, Type const& on);
        Type const& getIndirection() const;

    };

    class Array : public Indirect
    {
        size_t m_dimension;

    public:
        Array(Type const& of, size_t dimension);
        size_t getDimension() const;

        static std::string getArrayName(std::string const& base, size_t new_dim);
    };

    class Pointer : public Indirect
    {
    public:
        Pointer(Type const& on);

        static std::string getPointerName(std::string const& base);
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
            : BadCategory(Type::NullType, Type::ValidCategories) {}
    };
};

#endif

