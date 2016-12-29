#include "typedisplay.hh"
#include <string>
#include <iostream>

using namespace Typelib;
using namespace std;

namespace
{
    struct Indent
    {
        std::string& m_indent;
        std::string  m_save;
        Indent(std::string& current)
            : m_indent(current), m_save(current)
        { m_indent += "  "; }
        ~Indent() { m_indent = m_save; }
    };
}

TypeDisplayVisitor::TypeDisplayVisitor(std::ostream& stream, std::string const& base_indent)
    : m_stream(stream), m_indent(base_indent) {}
            
bool TypeDisplayVisitor::visit_(NullType const& type)
{
    m_stream << "null\n";
    return true;
}

bool TypeDisplayVisitor::visit_(OpaqueType const& type)
{
    m_stream << "opaque " << type.getName() << "\n";
    return true;
}

bool TypeDisplayVisitor::visit_(Compound const& type)
{ 
    m_stream << "compound " << type.getName() << " [" << type.getSize() << "] {\n";
    
    { Indent indenter(m_indent);
        TypeVisitor::visit_(type);
    }

    m_stream << m_indent
        << "};";
    return true;
}
bool TypeDisplayVisitor::visit_(Compound const& type, Field const& field)
{ 
    m_stream << m_indent << "(+" << field.getOffset() << ") ";
    TypeVisitor::visit_(type, field);
    m_stream << " " << field.getName() << std::endl;
    return true;
}

bool TypeDisplayVisitor::visit_(Numeric const& type)
{
    char const* name = "";
    switch (type.getNumericCategory())
    {
    case Numeric::SInt:
        name = "sint";
        break;
    case Numeric::UInt:
        name = "uint";
        break;
    case Numeric::Float:
        name = "float";
        break;
    default:
        throw UnsupportedType(type, "unsupported numeric category");
    };

    m_stream
        << name << "(" << type.getSize() << ") (" << type.getName() << ")";
    return true;
}

bool TypeDisplayVisitor::visit_(Enum const& type)
{
    m_stream << "enum " << type.getName();
    Enum::ValueMap::const_iterator it;

    for (it = type.values().begin(); it != type.values().end(); ++it)
	m_stream << "\n    " << it->first << " -> " << it->second;

    return true;
}

bool TypeDisplayVisitor::visit_(Pointer const& type)
{
    m_stream << "pointer on " << type.getIndirection().getName() << "\n";
    return true;
}

bool TypeDisplayVisitor::visit_(Array const& type)
{
    m_stream << "array[" << type.getDimension() << "] of\n";
    { Indent indenter(m_indent);
        m_stream << m_indent;
        TypeVisitor::visit_(type);
    }
    return true;
}

bool TypeDisplayVisitor::visit_(Container const& type)
{
    m_stream << "container " << type.kind() << "<" << type.getIndirection().getName() << ">";
    return true;
}
