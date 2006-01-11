#ifndef TYPELIB_VALUE_HH
#define TYPELIB_VALUE_HH

#include "typemodel.hh"

namespace Typelib
{
    class Value
    {
        void*       m_data;
        Type const& m_type;
        
    public:
        Value(void* data, Type const& type)
            : m_data(data), m_type(type) {}

        void* getData() const { return m_data; }
        Type const& getType() const { return m_type; }
    };

    class ValueVisitor
    {
        class TypeDispatch;
        friend class TypeDispatch;

    protected:
        virtual bool visit_ (int8_t  & v) { return true; }
        virtual bool visit_ (uint8_t & v) { return true; }
        virtual bool visit_ (int16_t & v) { return true; }
        virtual bool visit_ (uint16_t& v) { return true; }
        virtual bool visit_ (int32_t & v) { return true; }
        virtual bool visit_ (uint32_t& v) { return true; }
        virtual bool visit_ (int64_t & v) { return true; }
        virtual bool visit_ (uint64_t& v) { return true; }
        virtual bool visit_ (float   & v) { return true; }
        virtual bool visit_ (double  & v) { return true; }

        virtual bool visit_pointer  (Value const& v) { return true; }
        virtual bool visit_array    (Value const& v) { return true; }
        virtual bool visit_compound (Value const& v) { return true; }
        virtual bool visit_enum     (Value const& v) { return true; }

    public:
        virtual ~ValueVisitor() {}
        void apply(Value& v);
    };

    class BadValueCast : public std::exception {};

    /** This visitor checks that a given Value object
     * is of the C type T. It either returns a reference to the
     * typed value or throws bad_cast
     */
    template<typename T>
    class CastingVisitor : public ValueVisitor
    {
        T* m_value;

        bool check_type(T& v)
        {
            m_value = &v;
            return false;
        }
        template<typename RealType>
        bool check_type(RealType& v) { throw BadValueCast(); }
         
        /* Do not allow the visitor to go on complex structures (T is supposed to be simple) */
        virtual bool visit_pointer  (Value const& v)
        { throw BadValueCast(); }
        virtual bool visit_array    (Value const& v)
        { throw BadValueCast(); }
        virtual bool visit_compound (Value const& v)
        { throw BadValueCast(); }
        virtual bool visit_enum     (Value const& v)
        { throw BadValueCast(); }

    public:
        CastingVisitor()
            : m_value(0) {};
        T& apply(Value& v)
        {
            ValueVisitor::apply(v);
            return *m_value;
        }
    };
}

#endif

