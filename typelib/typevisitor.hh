#ifndef TYPELIB_VISITOR_HH
#define TYPELIB_VISITOR_HH

#include "typemodel.hh"

namespace Typelib
{
    class UnsupportedType : public TypeException
    {
    public:
        Type const& type;
        std::string const reason;
        UnsupportedType(Type const& type_)
            : TypeException("type " + type_.getName() + " not supported"), type(type_) {}
        UnsupportedType(Type const& type_, std::string const& reason_)
            : TypeException("type " + type_.getName() + " not supported: " + reason_), type(type_), reason(reason_) {}
        ~UnsupportedType() throw() { }
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
    protected:
        bool dispatch(Type const& type);

        virtual bool visit_ (NullType const& type);
        virtual bool visit_ (OpaqueType const& type);
        virtual bool visit_ (Numeric const& type);
        virtual bool visit_ (Enum const& type);

        virtual bool visit_ (Pointer const& type);
        virtual bool visit_ (Array const& type);
        virtual bool visit_ (Container const& type);

        virtual bool visit_ (Compound const& type);
        virtual bool visit_ (Compound const& type, Field const& field);

    public:
        virtual ~TypeVisitor() {}
        void apply(Type const& type);
    };
}

#endif

