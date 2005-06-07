#ifndef TYPELIB_DISPLAYVISITOR_HH
#define TYPELIB_DISPLAYVISITOR_HH

#include "typevisitor.hh"
#include <iosfwd>

namespace Typelib
{
    class TypeDisplayVisitor : public TypeVisitor
    {
        template<typename T>
        void display_compound(T const& type, char const* compound_name);

        std::ostream& m_stream;
        std::string   m_indent;

    protected:
        bool visit_(Compound const& type);
        bool visit_(Compound const& type, Field const& field);
        
        bool visit_(Numeric const& type);
        bool visit_(Enum const& type);
       
        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

    public:
        TypeDisplayVisitor(std::ostream& stream, std::string const& base_indent);
    };
}

#endif

