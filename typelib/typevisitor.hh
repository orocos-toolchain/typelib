#ifndef TYPELIB_VISITOR_HH
#define TYPELIB_VISITOR_HH

#include "typemodel.hh"

namespace Typelib
{
    class UnsupportedType : public TypeException  
    {
    public:
        Type const& type;
        UnsupportedType(Type const& type_) 
            : type(type_) {}
    };
    
    /** Base class for type visitors 
     * Given a Type object, TypeVisitor::apply dispatches the
     * casted type to the appropriate visit_ method
     *
     * The default visit_ methods either do nothing or visits the
     * types recursively (for arrays, pointers and compound types) 
     */
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

