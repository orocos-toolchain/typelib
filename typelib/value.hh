#ifndef TYPELIB_VALUE_HH
#define TYPELIB_VALUE_HH

#include "typemodel.hh"
#include "typevisitor.hh"
#include "registry.hh"
#include <boost/ref.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <limits>
#include "normalized_numerics.hh"

namespace Typelib
{
    class Value
    {
        void*       m_data;
        boost::reference_wrapper<Type const> m_type;
        
    public:
        Value()
            : m_data(0), m_type(Registry::null()) {}

        Value(void* data, Type const& type)
            : m_data(data), m_type(type) {}

        void* getData() const { return m_data; }
        Type const& getType() const { return m_type; }

        bool operator == (Value const& with) const
        { return (m_data == with.m_data) && (m_type.get() == with.m_type.get()); }
        bool operator != (Value const& with) const
        { return (m_data != with.m_data) || (m_type.get() != with.m_type.get()); }
    };

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

        virtual bool visit_ (Value const& v, Pointer const& t);
        virtual bool visit_ (Value const& v, Array const& a);
        virtual bool visit_ (Value const& v, Compound const& c); 
        virtual bool visit_ (Value const& v, Compound const& c, Field const& f);
        virtual bool visit_ (Enum::integral_type& v, Enum const& e);

    public:
        ValueVisitor(bool defval = false) 
            : m_defval(defval), m_dispatcher(0) {}
        virtual ~ValueVisitor() {}
        void apply(Value v);
    };

    class BadValueCast : public std::exception {};

    /** This visitor checks that a given Value object
     * is of the C type T. It either returns a reference to the
     * typed value or throws BadValueCast
     */
    template<typename T>
    class CastingVisitor : public ValueVisitor
    {
        bool m_found;
        T    m_value;

        bool visit_( typename normalized_numeric_type<T>::type& v)
        {
            m_value = v;
            m_found = true;
            return false;
        }
         
    public:
        CastingVisitor()
            : ValueVisitor(false), m_found(false), m_value() {};
        T& apply(Value v)
        {
            m_found = false;
            ValueVisitor::apply(v);
            if (!m_found)
                throw BadValueCast();

            return m_value;
        }
    };

    /** casts a Value object to a given simple type T */
    template<typename T>
    T& value_cast(Value v)
    {
        CastingVisitor<T> caster;
        return caster.apply(v);
    }

    /** casts a pointer to a given simple type T using \c type as the type for \c *ptr */
    template<typename T>
    T& value_cast(void* ptr, Type const& type)
    { return value_cast<T>(Value(ptr, type)); }

    class FieldNotFound : public BadValueCast 
    {
    public:
        ~FieldNotFound() throw() {}
        std::string const name;
        FieldNotFound(std::string const& name_)
            : name(name_) {}
    };

    /** Gets the object describing a given field */
    class FieldGetter : public ValueVisitor
    {
        std::string m_name;
        Value m_field;

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

    inline Value value_get_field(Value v, std::string const& name)
    {
        FieldGetter getter;
        return getter.apply(v, name);
    }

    inline Value value_get_field(void* ptr, Type const& type, std::string const& name)
    {
        Value v(ptr, type);
        return value_get_field(v, name);
    }
}

#endif

