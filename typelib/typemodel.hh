#ifndef __TYPELIB_TYPEMODEL_HH__
#define __TYPELIB_TYPEMODEL_HH__

#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <stdint.h>
#include <stdexcept>

namespace Typelib
{
    namespace ValueOps
    {
        // Forward declarations for load/dump support in containers

        struct OutputStream;
        struct InputStream;
    }

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
            Compound,
            Opaque,
            Container
        };
        static const int ValidCategories = Compound + 1;
        
    private:
        std::string m_name;

        size_t      m_size;
        Category    m_category;

	/** Checks that @c identifier is a valid type name */
        static bool isValidIdentifier(const std::string& identifier);

    protected:

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

	/** The set of types this type depends upon
         *
         * This method returns the set of types that are directly depended-upon
         * by this type
         */
	virtual std::set<Type const*> dependsOn() const = 0;

        /** Called by the registry if one (or more) of this type's dependencies
         * is aliased
         *
         * The default implementation does nothing. It is reimplemented in
         * types for which the name is built from the dependencies' name
         */
        virtual void modifiedDependencyAliases(Registry& registry) const;

        bool operator == (Type const& with) const;
        bool operator != (Type const& with) const;

	/** Deep check that \c other defines the same type than 
	 * self. Basic checks on name, size and category are
	 * performed by ==
	 */
	bool isSame(Type const& other) const;

        /** Returns true if \c to can be used to manipulate a value that is
         * described by \c from
         */
        bool canCastTo(Type const& to) const;

        /** Merges this type into \c registry: creates a type equivalent to
         * this one in the target registry, reusing possible equivalent types
         * already present in +registry+.
	 */
	Type const& merge(Registry& registry) const;

        /** Foreign => local mapping for merge() and isSame() */
        typedef std::map<Type const*, Type const*> RecursionStack;

        /** Base merge method. The default implementation should be fine for
         * most types.
         *
         * Call try_merge to check if a type equivalent to *this exists in \c
         * registry, and do_merge to create a copy of *this in \c registry.
         */
        virtual Type const& merge(Registry& registry, RecursionStack& stack) const;

        /** Update this type to reflect a type resize. The default
         * implementation will resize *this if it is listed in \c new_sizes.
         * new_sizes gets updated with the types that are modified.
         *
         * This is not to be called directly. Only use Registry::resize().
         *
         * @return true if this type has been modified, and false otherwise.
         */
        virtual bool resize(Registry& registry, std::map<std::string, std::pair<size_t, size_t> >& new_sizes);

        /** Returns the number of bytes that are unused at the end of the
         * compound */
        virtual unsigned int getTrailingPadding() const;

    protected:
        /** Method that is implemented by type definitions to compare *this with
         * \c other.
         *
         * If \c equality is true, the method must check for strict equality. If
         * it is false, it must check if \c other can be used to manipulate
         * values of type \c *this.
         *
         * For instance, let's consider a compound that has padding bytes, and
         * assume that *this and \c other have different padding (but are the
         * same on every other aspects). do_compare should then return true if
         * equality is false, and false if equality is true.
         *
         * It must use rec_compare to check for indirections (i.e. for
         * recursive calls).
         *
         * The base definition compares the name, size and category of the
         * types.
         */
	virtual bool do_compare(Type const& other, bool equality, std::map<Type const*, Type const*>& stack) const;

        /** Called by do_compare to compare +left+ to +right+. This method takes
         * into account potential loops in the recursion
         *
         * See do_compare for a description of the equality flag.
         */
        bool rec_compare(Type const& left, Type const& right, bool equality, RecursionStack& stack) const;

        /** Checks if there is already a type with the same name and definition
         * than *this in \c registry. If that is the case, returns it, otherwise
         * returns NULL;
         *
         * If there is a type with the same name, but whose definition
         * mismatches, throws DefinitionMismatch
         */
        Type const* try_merge(Registry& registry, RecursionStack& stack) const;

        /** Called by Type::merge when the type does not exist in \c registry
         * already. This method has to create a new type in registry that
         * matches the type definition of *this.
         *
         * All types referenced by *this must be moved to their equivalent in \c
         * registry.
	 */
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const = 0;

        /** Implementation of the actual resizing. Called by resize() */
        virtual bool do_resize(Registry& into, std::map<std::string, std::pair<size_t, size_t> >& new_sizes);
    };

    class NullType : public Type
    {
    public:
        NullType(std::string const& name) : Type(name, 0, Type::NullType ) {}
	virtual std::set<Type const*> dependsOn() const { return std::set<Type const*>(); }

    private:
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const { return new NullType(*this); }
    };

    /** This type is to be used when we want a field to be completely ignored,
     * while being able to handle the rest of the system. Endianness swapping
     * will for instance completely ignore this type.
     */
    class OpaqueType : public Type
    {
    public:
        OpaqueType(std::string const& name, size_t size) : Type(name, size, Type::Opaque) {}
	virtual std::set<Type const*> dependsOn() const { return std::set<Type const*>(); }
	virtual bool do_compare(Type const& other, bool equality, std::map<Type const*, Type const*>& stack) const;
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const { return new OpaqueType(*this); }
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
	virtual bool do_compare(Type const& other, bool equality, RecursionStack& stack) const;
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const;
        NumericCategory m_category;
    };

    /** Enums are defined as name => integer static mappings */
    class Enum : public Type
    {
    public:
        typedef int integral_type;
        typedef std::map<std::string, int> ValueMap;

        class AlreadyExists : public std::runtime_error
        {
        public:
            AlreadyExists(Type const& type, std::string const& name);
        };
        class SymbolNotFound : public std::runtime_error
        {
        public:
            SymbolNotFound(Type const& type, std::string const& name);
        };
        class ValueNotFound : public std::runtime_error
        {
        public:
            ValueNotFound(Type const& type, int value);
        };
        
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
	virtual bool do_compare(Type const& other, bool equality, RecursionStack& stack) const;
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

        /** Returns the number of bytes that are unused at the end of the
         * compound */
        unsigned int getTrailingPadding() const;

	virtual std::set<Type const*> dependsOn() const;

    private:
	virtual bool do_compare(Type const& other, bool equality, RecursionStack& stack) const;
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const;
        bool do_resize(Registry& registry, std::map<std::string, std::pair<size_t, size_t> >& new_sizes);
        FieldList m_fields;
    };

    /** Base class for types with indirection (pointer, array) */
    class Indirect : public Type
    {
    public:
        static const int ValidIDs = Type::Pointer | Type::Array;

    public:

        Indirect(std::string const& name, size_t size, Category category, Type const& on);

        void modifiedDependencyAliases(Registry& registry) const;

        Type const& getIndirection() const;
	virtual std::set<Type const*> dependsOn() const;

        virtual Type const& merge(Registry& registry, RecursionStack& stack) const;

    protected:
	virtual bool do_compare(Type const& other, bool equality, RecursionStack& stack) const;
        virtual bool do_resize(Registry& registry, std::map<std::string, std::pair<size_t, size_t> >& new_sizes);

        /** Overloaded in subclasses to return the name of this type based on
         * the name of the indirection
         *
         * This is solely used by modifiedDependencyAliases() to update the set
         * of aliases for a given type
         */
        virtual std::string getIndirectTypeName(std::string const& inside_name) const = 0;

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

    protected:
        virtual std::string getIndirectTypeName(std::string const& inside_name) const;

    private:
	virtual bool do_compare(Type const& other, bool equality, RecursionStack& stack) const;
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const;
        virtual bool do_resize(Registry& into, std::map<std::string, std::pair<size_t, size_t> >& new_sizes);
        size_t m_dimension;
    };

    /** Pointer types */
    class Pointer : public Indirect
    {
    public:
        Pointer(Type const& on);
        static std::string getPointerName(std::string const& base);

    protected:
        virtual std::string getIndirectTypeName(std::string const& inside_name) const;

    private:
	virtual Type* do_merge(Registry& registry, RecursionStack& stack) const;
    };

    struct UnknownContainer : public std::runtime_error
    {
        UnknownContainer(std::string const& name)
            : std::runtime_error("unknown container " + name) {}
    };

    class ValueVisitor;
    class Value;

    /** Base type for variable-length sets */
    class Container : public Indirect
    {
        std::string m_kind;


    protected:
        struct DeleteIfPredicate
        {
            virtual bool should_delete(Value const& v) = 0;
        };

        virtual void delete_if_impl(void* ptr, DeleteIfPredicate& pred) const = 0;

    private:
        template<typename Pred>
        struct PredicateWrapper : public DeleteIfPredicate
        {
            Pred pred;

            PredicateWrapper(Pred pred)
                : pred(pred) {}

            virtual bool should_delete(Value const& v)
            {
                return pred(v);
            };
        };


    public:
        typedef std::vector<size_t> MarshalOps;

        Container(std::string const& kind, std::string const& name, size_t size, Type const& of);

        std::string kind() const;
        virtual void init(void* ptr) const = 0;
        virtual void destroy(void* ptr) const = 0;
        virtual bool visit(void* ptr, ValueVisitor& visitor) const = 0;

        /** Removes all elements from this container
         */
        virtual void clear(void* ptr) const = 0;

        /** Insert the given element \c v into the container at +ptr+
         */
        virtual void insert(void* ptr, Value v) const = 0;

        /** Removes the element equal to \c in \c ptr.
         *
         * @return true if \c has been found in \c ptr, false otherwise.
         */
        virtual bool erase(void* ptr, Value v) const = 0;

        /** Called to check if +ptr+ and +other+, which are containers of the
         * same type, actually contain the same data
         *
         * @return true if the two containers have the same data, false otherwise
         */
        virtual bool compare(void* ptr, void* other) const = 0;

        /** Called to copy the contents of +src+ into +dst+, which are both
         * containers of the same type.
         */
        virtual void copy(void* dst, void* src) const = 0;

        /** Called to return the natural size of the container, i.e. the size it
         * has on this particular machine. This can be different than getSize()
         * in case of registries generated on other machines.
         */
        virtual long getNaturalSize() const = 0;

        template<typename Pred>
        void delete_if(void* ptr, Pred pred) const
        {
            PredicateWrapper<Pred> predicate(pred);
            delete_if_impl(ptr, predicate);
        }

        virtual size_t getElementCount(void const* ptr) const = 0;

        /** The marshalling process calls this method so that the contents of
         * the container are dumped into the provided buffer.
         *
         * In the marshalled stream, all containers are dumped as
         *   <element-count [64 bits]> <elements>
         *
         * @arg container_ptr the pointer to the container data
         * @arg element_count the count of elements in the container. This is
         *   passed here to avoid costly computations: getElementCount is already
         *   called by the marshalling code itself.
         * @arg stream the stream that will be used to dump the data
         * @arg begin the marshalling code that describes the marshalling process for one element
         * @arg end end of the marshalling code that describes the marshalling process for one element
         * @return the marshalling process should end at the first FLAG_END found in [begin, end)
         *   (with nesting taken into account). The returned value is the iterator on this FLAG_END element
         *   (i.e. *retval == FLAG_END is a postcondition of this method)
         */
        virtual MarshalOps::const_iterator dump(
            void const* container_ptr, size_t element_count,
            ValueOps::OutputStream& stream,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const = 0;

        /** The marshalling process calls this method so that the contents of
         * the container are loaded from the provided buffer.
         *
         * In the marshalled stream, all containers are dumped as
         *   <element-count [64 bits]> <elements>
         *
         * @arg container_ptr the pointer to the container data
         * @arg element_count the count of elements in the container, loaded from the stream.
         * @arg stream the stream from which the data will be read
         * @arg in_offset the offset in \c buffer of the first element of the container
         * @arg begin the marshalling code that describes the loading process for one element
         * @arg end end of the marshalling code that describes the loading process for one element
         * @return the marshalling process should end at the first FLAG_END found in [begin, end)
         *   (with nesting taken into account). The first element of the returned value is the iterator on this FLAG_END element
         *   (i.e. *retval == FLAG_END is a postcondition of this method). The second element is the new value of \c in_offset
         */
        virtual MarshalOps::const_iterator load(
            void* container_ptr, size_t element_count,
            ValueOps::InputStream& stream,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const = 0;

        typedef Container const& (*ContainerFactory)(Registry& r, std::list<Type const*> const& base_type);
        typedef std::map<std::string, ContainerFactory> AvailableContainers;

        static AvailableContainers availableContainers();
        static void registerContainer(std::string const& name, ContainerFactory factory);
        static Container const& createContainer(Registry& r, std::string const& name, Type const& on);
        static Container const& createContainer(Registry& r, std::string const& name, std::list<Type const*> const& on);

    protected:
	virtual bool do_compare(Type const& other, bool equality, RecursionStack& stack) const;
        virtual ContainerFactory getFactory() const = 0;
        Type* do_merge(Registry& registry, RecursionStack& stack) const;

    private:
        static AvailableContainers* s_available_containers;
    };

    struct TypeException : public std::runtime_error
    {
        TypeException(std::string const& msg) : std::runtime_error(msg) { }
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

