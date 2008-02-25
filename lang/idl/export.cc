#include "export.hh"
#include <iostream>
#include <utilmm/stringtools.hh>

#include "typevisitor.hh"

namespace
{
    using namespace std;
    using namespace Typelib;
    class IDLExportVisitor : public TypeVisitor
    {
        ostream&  m_stream;
        string    m_indent;

        template<typename T>
        void display_compound(T const& type, char const* compound_name);
        bool display_field(Field const& field);

	static std::string getIDLTypename(Type const& type);
	static std::string getIDLBaseType(Numeric const& type);

    protected:
        bool visit_(Compound const& type);
        bool visit_(Compound const& type, Field const& field);

        bool visit_(Numeric const& type);

        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

        bool visit_(Enum const& type);

    public:
        IDLExportVisitor(ostream& stream, string const& base_indent);
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

    IDLExportVisitor::IDLExportVisitor(ostream& stream, string const& base_indent)
        : m_stream(stream), m_indent(base_indent) {}

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
		<< getIDLTypename(array_type.getIndirection())
		<< " "
		<< field.getName()
		<< "[" << array_type.getDimension() << "];\n";
	}
	else
	{
	    m_stream 
		<< m_indent
		<< getIDLTypename(field.getType()) << " "
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
	m_stream << m_indent << "enum " << getIDLTypename(type) << " { ";

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
    return true;
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

bool IDLExport::save
    ( ostream& stream
    , Typelib::RegistryIterator const& type )
{
    if (m_namespace != type.getNamespace())
    {
	NameTokenizer old_namespace(m_namespace);
	NameTokenizer new_namespace(type.getNamespace());
	NameTokenizer::const_iterator old_it = old_namespace.begin();
	NameTokenizer::const_iterator new_it = new_namespace.begin();
	while (*old_it == *new_it && old_it != old_namespace.end() && new_it != new_namespace.end())
	{
	    ++old_it;
	    ++new_it;
	}

	for (; old_it != old_namespace.end(); ++old_it)
	{
	    m_indent = std::string(m_indent, 0, m_indent.size() - 4);
	    stream << "\n" << m_indent << "}\n";
	}
	for (; new_it != new_namespace.end(); ++new_it)
	{
	    stream << m_indent << "module " << *new_it << " {\n";
	    m_indent += "    ";
	}
	m_namespace = type.getNamespace();
    }

    if (type.isAlias())
    {
	// IDL has C++-like rules for struct and enums. Do not alias a "struct A" to "A";
	if (type->getNamespace() != type.getNamespace()
		|| (type->getBasename() != "struct " + type.getBasename()
		    && type->getBasename() != "enum " + type.getBasename()))
	{
	    IDLExport::checkType(*type);

	    // Alias types using typedef, taking into account that the aliased type
	    // may not be in the same module than the new alias.
	    stream << m_indent << "typedef ";

	    std::string idl_ns = utilmm::join(utilmm::split(type->getNamespace(), "/"), "::");
	    if (!idl_ns.empty() && type->getNamespace() != m_namespace)
		stream << idl_ns << "::";
	    stream << type->getBasename() << " " << type.getBasename() << ";\n";
	}
    }
    else
    {
        IDLExportVisitor exporter(stream, m_indent);
        exporter.apply(*type);
    }
    return true;
}

