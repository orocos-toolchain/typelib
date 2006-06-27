#ifndef __TYPELIB_TYPEMODEL_HH__
#define __TYPELIB_TYPEMODEL_HH__

#include <string>
#include <list>
#include <map>

#include "typename.hh"
  
namespace Typelib
{
    /** Base class for all type definitions */
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

	/** Checks that @c identifier is a valid type name */
        static bool isValidIdentifier(const std::string& identifier);

    protected:
        void setName    (const std::string& name);
        void setSize    (size_t size);

        // Creates a basic type from \c name, \c size and \c category
        Type(const std::string& name, size_t size, Category category);

    public:
        virtual ~Type();

	/** The type full name */
        std::string   getName() const;
	/** Size in bytes of a value */
        size_t        getSize() const;
	/** The type category */
        Category      getCategory() const;
	/** true if this type is null */
        bool          isNull() const;

        bool operator == (Type const& with) const;
        bool operator != (Type const& with) const;
    };

    class NullType : public Type
    {
    public:
        NullType(std::string const& name) : Type(name, 0, Type::NullType ) {}
    };

    /** Numeric values (integer, unsigned integer and floating point) */
    class Numeric : public Type
    {
    public:
        enum NumericCategory
        { 
	    SInt = Type::ValidCategories, /// signed integer
	    UInt,			  /// unsigned integer
	    Float			  /// floating point
	};
        static const int ValidCategories = Float + 1;

	/** The category of this numeric type */
        NumericCategory getNumericCategory() const;

    public:
        /** Creates a basic type from \c name, \c size and \c category */
        Numeric(const std::string& name, size_t size, NumericCategory category);

    private:
        NumericCategory m_category;
    };

    /** Enums are defined as name => integer static mappings */
    class Enum : public Type
    {
    public:
        typedef int integral_type;
        typedef std::map<std::string, int> ValueMap;

        class AlreadyExists  {};
        class SymbolNotFound {};
        class ValueNotFound  {};
        
        Enum(const std::string& name);
	/** Add a new definition */
        void            add(std::string const& name, int value);
	/** Gets the value for @name 
	 * @throws SymbolNotFound if @name is not defined */
        integral_type   get(std::string const& name) const;
	/** Gets the name for @value
	 * @throws ValueNotFound if @value is not defined in this enum */
        std::string     get(integral_type value) const;
        
	/** The list of all names */
        std::list<std::string> names() const;
	/** The name => value map */
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

	/** The field name */
        std::string getName() const;
	/** The field type */
        const Type& getType() const;
	/** The offset, in bytes, of this field w.r.t. the 
	 * begginning of the parent value */
        size_t getOffset() const;
    };

    /** Base class for types that are composed of other 
     * types (structures and enums) */
    class Compound : public Type
    {
    public:
        typedef std::list<Field> FieldList;

    public:
        Compound(std::string const& name);

	/** The list of all fields */
        FieldList const&  getFields() const;
	/** Get a field by its name
	 * @return 0 if there is no @name field, or the Field object */
        Field const*      getField(const std::string& name) const;
	/** Add a new field */
        void              addField(const Field& field, size_t offset);
	/** Add a new field */
        void              addField(const std::string& name, const Type& type, size_t offset);

    private:
        FieldList m_fields;
    };



    /** Base class for types with indirection (pointer, array) */
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

    /** Unidimensional array types. As in C, the multi-dimensional arrays
     * are defined as arrays-of-arrays
     */
    class Array : public Indirect
    {
        size_t m_dimension;

    public:
        Array(Type const& of, size_t dimension);
        size_t getDimension() const;
        static std::string getArrayName(std::string const& base, size_t new_dim);
    };

    /** Pointer types */
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

