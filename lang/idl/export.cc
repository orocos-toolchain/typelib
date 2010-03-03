#include "export.hh"
#include <iostream>
#include <utilmm/stringtools.hh>

#include <typelib/typevisitor.hh>
#include <typelib/ioplugins.hh>


namespace
{
    using namespace std;
    using namespace Typelib;

    class IDLTypeIdentifierVisitor : public TypeVisitor
    {
    public:
	IDLExport const& m_exporter;
        string    m_front, m_back;
        bool      m_opaque_as_any;

        bool visit_(OpaqueType const& type);
        bool visit_(NullType const& type);
        bool visit_(Container const& type);
        bool visit_(Compound const& type);

        bool visit_(Numeric const& type);

        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

        bool visit_(Enum const& type);

        IDLTypeIdentifierVisitor(IDLExport const& exporter, bool opaque_as_any)
            : m_exporter(exporter), m_opaque_as_any(opaque_as_any) {}
    };
    static string idl_type_identifier(Type const& type, IDLExport const& exporter, bool opaque_as_any, std::string const& field_name = std::string())
    {
        IDLTypeIdentifierVisitor visitor(exporter, opaque_as_any);
        visitor.apply(type);
        if (field_name.empty())
            return visitor.m_front + visitor.m_back;
        else
            return visitor.m_front + " " + field_name + visitor.m_back;
    }

    bool IDLTypeIdentifierVisitor::visit_(OpaqueType const& type)
    {
        if (m_opaque_as_any)
            m_front = "any";
        else
            throw UnsupportedType(type, "opaque types are not allowed in IDL");

        return true;
    }
    bool IDLTypeIdentifierVisitor::visit_(NullType const& type)
    { throw UnsupportedType(type, "null types are not supported for export in IDL"); }
    bool IDLTypeIdentifierVisitor::visit_(Container const& type)
    {
        if (type.getName() == "/std/string")
            m_front = "string";
        else
            m_front = "sequence<" + idl_type_identifier(type.getIndirection(), m_exporter, m_opaque_as_any) + ">";

        return true;
    }
    bool IDLTypeIdentifierVisitor::visit_(Compound const& type)
    { m_front = m_exporter.getIDLAbsoluteTypename(type);
        return true; }
    bool IDLTypeIdentifierVisitor::visit_(Numeric const& type)
    { m_front = m_exporter.getIDLAbsoluteTypename(type);
        return true; }
    bool IDLTypeIdentifierVisitor::visit_(Pointer const& type)
    { throw UnsupportedType(type, "pointer types are not supported for export in IDL"); }
    bool IDLTypeIdentifierVisitor::visit_(Array const& type)
    {
        m_front = idl_type_identifier(type.getIndirection(), m_exporter, m_opaque_as_any);
        m_back = "[" + boost::lexical_cast<string>(type.getDimension()) + "]";
        return true;
    }
    bool IDLTypeIdentifierVisitor::visit_(Enum const& type)
    { m_front = m_exporter.getIDLAbsoluteTypename(type);
        return true; }

    class IDLExportVisitor : public TypeVisitor
    {
    public:
	IDLExport const& m_exporter;
        ostream&  m_stream;
        string    m_indent;
        bool      m_opaque_as_any;

        bool visit_(OpaqueType const& type);
        bool visit_(Container const& type);
        bool visit_(Compound const& type);
        bool visit_(Compound const& type, Field const& field);

        bool visit_(Numeric const& type);

        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

        bool visit_(Enum const& type);

        IDLExportVisitor(Registry const& registry, IDLExport const& exporter, ostream& stream, string const& base_indent, bool opaque_as_any);
    };

    struct Indent
    {
        string& m_indent;
        string  m_save;
        Indent(string& current)
            : m_indent(current), m_save(current)
        { m_indent += "    "; }
        ~Indent() { m_indent = m_save; }
    };

    IDLExportVisitor::IDLExportVisitor(Registry const& registry, IDLExport const& exporter,
	    ostream& stream, string const& base_indent, bool opaque_as_any)
        : m_exporter(exporter), m_stream(stream), m_indent(base_indent)
        , m_opaque_as_any(opaque_as_any) {}

    bool IDLExportVisitor::visit_(Compound const& type)
    { 
        m_stream << m_indent << "struct " << IDLExport::getIDLTypename(type) << " {\n";
        
        { Indent indenter(m_indent);
            TypeVisitor::visit_(type);
        }

        m_stream << m_indent
            << "};\n";

        return true;
    }
    bool IDLExportVisitor::visit_(Compound const& type, Field const& field)
    { 
        Type const& field_type(field.getType());
        m_stream
            << m_indent
            << idl_type_identifier(field.getType(), m_exporter, m_opaque_as_any, field.getName())
            << ";\n";

        return true;
    }

    bool IDLExportVisitor::visit_(Numeric const& type)
    {
	// no need to export Numeric types, they are already defined by IDL
	// itself
	return true;
    }

    bool IDLExportVisitor::visit_ (Pointer const& type)
    {
	throw UnsupportedType(type, "pointers are not allowed in IDL");
        return true;
    }
    bool IDLExportVisitor::visit_ (Array const& type)
    {
	throw UnsupportedType(type, "top-level arrays are not handled by the IDLExportVisitor");
        return true;
    }

    bool IDLExportVisitor::visit_ (Enum const& type)
    {
	m_stream << m_indent << "enum " << m_exporter.getIDLTypename(type) << " { ";

	utilmm::stringlist symbols;
        Enum::ValueMap const& values = type.values();
	Enum::ValueMap::const_iterator it, end = values.end();
	for (it = values.begin(); it != values.end(); ++it)
	    symbols.push_back(it->first);
	m_stream << utilmm::join(symbols, ", ") << " };\n";

        return true;
    }

    bool IDLExportVisitor::visit_(Container const& type)
    {
        // Simply ignore the type, it is uneeded to export it. Containers in structures
        // are already handled in visit_(Compound, Field)
        return true;
    }
    bool IDLExportVisitor::visit_(OpaqueType const& type)
    {
        if (m_opaque_as_any)
            return true;

        throw UnsupportedType(type, "opaque types are not supported for export in IDL");
    }
}

using namespace std;
IDLExport::IDLExport() 
    : m_namespace("/"), m_opaque_as_any(false) {}

void IDLExport::end
    ( ostream& stream
    , Typelib::Registry const& /*registry*/ )
{
    generateTypedefs(stream);

    // Close the remaining namespaces
    utilmm::stringlist
	ns_levels = utilmm::split(m_namespace, "/");
    closeNamespaces(stream, ns_levels.size());
}

void IDLExport::closeNamespaces(ostream& stream, int levels)
{
    for (int i = 0; i < levels; ++i)
    {
	m_indent = std::string(m_indent, 0, m_indent.size() - 4);
	stream << "\n" << m_indent << "};\n";
    }
}

void IDLExport::checkType(Type const& type)
{
    if (type.getCategory() == Type::Pointer)
	throw UnsupportedType(type, "pointers are not allowed in IDL");
    if (type.getCategory() == Type::Array)
    {
	Type::Category pointed_to = static_cast<Indirect const&>(type).getIndirection().getCategory();
	if (pointed_to == Type::Array || pointed_to == Type::Pointer)
	    throw UnsupportedType(type, "multi-dimensional arrays are not supported yet");
    }

}

void IDLExport::adaptNamespace(ostream& stream, string const& ns)
{
    if (m_namespace != ns)
    {
	utilmm::stringlist
	    old_namespace = utilmm::split(m_namespace, "/"),
	    new_namespace = utilmm::split(ns, "/");

	while(!old_namespace.empty() && !new_namespace.empty() && old_namespace.front() == new_namespace.front())
	{
	    old_namespace.pop_front();
	    new_namespace.pop_front();
	}

	closeNamespaces(stream, old_namespace.size());

	while (!new_namespace.empty())
	{
	    stream << m_indent << "module " << new_namespace.front() << " {\n";
	    m_indent += "    ";
	    new_namespace.pop_front();
	}
    }
    m_namespace = ns;
}

std::string IDLExport::getExportNamespace(std::string const& type_ns) const
{
    string ns = type_ns;
    if (!m_ns_prefix.empty())
	ns = m_ns_prefix + "/" + ns;
    if (!m_ns_suffix.empty())
	ns = ns + "/" + m_ns_suffix;
    return ns;
}

std::string IDLExport::getIDLAbsoluteTypename(Type const& type) const
{
    return getIDLAbsoluteTypename(type, m_namespace);
}

std::string IDLExport::getIDLAbsoluteTypename(Type const& type, std::string const& current_namespace) const
{
    string result;

    if (type.getName() == "std::string")
        return "string";

    string type_ns = getExportNamespace(type.getNamespace());
    if (type_ns != current_namespace && type.getCategory() != Type::Numeric)
    {
	result = utilmm::join(utilmm::split(type_ns, "/"), "::");
	if (!result.empty())
	    result += "::";
    }
    return result + getIDLTypename(type);
}

std::string IDLExport::getIDLTypename(Type const& type)
{
    if (type.getCategory() == Type::Numeric)
	return getIDLBaseType(static_cast<Numeric const&>(type));
    else if (type.getCategory() == Type::Compound || type.getCategory() == Type::Enum)
    {
	utilmm::stringlist type_name = utilmm::split(type.getBasename(), " ");
	if (*type_name.begin() == "struct" || *type_name.begin() == "enum")
	    return *type_name.rbegin();
    }

    return type.getBasename();
}

std::string IDLExport::getIDLBaseType(Numeric const& type)
{
    std::string idl_name;
    if (type.getName() == "/bool")
        return "boolean";
    if (type.getNumericCategory() != Numeric::Float)
    {
	if (type.getNumericCategory() == Numeric::UInt && type.getSize() != 1)
	    idl_name = "unsigned ";
	switch (type.getSize())
	{
	    case 1: idl_name += "octet"; break;
	    case 2: idl_name += "short"; break;
	    case 4: idl_name += "long"; break;
	    case 8: idl_name += "long long"; break;
	}
    }
    else
    {
	if (type.getSize() == 4)
	    idl_name = "float";
	else
	    idl_name = "double";
    }

    return idl_name;
}


void IDLExport::save
    ( ostream& stream
    , utilmm::config_set const& config
    , Typelib::Registry const& type )
{
    m_ns_prefix = config.get<std::string>("namespace_prefix", "");
    m_ns_suffix = config.get<std::string>("namespace_suffix", "");
    m_blob_threshold = config.get<int>("blob_threshold", 0);
    m_opaque_as_any  = config.get<bool>("opaque_as_any", false);
    list<string> selection = config.get< list<string> >("selected");
    m_selected_types = set<string>(selection.begin(), selection.end());
    return Exporter::save(stream, config, type);
}

void IDLExport::generateTypedefs(ostream& stream)
{
    for (TypedefMap::const_iterator it = m_typedefs.begin();
            it != m_typedefs.end(); ++it)
    {
        adaptNamespace(stream, it->first);

        list<string> const& lines = it->second;
        for (list<string>::const_iterator str_it = lines.begin(); str_it != lines.end(); ++str_it)
            stream << m_indent << "typedef " << *str_it << std::endl;
    }
}

void IDLExport::save
    ( ostream& stream
    , Typelib::RegistryIterator const& type )
{
    if (! m_selected_types.empty())
    {
        if (m_selected_types.count(type->getName()) == 0)
        {
            return;
        }
    }

    if (type.isAlias())
    {
	// IDL has C++-like rules for struct and enums. Do not alias a "struct A" to "A";
	if (type->getNamespace() != type.getNamespace()
		|| (type->getBasename() != "struct " + type.getBasename()
		    && type->getBasename() != "enum " + type.getBasename()
		    && type.getBasename() != "struct " + type->getBasename()
		    && type.getBasename() != "enum " + type->getBasename()))
	{
	    IDLExport::checkType(*type);
            ostringstream stream;

            std::string type_namespace = getExportNamespace(type.getNamespace());

	    // Alias types using typedef, taking into account that the aliased type
	    // may not be in the same module than the new alias.
	    if (type->getCategory() == Type::Array)
	    {
		Array const& array_t = dynamic_cast<Array const&>(*type);
		stream 
		    << getIDLAbsoluteTypename(array_t.getIndirection(), type_namespace) 
		    << " " << type.getBasename() << "[" << array_t.getDimension() << "];";
	    }
            else if (type->getCategory() == Type::Container)
            {
                // Generate a sequence, regardless of the actual container type
                Container const& container_t = dynamic_cast<Container const&>(*type);
                stream << "sequence<" << getIDLAbsoluteTypename(container_t.getIndirection(), type_namespace) << "> " << type.getBasename() << ";";
            }
	    else
		stream << getIDLAbsoluteTypename(*type, type_namespace) << " " << type.getBasename() << ";";

            m_typedefs[type_namespace].push_back(stream.str());
	}
    }
    else
    {
	// Don't call adaptNamespace right now, since some types can be simply
	// ignored by the IDLExportVisitor -- resulting in an empty module,
	// which is not accepted in IDL
	//
	// Instead, act "as if" the namespace was changed, capturing the output
	// in a temporary ostringstream and change namespace only if some output
	// has actually been generated
	
	string target_namespace = getExportNamespace(type.getNamespace());
	size_t ns_size = utilmm::split(target_namespace, "/").size();
	string indent_string = string(ns_size * 4, ' ');

	if (m_blob_threshold && type->getSize() > m_blob_threshold)
	{
	    adaptNamespace(stream, target_namespace);
	    stream << indent_string << "typedef sequence<octet> " << type->getBasename() << ";\n";
	}
	else
	{
            std::string old_namespace = m_namespace;
            m_namespace = target_namespace;
	    std::ostringstream temp_stream;
	    IDLExportVisitor exporter(type.getRegistry(), *this, temp_stream, indent_string, m_opaque_as_any);
	    exporter.apply(*type);
            m_namespace = old_namespace;

	    string result = temp_stream.str();
	    if (! result.empty())
	    {
		adaptNamespace(stream, target_namespace);
		stream << result;
	    }
	}
    }
}

TYPELIB_REGISTER_IO1(idl, IDLExport)

