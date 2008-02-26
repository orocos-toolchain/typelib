#include "export.hh"
#include <iostream>
#include <utilmm/stringtools.hh>

#include <typelib/typevisitor.hh>

namespace
{
    using namespace std;
    using namespace Typelib;
    class IDLExportVisitor : public TypeVisitor
    {
    public:
        ostream&  m_stream;
        string    m_indent;
	string    m_namespace;

        template<typename T>
        void display_compound(T const& type, char const* compound_name);
        bool display_field(Field const& field);

	static std::string getIDLAbsoluteTypename(Type const& type, string const& ns);
	static std::string getIDLTypename(Type const& type);
	static std::string getIDLBaseType(Numeric const& type);

        bool visit_(Compound const& type);
        bool visit_(Compound const& type, Field const& field);

        bool visit_(Numeric const& type);

        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

        bool visit_(Enum const& type);

        IDLExportVisitor(ostream& stream, string const& base_indent, string const& ns);
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

    IDLExportVisitor::IDLExportVisitor(ostream& stream, string const& base_indent, string const& ns)
        : m_stream(stream), m_indent(base_indent), m_namespace(ns) {}

    std::string IDLExportVisitor::getIDLTypename(Type const& type)
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

    std::string IDLExportVisitor::getIDLAbsoluteTypename(Type const& type, std::string const& current_namespace)
    {
	string result;

	if (type.getNamespace() != current_namespace)
	{
	    result = utilmm::join(utilmm::split(type.getNamespace(), "/"), "::");
	    if (!result.empty())
		result += "::";
	}
	return result + getIDLTypename(type);
    }

    bool IDLExportVisitor::visit_(Compound const& type)
    { 
        m_stream << m_indent << "struct " << getIDLTypename(type) << " {\n";
        
        { Indent indenter(m_indent);
            TypeVisitor::visit_(type);
        }

        m_stream << m_indent
            << "};\n";

        return true;
    }
    bool IDLExportVisitor::visit_(Compound const& type, Field const& field)
    { 
	IDLExport::checkType(field.getType());

	if (field.getType().getCategory() == Type::Array)
	{
	    Array const& array_type = static_cast<Array const&>(field.getType());
	    m_stream
		<< m_indent
		<< IDLExportVisitor::getIDLAbsoluteTypename(array_type.getIndirection(), m_namespace)
		<< " "
		<< field.getName()
		<< "[" << array_type.getDimension() << "];\n";
	}
	else
	{
	    m_stream 
		<< m_indent
		<< IDLExportVisitor::getIDLAbsoluteTypename(field.getType(), m_namespace) << " "
		<< field.getName() << ";\n";
	}

        return true;
    }

    bool IDLExportVisitor::visit_(Numeric const& type)
    {
	// no need to export Numeric types, they are already defined by IDL
	// itself
	return true;
    }

    std::string IDLExportVisitor::getIDLBaseType(Numeric const& type)
    {
	std::string idl_name;
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

    bool IDLExportVisitor::visit_ (Pointer const& type)
    {
	throw UnsupportedType(type, "pointers are not allowed in IDL");
        return true;
    }
    bool IDLExportVisitor::visit_ (Array const& type)
    {
	IDLExport::checkType(type);
	m_stream << m_indent << type.getBasename() << "[" << type.getDimension() << "]\n";
        return true;
    }

    bool IDLExportVisitor::visit_ (Enum const& type)
    {
	m_stream << m_indent << "enum " << IDLExportVisitor::getIDLAbsoluteTypename(type, m_namespace) << " { ";

	utilmm::stringlist symbols;
        Enum::ValueMap const& values = type.values();
	Enum::ValueMap::const_iterator it, end = values.end();
	for (it = values.begin(); it != values.end(); ++it)
	    symbols.push_back(it->first);
	m_stream << utilmm::join(symbols, ", ") << " };\n";

        return true;
    }
}

using namespace std;
IDLExport::IDLExport() 
    : m_namespace("/") {}

bool IDLExport::begin
    ( ostream& stream
    , Typelib::Registry const& /*registry*/ )
{
    return true;
}
bool IDLExport::end
    ( ostream& stream
    , Typelib::Registry const& /*registry*/ )
{
    // Close the remaining namespaces
    utilmm::stringlist
	ns_levels = utilmm::split(m_namespace, "/");
    closeNamespaces(stream, ns_levels.size());

    return true;
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
	    throw UnsupportedType(type, "multi-dimentional arrays are not supported yet");
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

bool IDLExport::save
    ( ostream& stream
    , Typelib::RegistryIterator const& type )
{
    if (type.isAlias())
    {
	// IDL has C++-like rules for struct and enums. Do not alias a "struct A" to "A";
	if (type->getNamespace() != type.getNamespace()
		|| (type->getBasename() != "struct " + type.getBasename()
		    && type->getBasename() != "enum " + type.getBasename()))
	{
	    IDLExport::checkType(*type);

	    adaptNamespace(stream, type.getNamespace());
	    // Alias types using typedef, taking into account that the aliased type
	    // may not be in the same module than the new alias.
	    stream << m_indent << "typedef ";
	    stream << IDLExportVisitor::getIDLAbsoluteTypename(*type, m_namespace) << " " << type.getBasename() << ";\n";
	}
    }
    else
    {
	adaptNamespace(stream, type.getNamespace());
        IDLExportVisitor exporter(stream, m_indent, m_namespace);
        exporter.apply(*type);
    }
    return true;
}


