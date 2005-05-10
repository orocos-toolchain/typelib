#ifndef TYPELIB_VISITOR_HH
#define TYPELIB_VISITOR_HH

#include "typemodel.hh"

namespace Typelib
{
    class Visitor
    {
        template<typename C>
        bool dispatch_compound(C const& type);
        bool dispatch(Type const& type);

    protected:
        virtual bool visit_ (Numeric const& type);
        virtual bool visit_ (Enum const& type);

        virtual bool visit_ (Pointer const& type);
        virtual bool visit_ (Array const& type);

        virtual bool visit_ (Struct const& type);
        virtual bool visit_ (Struct const& type, Field const& field);
        virtual bool visit_ (Union const& type);
        virtual bool visit_ (Union const& type,  Field const& field);
       
    public:
        void visit(Type const& type);
    };
}

#endif

