#ifndef TYPELIB_VALUE_HH
#define TYPELIB_VALUE_HH

#include "typemodel.hh"
#include "typevisitor.hh"
#include "registry.hh"
#include <boost/ref.hpp>

namespace Typelib
{
    /** A Value object is raw data given through a void* pointer typed
     * by a Type object
     *
     * There are two basic operations on value object:
     * <ul>
     *   <li> Typelib::value_cast typesafe cast from Value to a C type
     *   <li> Typelib::value_get_field     gets a Value object for a field in a Compound
     * </ul>
     */
    class Value
    {
        void *m_data;
        size_t  m_dataSize;
        boost::reference_wrapper<Type const> m_type;
        
    public:
        Value()
            : m_data(0), m_dataSize(0), m_type(Registry::null()) {}

        Value(void* data, Type const& type)
            : m_data(data), m_dataSize(0), m_type(type) {}

        Value(void* data, size_t dataSize, Type const& type)
            : m_data(data), m_dataSize(dataSize), m_type(type) 
            {
                if(m_dataSize != type.getSize())
                    throw std::runtime_error("Error, given data pointer has different size than expected");
            }

	/** The raw data pointer */
        void* getData() const { return m_data; }
	/** The data type */
        Type const& getType() const { return m_type; }

        bool operator == (Value const& with) const
        { return (m_data == with.m_data) && (m_type.get() == with.m_type.get()); }
        bool operator != (Value const& with) const
        { return (m_data != with.m_data) || (m_type.get() != with.m_type.get()); }
    };

    /** A visitor object that can be used to discover the type
     * of a Value object */
    class ValueVisitor
    {
        class TypeDispatch;
        friend class TypeDispatch;
        bool m_defval;

        TypeDispatch* m_dispatcher;

    protected:
        virtual bool visit_ (int8_t  &) { return m_defval; }
        virtual bool visit_ (uint8_t &) { return m_defval; }
        virtual bool visit_ (int16_t &) { return m_defval; }
        virtual bool visit_ (uint16_t&) { return m_defval; }
        virtual bool visit_ (int32_t &) { return m_defval; }
        virtual bool visit_ (uint32_t&) { return m_defval; }
        virtual bool visit_ (int64_t &) { return m_defval; }
        virtual bool visit_ (uint64_t&) { return m_defval; }
        virtual bool visit_ (float   &) { return m_defval; }
        virtual bool visit_ (double  &) { return m_defval; }

        virtual bool visit_ (Value const& v, OpaqueType const& t);
        virtual bool visit_ (Value const& v, Pointer const& t);
        virtual bool visit_ (Value const& v, Array const& a);
        virtual bool visit_ (Value const& v, Container const& a);
        virtual bool visit_ (Value const& v, Compound const& c); 
        virtual bool visit_ (Value const& v, Compound const& c, Field const& f);
        virtual bool visit_ (Enum::integral_type& v, Enum const& e);

    public:
        ValueVisitor(bool defval = false);
        virtual ~ValueVisitor();

        /** This is for internal use only. To visit a Value object, use apply */
        virtual void dispatch(Value v);

        void apply(Value v);
    };

    /** Raised by value_cast when the type of a Value object
     * is not compatible with the wanted C type */
    class BadValueCast : public std::exception {};

    /** Exception raised if a non existent field is required */
    class FieldNotFound : public BadValueCast 
    {
    public:
        ~FieldNotFound() throw() {}
        std::string const name;
        FieldNotFound(std::string const& name_)
            : name(name_) {}
    };

    /** Gets the object describing a given field 
     * Throws FieldNotFound if the field is not a field of the base type */
    class FieldGetter : public ValueVisitor
    {
        std::string m_name;
        Value m_field;

        using ValueVisitor::visit_;

        bool visit_(Compound const& type) { return true; }
        bool visit_(Value const& value, Compound const&, Field const& field)
        {
            if (field.getName() == m_name)
            {
                m_field = value;
                return false;
            }
            return true;
        }
        
    public:
        FieldGetter()
            : ValueVisitor(true) {}
        Value apply(Value v, std::string const& name)
        {
            m_name = name;
            m_field = Value();
            ValueVisitor::apply(v);
            if (! m_field.getData())
                throw FieldNotFound(name);
            return m_field;
        }
        
    };

    /** Get the Value object for a named field in @c v 
     * @throws FieldNotFound if @name is not a field of the base type */
    inline Value value_get_field(Value v, std::string const& name)
    {
        FieldGetter getter;
        return getter.apply(v, name);
    }

    /** Get the Value object for a named field in @c v 
     * @throws FieldNotFound if @name is not a field of the base type */
    inline Value value_get_field(void* ptr, Type const& type, std::string const& name)
    {
        Value v(ptr, type);
        return value_get_field(v, name);
    }
}

#endif

