#include "value.hh"

#include "typevisitor.hh"

namespace Typelib
{
    class ValueVisitor::TypeDispatch : public TypeVisitor
    {
        friend class ValueVisitor;

        // The dispatching stack
        std::list<uint8_t*> m_stack;

        // The ValueVisitor object
        ValueVisitor& m_visitor;
    
        template<typename T8, typename T16, typename T32, typename T64>
        bool integer_cast(uint8_t* value, Type const& t)
        {
            switch(t.getSize())
            {
                case 1: return m_visitor.visit_(*reinterpret_cast<T8*>(value));
                case 2: return m_visitor.visit_(*reinterpret_cast<T16*>(value));
                case 4: return m_visitor.visit_(*reinterpret_cast<T32*>(value));
                case 8: return m_visitor.visit_(*reinterpret_cast<T64*>(value));
                default:
                         throw UnsupportedType(t);
            };
        }

    protected:
        bool do_visit (Enum const& type) { return TypeVisitor::visit_(type); }
        bool do_visit (Pointer const& type) { return TypeVisitor::visit_(type); }
        bool do_visit (Array const& type) { return TypeVisitor::visit_(type); }
        bool do_visit (Compound const& type) { return TypeVisitor::visit_(type); }
        bool do_visit (Compound const& type, Field const& field) { return TypeVisitor::visit_(type); }


        virtual bool visit_ (Numeric const& type)
        { 
            uint8_t* value(m_stack.back());
            switch(type.getNumericCategory())
            {
                case Numeric::SInt:
                    return integer_cast<int8_t, int16_t, int32_t, int64_t>(value, type);
                case Numeric::UInt:
                    return integer_cast<uint8_t, uint16_t, uint32_t, uint64_t>(value, type);
                case Numeric::Float:
                    switch(type.getSize())
                    {
                        case sizeof(float):  return m_visitor.visit_(*reinterpret_cast<float*>(value));
                        case sizeof(double): return m_visitor.visit_(*reinterpret_cast<double*>(value));
                        default:
                            throw UnsupportedType(type);
                    }
            }
            // Never reached
            assert(false);
            return true;
        }

        virtual bool visit_ (Enum const& type)
        { 
            throw UnsupportedType(type);
            return true;
        }

        virtual bool visit_ (Pointer const& type)
        {
            Value v(m_stack.back(), type);
            m_stack.push_back( *reinterpret_cast<uint8_t**>(m_stack.back()) );
            bool ret = m_visitor.visit_pointer(v, type);
            m_stack.pop_back();
            return ret;
        }
        virtual bool visit_ (Array const& type)
        {
            throw UnsupportedType(type);
            return true;
        }

        virtual bool visit_ (Compound const& type)
        {
            Value v(m_stack.back(), type);
            return m_visitor.visit_compound(v, type);
        }

        virtual bool visit_ (Compound const& type, Field const& field)
        {
            m_stack.push_back( m_stack.back() + field.getOffset() );
            bool ret = m_visitor.visit_field(Value(m_stack.back(), field.getType()), type, field);
            m_stack.pop_back();
            return ret;
        }

    public:
        TypeDispatch(ValueVisitor& visitor)
            : m_visitor(visitor) { }

        void apply(Value value)
        {
            m_stack.clear();
            m_stack.push_back( reinterpret_cast<uint8_t*>(value.getData()));
            TypeVisitor::apply(value.getType());
        }

    };

    bool ValueVisitor::visit_pointer  (Value const& v, Pointer const& t)
    { return m_dispatcher->TypeVisitor::visit_(t); }
    bool ValueVisitor::visit_array    (Value const& v, Array const& a) 
    { return m_dispatcher->TypeVisitor::visit_(a); }
    bool ValueVisitor::visit_compound (Value const&, Compound const& c) 
    { return m_dispatcher->TypeVisitor::visit_(c); }
    bool ValueVisitor::visit_field    (Value const&, Compound const& c, Field const& f) 
    { return m_dispatcher->TypeVisitor::visit_(c, f); }
    bool ValueVisitor::visit_enum     (Value const&, Enum const& e) 
    { return m_dispatcher->TypeVisitor::visit_(e); }

}

namespace Typelib
{
    void ValueVisitor::apply(Value v)
    {
        TypeDispatch dispatcher(*this);
        m_dispatcher = &dispatcher;
        dispatcher.apply(v);
        m_dispatcher = 0;
    }
}

