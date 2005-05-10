#include "visitor.hh"

namespace Typelib
{
    bool Visitor::visit_  (Numeric const& type)
    { return true; }
    bool Visitor::visit_  (Enum const& type)
    { return true; }

    bool Visitor::visit_  (Pointer const& type)
    { return dispatch(type.getIndirection()); }
    bool Visitor::visit_  (Array const& type)
    { return dispatch(type.getIndirection()); }

    template<typename C>
    bool Visitor::dispatch_compound( C const& type )
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
    bool Visitor::visit_  (Struct const& type)                 
    { return dispatch_compound(type); }
    bool Visitor::visit_  (Union const& type)                  
    { return dispatch_compound(type); }
    bool Visitor::visit_  (Struct const& type, Field const& field)   
    { return dispatch(field.getType()); }
    bool Visitor::visit_  (Union const& type,  Field const& field)
    { return dispatch(field.getType()); }

    bool Visitor::dispatch(Type const& type)
    {
        switch(type.getCategory())
        {
            case Type::NullType:
                throw NullTypeFound();
            case Type::Numeric:
                return visit_( dynamic_cast<Numeric const&>(type) );
            case Type::Enum:
                return visit_( dynamic_cast<Enum const&>(type) );
            case Type::Array:
                return visit_( dynamic_cast<Array const&>(type) );
            case Type::Pointer:
                return visit_( dynamic_cast<Pointer const&>(type) );
            case Type::Struct:
                return visit_( dynamic_cast<Struct const&>(type) );
            case Type::Union:
                return visit_( dynamic_cast<Union const&>(type) );
        }
        // Never reached
        assert(false);
    }

    void Visitor::visit(Type const& type)
    { dispatch(type); }
}

