#ifndef __TYPELIB_TYPEMODEL_HH__
#define __TYPELIB_TYPEMODEL_HH__

#include <string>
#include <list>
#include <map>
#include <set>

#include "typename.hh"
  
namespace Typelib
{
    class Registry;

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
        typedef std::map<Type const*, Type const*> RecursionStack;

        // Creates a basic type from \c name, \c size and \c category
        Type(const std::string& name, size_t size, Category category);

    public:
        virtual ~Type();

        /** Changes the type name. Never use once the type has been added to a
         * registry */
        void setName    (const std::string& name);
        /** Changes the type size. Don't use that unless you know what you are
         * doing. In particular, don't use it once the type is used in a
         * Compound.
         */
        void setSize    (size_t size);
	/** The type full name (including namespace) */
        std::string   getName() const;
	/** The type name without the namespace */
	std::string   getBasename() const;
	/** The type namespace */
	std::string   getNamespace() const;
	/** Size in bytes of a value */
        size_t        getSize() const;
	/** The type category */
        Category      getCategory() const;
	/** true if this type is null */
        bool          isNull() const;

	/** The set of types this type depends upon */
	virtual std::set<Type const*> dependsOn() const = 0;

        bool operator == (Type const& with) const;
        bool operator != (Type const& with) const;

	/** Deep check that \c other defines the same type than 
	 * self. Basic checks on name, size and category are
	 * performed by ==
	 */
	bool isSame(Type const& other) const;

        /** Merges this type into \c registry: creates a type equivalent to
         * this one in the target registry, reusing possible equivalent types
         * already present in +registry+.
	 */
	Type const& merge(Registry& registry) const;

        virtual Type const& merge(Registry& registry, RecursionStack& stack) const;

    protected:
	virtual bool do_isSame(Type const& other, std::map<Type const*, Type const*>& stack) const;

        bool rec_isSame(Type const& left, Type const& right, RecursionStack& stack) const;


        Type const* try_merge(Registry& registry, RecursionStack& stack) const;
	/** Called by Type::merge when the type does not exist
	 * in \c registry already. This is needed for types
	 * to update their subtypes (pointed-to type, ...) to
	 * the definitions found in \c registry
	 */
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const = 0;
    };

    class NullType : public Type
    {
    public:
        NullType(std::string const& name) : Type(name, 0, Type::NullType ) {}
	virtual std::set<Type const*> dependsOn() const { return std::set<Type const*>(); }

    private:
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const { return new NullType(*this); }
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

	virtual std::set<Type const*> dependsOn() const { return std::set<Type const*>(); }

    private:
	virtual bool do_isSame(Type const& other, RecursionStack& stack) const;
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const;
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
        
        Enum(const std::string& name, Enum::integral_type initial_value = 0);
	/** Add a new definition */
        void            add(std::string const& name, int value);
	/** Gets the value for @c name 
	 * @throws SymbolNotFound if @c name is not defined */
        integral_type   get(std::string const& name) const;
	/** Gets the name for @c value
	 * @throws ValueNotFound if @c value is not defined in this enum */
        std::string     get(integral_type value) const;
        
	/** The list of all names */
        std::list<std::string> names() const;
	/** The name => value map */
        ValueMap const& values() const;

	virtual std::set<Type const*> dependsOn() const { return std::set<Type const*>(); }

        /** Returns the value the next inserted element should have (it is
         * last_inserted_value + 1) */
        Enum::integral_type getNextValue() const;

    private:
	virtual bool do_isSame(Type const& other, RecursionStack& stack) const;
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const;
        integral_type m_last_value;
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

	bool operator == (Field const& field) const;
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

	virtual std::set<Type const*> dependsOn() const;

    private:
	virtual bool do_isSame(Type const& other, RecursionStack& stack) const;
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const;
        FieldList m_fields;
    };

    /** Base class for types with indirection (pointer, array) */
    class Indirect : public Type
    {
    public:
        static const int ValidIDs = Type::Pointer | Type::Array;

    public:

        Indirect(std::string const& name, size_t size, Category category, Type const& on);
        Type const& getIndirection() const;
	virtual std::set<Type const*> dependsOn() const;

        virtual Type const& merge(Registry& registry, RecursionStack& stack) const;

    protected:
	virtual bool do_isSame(Type const& other, RecursionStack& stack) const;

    private:
        Type const& m_indirection;
    };

    /** Unidimensional array types. As in C, the multi-dimensional arrays
     * are defined as arrays-of-arrays
     */
    class Array : public Indirect
    {
    public:

        Array(Type const& of, size_t dimension);
        size_t getDimension() const;
        static std::string getArrayName(std::string const& base, size_t new_dim);

    private:
	virtual bool do_isSame(Type const& other, RecursionStack& stack) const;
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const;
        size_t m_dimension;
    };

    /** Pointer types */
    class Pointer : public Indirect
    {
    public:
        Pointer(Type const& on);
        static std::string getPointerName(std::string const& base);

    private:
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const;
    };







    struct TypeException : public std::logic_error
    {
        TypeException(std::string const& msg) : std::logic_error(msg) { }
    };

    struct BadCategory : public TypeException
    {
        Type::Category const found;
        int            const expected;

        BadCategory(Type::Category found, int expected);
    };

    struct NullTypeFound : public TypeException
    {
        NullTypeFound();
    };
};

#endif

