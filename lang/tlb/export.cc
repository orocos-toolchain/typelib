#include "export.hh"
#include <iostream>

#include <typelib/typevisitor.hh>

namespace
{
    using namespace std;
    using namespace Typelib;
    class TlbExportVisitor : public TypeVisitor
    {
        ostream&  m_stream;
        string    m_indent;

    protected:
        bool visit_(Compound const& type);
        bool visit_(Compound const& type, Field const& field);

        bool visit_(Numeric const& type);

        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

        bool visit_(Enum const& type);

	bool visit_(NullType const& type);
        bool visit_(OpaqueType const& type);

    public:
        TlbExportVisitor(ostream& stream, string const& base_indent);
    };

    struct Indent
    {
        string& m_indent;
        string  m_save;
        Indent(string& current)
            : m_indent(current), m_save(current)
        { m_indent += "  "; }
        ~Indent() { m_indent = m_save; }
    };

    TlbExportVisitor::TlbExportVisitor(ostream& stream, string const& base_indent)
        : m_stream(stream), m_indent(base_indent) {}
                
    std::string xmlEscape(std::string const& source)
    {
        string result = source;
        size_t pos = result.find_first_of("<>");
        while (pos != string::npos)
        {
            if (result[pos] == '<')
                result.replace(pos, 1, "&lt;");
            else if (result[pos] == '>')
                result.replace(pos, 1, "&gt;");
            pos = result.find_first_of("<>");
        }

        return result;
    }

    bool TlbExportVisitor::visit_(OpaqueType const& type)
    {
        m_stream << "<opaque name=\"" << xmlEscape(type.getName()) << "\" size=\"" << type.getSize() << "\" />\n";
        return true;
    }

    bool TlbExportVisitor::visit_(Compound const& type)
    { 
        m_stream << "<compound name=\"" << xmlEscape(type.getName()) << "\" size=\"" << type.getSize() << "\">\n";
        
        { Indent indenter(m_indent);
            TypeVisitor::visit_(type);
        }

        m_stream << m_indent
            << "</compound>";

        return true;
    }
    bool TlbExportVisitor::visit_(Compound const& type, Field const& field)
    { 
        m_stream 
            << m_indent
            << "<field name=\"" << field.getName() << "\""
            << " type=\""   << xmlEscape(field.getType().getName())  << "\""
            << " offset=\"" << field.getOffset() << "\"/>\n";
        return true;
    }

    namespace
    {
        char const* getStringCategory(Numeric::NumericCategory category)
        {
            switch(category)
            {
                case Numeric::SInt:
                    return "sint";
                case Numeric::UInt:
                    return "uint";
                case Numeric::Float:
                    return "float";
            }
            // never reached
            return 0;
        }
    }

    bool TlbExportVisitor::visit_(Numeric const& type)
    {
        m_stream 
            << "<numeric name=\"" << type.getName() << "\" " 
            << "category=\"" << getStringCategory(type.getNumericCategory()) << "\" "
            << "size=\"" << type.getSize() << "\"/>";
        return true;
    }
    bool TlbExportVisitor::visit_ (NullType const& type)
    {
        m_stream << "<null " << " name=\"" << type.getName() << "\"/>";
	return true;
    }

    void indirect(ostream& stream, Indirect const& type)
    {
        stream 
            << " name=\"" << type.getName() 
            << "\" of=\"" << type.getIndirection().getName() << "\"";
    }

    bool TlbExportVisitor::visit_ (Pointer const& type)
    {
        m_stream << "<pointer ";
        indirect(m_stream, type);
        m_stream << "/>";
        return true;
    }
    bool TlbExportVisitor::visit_ (Array const& type)
    {
        m_stream << "<array ";
        indirect(m_stream, type);
        m_stream << " dimension=\"" << type.getDimension() << "\"/>";
        return true;
    }
    bool TlbExportVisitor::visit_ (Enum const& type)
    {
        Enum::ValueMap const& values = type.values();
        if (values.empty())
            m_stream << "<enum name=\"" << type.getName() << "\"/>";
        else
        {
            m_stream << "<enum name=\"" << type.getName() << "\">\n";
            { Indent indenter(m_indent);
                Enum::ValueMap::const_iterator it, end = values.end();
                for (it = values.begin(); it != values.end(); ++it)
                    m_stream << m_indent << "<value symbol=\"" << it->first << "\" value=\"" << it->second << "\"/>\n";
            }
            m_stream << m_indent << "</enum>";
        }

        return true;
    }

}

using namespace std;
void TlbExport::begin
    ( ostream& stream
    , Typelib::Registry const& /*registry*/ )
{
    stream << 
        "<?xml version=\"1.0\"?>\n"
        "<typelib>\n";
}
void TlbExport::end
    ( ostream& stream
    , Typelib::Registry const& /*registry*/ )
{
    stream << 
        "</typelib>\n";
}

void TlbExport::save
    ( ostream& stream
    , Typelib::RegistryIterator const& type )
{
    if (type.isAlias())
    {
        stream << "  <alias "
            "name=\"" << type.getName() << "\" "
            "source=\"" << type->getName() << "\"/>\n";
    }
    else
    {
        stream << "  ";
        TlbExportVisitor exporter(stream, "  ");
        exporter.apply(*type);
        stream << "\n";
    }
}

