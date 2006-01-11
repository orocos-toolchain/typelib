#ifndef TYPELIB_VISITOR_HH
#define TYPELIB_VISITOR_HH

#include "typemodel.hh"

namespace Typelib
{
    class UnsupportedType : public TypeException  
    {
    public:
        Type const& type;
        UnsupportedType(Type const& type) 
            : type(type) {}
    };
    
    class TypeVisitor
    {
        bool dispatch(Type const& type);

    protected:
        virtual bool visit_ (Numeric const& type);
        virtual bool visit_ (Enum const& type);

        virtual bool visit_ (Pointer const& type);
        virtual bool visit_ (Array const& type);

        virtual bool visit_ (Compound const& type);
        virtual bool visit_ (Compound const& type, Field const& field);
       
    public:
        virtual ~TypeVisitor() {}
        void apply(Type const& type);
    };
}

#endif

