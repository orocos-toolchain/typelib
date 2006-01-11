#include "value.hh"

#include "typevisitor.hh"

namespace Typelib
{
    class ValueVisitor::TypeDispatch : public TypeVisitor
    {
        // The dispatching stack
        std::list<uint8_t*> m_stack;

        // The ValueVisitor object
        ValueVisitor& m_visitor;
    
        template<typename T8, typename T16, typename T32, typename T64>
        bool integer_cast(uint8_t* value, Type const& t)
        {
            switch(t.getSize())
            {
                case 8:  return m_visitor.visit_(*reinterpret_cast<T8*>(value));
                case 16: return m_visitor.visit_(*reinterpret_cast<T16*>(value));
                case 32: return m_visitor.visit_(*reinterpret_cast<T32*>(value));
                case 64: return m_visitor.visit_(*reinterpret_cast<T64*>(value));
                default:
                         throw UnsupportedType(t);
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
                        default:
                            throw UnsupportedType(type);
                    }
            }
            // Never reached
            return true;
        }
        virtual bool visit_ (Enum const& type)
        { 
            throw UnsupportedType(type);
            return true;
        }

        virtual bool visit_ (Pointer const& type)
        {
            m_stack.push_back( *reinterpret_cast<uint8_t**>(m_stack.back()) );
            bool ret = TypeVisitor::visit_(type.getIndirection());
            m_stack.pop_back();
            return ret;
        }
        virtual bool visit_ (Array const& type)
        {
            Type const& array_type(type.getIndirection());
            uint8_t* array_front = *reinterpret_cast<uint8_t**>(m_stack.back());
            for (size_t i = 0; i < type.getDimension(); ++i)
            {
                m_stack.push_back( array_front + array_type.getSize() * i );
                bool ret = TypeVisitor::visit_(array_type);
                m_stack.pop_back();
                if (!ret) return false;
            }

            return true;
        }

        virtual bool visit_ (Compound const& type, Field const& field)
        {
            m_stack.push_back( m_stack.back() + field.getOffset() );
            bool ret = TypeVisitor::visit_(type);
            m_stack.pop_back();
            return ret;
        }

    public:
        TypeDispatch(ValueVisitor& visitor)
            : m_visitor(visitor) { }

        void apply(Value const& value)
        {
            m_stack.clear();
            m_stack.push_back( reinterpret_cast<uint8_t*>(value.getData()));
            TypeVisitor::apply(value.getType());
        }

    };
}

namespace Typelib
{
    void ValueVisitor::apply(Value& v)
    {
        TypeDispatch dispatcher(*this);
        dispatcher.apply(v);
    }
}

