#include "typevisitor.hh"

namespace Typelib
{
    bool visit_ ( NullType const& type )
    { throw NullTypeFound(); }
    bool TypeVisitor::visit_  (Numeric const& type)
    { return true; }
    bool TypeVisitor::visit_  (Enum const& type)
    { return true; }

    bool TypeVisitor::visit_  (Pointer const& type)
    { return dispatch(type.getIndirection()); }
    bool TypeVisitor::visit_  (Array const& type)
    { return dispatch(type.getIndirection()); }

    bool TypeVisitor::visit_  (Compound const& type)                 
    {  
        typedef Compound::FieldList Fields;
        Fields const& fields(type.getFields());
        Fields::const_iterator const end = fields.end();
        
        for (Fields::const_iterator it = fields.begin(); it != end; ++it)
        {
            if (! visit_(type, *it))
                return false;
        }
        return true;
    }
    bool TypeVisitor::visit_  (Compound const& type, Field const& field)   
    { return dispatch(field.getType()); }

    bool TypeVisitor::dispatch(Type const& type)
    {
        switch(type.getCategory())
        {
            case Type::NullType:
                return visit_ ( dynamic_cast<NullType const&>(type) );
            case Type::Numeric:
                return visit_( dynamic_cast<Numeric const&>(type) );
            case Type::Enum:
                return visit_( dynamic_cast<Enum const&>(type) );
            case Type::Array:
                return visit_( dynamic_cast<Array const&>(type) );
            case Type::Pointer:
                return visit_( dynamic_cast<Pointer const&>(type) );
            case Type::Compound:
                return visit_( dynamic_cast<Compound const&>(type) );
        }
        // Never reached
        assert(false);
	return false; // to shut up gcc
    }

    void TypeVisitor::apply(Type const& type)
    { dispatch(type); }
}

