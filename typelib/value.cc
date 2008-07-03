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
                         throw UnsupportedType(t, "unsupported integer size");
            };
        }

    protected:
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
                    }
            }
	    throw UnsupportedType(type, "unsupported numeric category");
        }

        virtual bool visit_ (Enum const& type)
        { 
            Enum::integral_type& v = *reinterpret_cast<Enum::integral_type*>(m_stack.back());
            return m_visitor.visit_(v, type);
        }

        virtual bool visit_ (Pointer const& type)
        {
            Value v(m_stack.back(), type);
            m_stack.push_back( *reinterpret_cast<uint8_t**>(m_stack.back()) );
            bool ret = m_visitor.visit_(v, type);
            m_stack.pop_back();
            return ret;
        }
        virtual bool visit_ (Array const& type)
        {
            Value v(m_stack.back(), type);
            return m_visitor.visit_(v, type);
        }

        virtual bool visit_ (Compound const& type)
        {
            Value v(m_stack.back(), type);
            return m_visitor.visit_(v, type);
        }

        virtual bool visit_ (OpaqueType const& type)
        {
            Value v(m_stack.back(), type);
            return m_visitor.visit_(v, type);
        }

        virtual bool visit_ (Compound const& type, Field const& field)
        {
            m_stack.push_back( m_stack.back() + field.getOffset() );
            bool ret = m_visitor.visit_(Value(m_stack.back(), field.getType()), type, field);
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

    bool ValueVisitor::visit_(Value const& v, Pointer const& t)
    { return m_dispatcher->TypeVisitor::visit_(t); }
    bool ValueVisitor::visit_(Value const& v, Array const& a) 
    {
	uint8_t*  base = static_cast<uint8_t*>(v.getData());
	m_dispatcher->m_stack.push_back(base);
	uint8_t*& element = m_dispatcher->m_stack.back();

	Type const& array_type(a.getIndirection());
	for (size_t i = 0; i < a.getDimension(); ++i)
	{
	    element = base + array_type.getSize() * i;
	    if (! m_dispatcher->TypeVisitor::visit_(array_type))
		break;
	}

	m_dispatcher->m_stack.pop_back();
	return true;
    }
    bool ValueVisitor::visit_(Value const&, Compound const& c) 
    { return m_dispatcher->TypeVisitor::visit_(c); }
    bool ValueVisitor::visit_(Value const&, Compound const& c, Field const& f) 
    { return m_dispatcher->TypeVisitor::visit_(c, f); }
    bool ValueVisitor::visit_(Enum::integral_type&, Enum const& e) 
    { return m_dispatcher->TypeVisitor::visit_(e); }
    bool ValueVisitor::visit_(Value const& v, OpaqueType const& t)
    { return true; }

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

