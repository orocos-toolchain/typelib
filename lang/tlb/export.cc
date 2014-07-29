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
        string    m_source_id;
        string emitSourceID() const;
        string emitMetaData(Type const& type) const;
        string emitMetaData(MetaData const& metadata) const;

    protected:
        bool visit_(Compound const& type);
        bool visit_(Compound const& type, Field const& field);

        bool visit_(Numeric const& type);

        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

        bool visit_(Enum const& type);

	bool visit_(NullType const& type);
        bool visit_(OpaqueType const& type);
        bool visit_(Container const& type);

    public:
        TlbExportVisitor(ostream& stream, string const& base_indent, std::string const& source_id);
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

    TlbExportVisitor::TlbExportVisitor(ostream& stream, string const& base_indent, std::string const& source_id)
        : m_stream(stream), m_indent(base_indent), m_source_id(source_id) {}
                
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

    std::string TlbExportVisitor::emitMetaData(Type const& type) const
    {
        return emitMetaData(type.getMetaData());
    }
    std::string TlbExportVisitor::emitMetaData(MetaData const& metadata) const
    {
        std::ostringstream stream;
        MetaData::Map const& map = metadata.get();
        for (MetaData::Map::const_iterator it = map.begin(); it != map.end(); ++it)
        {
            std::string key = it->first;
            MetaData::Values values = it->second;
            for (MetaData::Values::const_iterator it_value = values.begin(); it_value != values.end(); ++it_value)
                stream << "<metadata key=\"" << key << "\"><![CDATA[" << *it_value << "]]></metadata>\n";
        }
        return stream.str();
    }

    std::string TlbExportVisitor::emitSourceID() const
    {
        if (!m_source_id.empty())
            return "source_id=\"" + xmlEscape(m_source_id) + "\"";
        else return std::string();
    }

    bool TlbExportVisitor::visit_(OpaqueType const& type)
    {
        m_stream << "<opaque name=\"" << xmlEscape(type.getName()) << "\" size=\"" << type.getSize() << "\" " << emitSourceID() << ">\n";
        m_stream << m_indent << emitMetaData(type) << "\n";
        m_stream << m_indent << "</opaque>";
        return true;
    }

    bool TlbExportVisitor::visit_(Compound const& type)
    { 
        m_stream << "<compound name=\"" << xmlEscape(type.getName()) << "\" size=\"" << type.getSize() << "\" " << emitSourceID() << ">\n";
        
        { Indent indenter(m_indent);
            TypeVisitor::visit_(type);
        }

        m_stream << m_indent << emitMetaData(type) << "\n";
        m_stream << m_indent << "</compound>";

        return true;
    }
    bool TlbExportVisitor::visit_(Compound const& type, Field const& field)
    { 
        m_stream 
            << m_indent
            << "<field name=\"" << field.getName() << "\""
            << " type=\""   << xmlEscape(field.getType().getName())  << "\""
            << " offset=\"" << field.getOffset() << "\">\n";
        m_stream << m_indent << emitMetaData(field.getMetaData()) << "\n";
        m_stream << m_indent << "</field>\n";
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
            << "size=\"" << type.getSize() << "\" " << emitSourceID() << ">";
        m_stream << m_indent << emitMetaData(type) << "\n";
        m_stream << m_indent << "</numeric>";

        return true;
    }
    bool TlbExportVisitor::visit_ (NullType const& type)
    {
        m_stream << "<null " << " name=\"" << type.getName() << "\" " << emitSourceID() << ">";
        m_stream << m_indent << emitMetaData(type) << "\n";
        m_stream << m_indent << "</null>";
	return true;
    }

    void indirect(ostream& stream, Indirect const& type)
    {
        stream 
            << " name=\"" << xmlEscape(type.getName())
            << "\" of=\"" << xmlEscape(type.getIndirection().getName()) << "\"";
    }

    bool TlbExportVisitor::visit_ (Pointer const& type)
    {
        m_stream << "<pointer ";
        indirect(m_stream, type);
        m_stream << " " << emitSourceID() << ">\n";
        m_stream << m_indent << emitMetaData(type) << "\n";
        m_stream << m_indent << "</pointer>";
        return true;
    }
    bool TlbExportVisitor::visit_ (Array const& type)
    {
        m_stream << "<array ";
        indirect(m_stream, type);
        m_stream << " dimension=\"" << type.getDimension() << "\" " << emitSourceID() << ">\n";
        m_stream << m_indent << emitMetaData(type) << "\n";
        m_stream << m_indent << "</array>";
        return true;
    }
    bool TlbExportVisitor::visit_(Container const& type)
    { 
        m_stream << "<container ";
        indirect(m_stream, type);
        m_stream
            << " size=\"" << type.getSize() << "\""
            << " kind=\"" << xmlEscape(type.kind()) << "\""
            << " " << emitSourceID() << ">\n";
        m_stream << m_indent << emitMetaData(type) << "\n";
        m_stream << m_indent << "</container>";
        return true;
    }

    bool TlbExportVisitor::visit_ (Enum const& type)
    {
        Enum::ValueMap const& values = type.values();
        m_stream << "<enum name=\"" << type.getName() << "\" " << emitSourceID() << ">\n";
        { Indent indenter(m_indent);
            Enum::ValueMap::const_iterator it;
            for (it = values.begin(); it != values.end(); ++it)
                m_stream << m_indent << "<value symbol=\"" << it->first << "\" value=\"" << it->second << "\"/>\n";
        }
        m_stream << m_indent << emitMetaData(type) << "\n";
        m_stream << m_indent << "</enum>";

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

bool TlbExport::save
    ( ostream& stream
    , Typelib::RegistryIterator const& type )
{
    if (type.isAlias())
    {
        stream << "  <alias "
            "name=\"" << xmlEscape(type.getName()) << "\" "
            "source=\"" << xmlEscape(type->getName()) << "\"/>\n";
    }
    else
    {
        stream << "  ";
        TlbExportVisitor exporter(stream, "  ", type.getSource());
        exporter.apply(*type);
        stream << "\n";
    }
    return true;
}

