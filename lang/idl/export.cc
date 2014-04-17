#include "export.hh"
#include <iostream>

#include <typelib/typevisitor.hh>
#include <typelib/plugins.hh>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace
{
    using namespace std;
    using namespace Typelib;
    using boost::split;
    using boost::join;
    using boost::is_any_of;

    static size_t namespaceIndentLevel(std::string const& ns)
    {
        vector<string> separators;
        split(separators, ns, is_any_of("/"));
        return separators.size();
    }

    static string normalizeIDLName(std::string const& name)
    {
        unsigned int template_mark;
        string result = name;
        while ((template_mark = result.find_first_of("<>/,")) < result.length())
            result.replace(template_mark, 1, "_");
        return result;
    }

    static string getIDLAbsoluteNamespace(std::string const& type_ns, IDLExport const& exporter)
    {
        string ns = type_ns;
        string prefix = exporter.getNamespacePrefix();
        string suffix = exporter.getNamespaceSuffix();
        if (!prefix.empty())
            ns = prefix + ns;
        if (!suffix.empty())
            ns += suffix;
        return ns;
    }

    class IDLTypeIdentifierVisitor : public TypeVisitor
    {
    public:
	IDLExport const& m_exporter;
        string    m_front, m_back;
        string    m_namespace;

        bool visit_(OpaqueType const& type);
        bool visit_(NullType const& type);
        bool visit_(Container const& type);
        bool visit_(Compound const& type);

        bool visit_(Numeric const& type);

        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

        bool visit_(Enum const& type);

        IDLTypeIdentifierVisitor(IDLExport const& exporter)
            : m_exporter(exporter) {}
        string getTargetNamespace() const { return m_namespace; }
        std::string getIDLAbsolute(Type const& type, std::string const& field_name = "");
        std::string getIDLRelative(Type const& type, std::string const& field_name = "");
        pair<string, string> getIDLBase(Type const& type, std::string const& field_name = "");
        void apply(Type const& type)
        {
            m_namespace = getIDLAbsoluteNamespace(type.getNamespace(), m_exporter);
            TypeVisitor::apply(type);
            m_front = normalizeIDLName(m_front);
        }
    };

    static bool isIDLBuiltinType(Type const& type)
    {
        if (type.getCategory() == Type::Numeric || type.getName() == "/std/string")
            return true;
        else if (type.getCategory() == Type::Array)
        {
            Type const& element_type = static_cast<Array const&>(type).getIndirection();
            return isIDLBuiltinType(element_type);
        }
        return false;
    }

    /** Returns the IDL identifier for the given type, without the type's own
     * namespace prepended
     */
    static pair<string, string> getIDLBase(Type const& type, IDLExport const& exporter, std::string const& field_name = std::string())
    {
        std::string type_name;
        IDLTypeIdentifierVisitor visitor(exporter);
        visitor.apply(type);
        if (field_name.empty())
            return make_pair(visitor.getTargetNamespace(), visitor.m_front + visitor.m_back);
        else
            return make_pair(visitor.getTargetNamespace(), visitor.m_front + " " + field_name + visitor.m_back);
    }

    /** Returns the IDL identifier for the given type, with the minimal
     * namespace specification to reach it from the exporter's current namespace
     */
    static string getIDLRelative(Type const& type, std::string const& relative_to, IDLExport const& exporter, std::string const& field_name = std::string())
    {
        pair<string, string> base = getIDLBase(type, exporter, field_name);
        if (!base.first.empty())
        {
            std::string ns = getMinimalPathTo(base.first + type.getBasename(), relative_to);
            boost::replace_all(ns, Typelib::NamespaceMarkString, "::");
            return normalizeIDLName(ns) + base.second;
        }
        else return base.second;
    }

    /** Returns the IDL identifier for the given type, with the complete
     * namespace specification prepended to it
     */
    static string getIDLAbsolute(Type const& type, IDLExport const& exporter, std::string const& field_name = std::string())
    {
        pair<string, string> base = getIDLBase(type, exporter, field_name);
        if (!base.first.empty())
        {
            std::string ns = base.first;
            boost::replace_all(ns, Typelib::NamespaceMarkString, "::");
            return normalizeIDLName(ns) + base.second;
        }
        else return base.second;
    }

    std::string IDLTypeIdentifierVisitor::getIDLAbsolute(Type const& type, std::string const& field_name)
    {
        return ::getIDLAbsolute(type, m_exporter, field_name);
    }
    std::string IDLTypeIdentifierVisitor::getIDLRelative(Type const& type, std::string const& field_name)
    {
        return ::getIDLRelative(type, m_namespace, m_exporter, field_name);
    }
    pair<string, string> IDLTypeIdentifierVisitor::getIDLBase(Type const& type, std::string const& field_name)
    {
        return ::getIDLBase(type, m_exporter, field_name);
    }

    bool IDLTypeIdentifierVisitor::visit_(OpaqueType const& type)
    {
        if (m_exporter.marshalOpaquesAsAny())
        {
            m_namespace = "";
            m_front = "any";
        }
        else
            throw UnsupportedType(type, "opaque types are not allowed in IDL");

        return true;
    }
    bool IDLTypeIdentifierVisitor::visit_(NullType const& type)
    { throw UnsupportedType(type, "null types are not supported for export in IDL, found " + type.getName()); }
    bool IDLTypeIdentifierVisitor::visit_(Container const& type)
    {
        if (type.getName() == "/std/string")
        {
            m_namespace = "";
            m_front = "string";
        }
        else
        {
            // Get the basename for "kind"
            m_namespace = getIDLBase(type.getIndirection()).first;
            // We generate all containers in orogen::Corba, even if their
            // element type is a builtin
            if (m_namespace.empty())
                m_namespace = getIDLAbsoluteNamespace("/", m_exporter);

            std::string container_kind = Typelib::getTypename(type.kind());
            std::string element_name   = type.getIndirection().getName();
            boost::replace_all(element_name, Typelib::NamespaceMarkString, "_");
            boost::replace_all(element_name, " ", "_");
            m_front = container_kind + "_" + element_name + "_";
        }

        return true;
    }
    bool IDLTypeIdentifierVisitor::visit_(Compound const& type)
    { m_front = type.getBasename();
        return true; }
    bool IDLTypeIdentifierVisitor::visit_(Numeric const& type)
    {
        m_namespace = "";
        if (type.getName() == "/bool")
        {
            m_front = "boolean";
        }
        else if (type.getNumericCategory() != Numeric::Float)
        {
            if (type.getNumericCategory() == Numeric::UInt && type.getSize() != 1)
                m_front = "unsigned ";
            switch (type.getSize())
            {
                case 1: m_front += "octet"; break;
                case 2: m_front += "short"; break;
                case 4: m_front += "long"; break;
                case 8: m_front += "long long"; break;
            }
        }
        else
        {
            if (type.getSize() == 4)
                m_front = "float";
            else
                m_front = "double";
        }
        return true;
    }
    bool IDLTypeIdentifierVisitor::visit_(Pointer const& type)
    { throw UnsupportedType(type, "pointer types are not supported for export in IDL"); }
    bool IDLTypeIdentifierVisitor::visit_(Array const& type)
    {
        if (type.getIndirection().getCategory() == Type::Array)
            throw UnsupportedType(type, "multi-dimensional arrays are not supported in IDL");

        pair<string, string> element_t = getIDLBase(type.getIndirection());
        m_namespace = element_t.first;
        m_front = element_t.second;
        m_back = "[" + boost::lexical_cast<string>(type.getDimension()) + "]";
        return true;
    }
    bool IDLTypeIdentifierVisitor::visit_(Enum const& type)
    { m_front = type.getBasename();
        return true; }

    class IDLExportVisitor : public TypeVisitor
    {
    public:
	IDLExport const& m_exporter;
        ostringstream  m_stream;
        string    m_indent;
        string    m_namespace;
        std::map<std::string, Type const*>& m_exported_typedefs;

        bool visit_(OpaqueType const& type);
        bool visit_(Container const& type);
        bool visit_(Compound const& type);
        bool visit_(Compound const& type, Field const& field);

        bool visit_(Numeric const& type);

        bool visit_(Pointer const& type);
        bool visit_(Array const& type);

        bool visit_(Enum const& type);

        IDLExportVisitor(Registry const& registry, IDLExport const& exporter, std::map<std::string, Type const*>& exported_typedefs);
        std::string getResult() const { return m_stream.str(); }
        std::string getTargetNamespace() const { return m_namespace; }
        void setTargetNamespace(std::string const& target_namespace)
        {
            size_t ns_size = namespaceIndentLevel(target_namespace);
            m_indent = string(ns_size * 4, ' ');
            m_namespace = target_namespace;
        }
        std::string getIDLAbsolute(Type const& type, std::string const& field_name = "");
        std::string getIDLRelative(Type const& type, std::string const& field_name = "");
        pair<string, string> getIDLBase(Type const& type, std::string const& field_name = "");
        void apply(Type const& type)
        {
            setTargetNamespace(getIDLAbsoluteNamespace(type.getNamespace(), m_exporter));
            TypeVisitor::apply(type);
        }
    };

    std::string IDLExportVisitor::getIDLAbsolute(Type const& type,
            std::string const& field_name)
    {
        return ::getIDLAbsolute(type, m_exporter, field_name);
    }
    std::string IDLExportVisitor::getIDLRelative(Type const& type,
            std::string const& field_name)
    {
        return ::getIDLRelative(type, m_namespace, m_exporter, field_name);
    }
    pair<string, string> IDLExportVisitor::getIDLBase(Type const& type,
            std::string const& field_name)
    {
        return ::getIDLBase(type, m_exporter, field_name);
    }

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
	    std::map<std::string, Type const*>& exported_typedefs)
        : m_exporter(exporter)
        , m_exported_typedefs(exported_typedefs) {}

    bool IDLExportVisitor::visit_(Compound const& type)
    { 
        m_stream << m_indent << "struct " << normalizeIDLName(type.getBasename()) << " {\n";
        
        { Indent indenter(m_indent);
            TypeVisitor::visit_(type);
        }

        m_stream << m_indent
            << "};\n";

        return true;
    }
    bool IDLExportVisitor::visit_(Compound const& type, Field const& field)
    { 
        m_stream
            << m_indent
            << getIDLAbsolute(field.getType(), field.getName())
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
	m_stream << m_indent << "enum " << type.getBasename() << " { ";

        list<string> symbols;
        Enum::ValueMap const& values = type.values();
	Enum::ValueMap::const_iterator it, end = values.end();
	for (it = values.begin(); it != end; ++it)
	    symbols.push_back(it->first);
	m_stream << join(symbols, ", ") << " };\n";

        return true;
    }

    bool IDLExportVisitor::visit_(Container const& type)
    {
        if (type.getName() == "/std/string")
            return true;
                
        // sequence<> can be used as-is, but in order to be as cross-ORB
        // compatible as possible we generate sequence typedefs and use them in
        // the compounds. Emit the sequence right now.
	string target_namespace  = getIDLBase(type.getIndirection()).first;
        // We never emit sequences into the main namespace, even if their
        // element is a builtin
        if (target_namespace.empty())
            target_namespace = getIDLAbsoluteNamespace("/", m_exporter);
        setTargetNamespace(target_namespace);

        std::string element_name = getIDLAbsolute(type.getIndirection());
        std::string typedef_name = getIDLBase(type).second;
        boost::replace_all(typedef_name, "::", "_");
        m_stream << m_indent << "typedef sequence<" << element_name << "> " << typedef_name << ";\n";
        m_exported_typedefs.insert(make_pair(type.getIndirection().getNamespace() + typedef_name, &type));

        return true;
    }
    bool IDLExportVisitor::visit_(OpaqueType const& type)
    {
        if (m_exporter.marshalOpaquesAsAny())
            return true;

        throw UnsupportedType(type, "opaque types are not supported for export in IDL");
    }
}

using namespace std;
IDLExport::IDLExport() 
    : m_namespace("/"), m_opaque_as_any(false) {}

bool IDLExport::marshalOpaquesAsAny() const { return m_opaque_as_any; }

void IDLExport::end
    ( ostream& stream
    , Typelib::Registry const& /*registry*/ )
{
    generateTypedefs(stream);

    // Close the remaining namespaces
    list<string> ns_levels;
    split(ns_levels, m_namespace, is_any_of("/"));
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
        list<string> old_namespace, new_namespace;
        split(old_namespace, m_namespace, is_any_of("/"));
        split(new_namespace, ns, is_any_of("/"));

	while(!old_namespace.empty() && !new_namespace.empty() && old_namespace.front() == new_namespace.front())
	{
	    old_namespace.pop_front();
	    new_namespace.pop_front();
	}

	closeNamespaces(stream, old_namespace.size());

	while (!new_namespace.empty())
	{
	    stream << m_indent << "module " << normalizeIDLName(new_namespace.front()) << " {\n";
	    m_indent += "    ";
	    new_namespace.pop_front();
	}
    }
    m_namespace = ns;
}

std::string IDLExport::getIDLAbsolute(Type const& type) const
{
    return ::getIDLAbsolute(type, *this);
}

std::string IDLExport::getIDLRelative(Type const& type, std::string const& relative_to) const
{
    return ::getIDLRelative(type, relative_to, *this);
}

void IDLExport::save
    ( ostream& stream
    , utilmm::config_set const& config
    , Typelib::Registry const& type )
{
    m_ns_prefix = config.get<std::string>("namespace_prefix", "");
    if (!m_ns_prefix.empty() && string(m_ns_prefix, 0, 1) != Typelib::NamespaceMarkString)
        m_ns_prefix = Typelib::NamespaceMarkString + m_ns_prefix;
    m_ns_suffix = config.get<std::string>("namespace_suffix", "");
    if (!m_ns_suffix.empty() && string(m_ns_suffix, m_ns_suffix.length() - 1, 1) != Typelib::NamespaceMarkString)
        m_ns_suffix += Typelib::NamespaceMarkString;
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

std::string IDLExport::getNamespacePrefix() const
{
    return m_ns_prefix;
}
std::string IDLExport::getNamespaceSuffix() const
{
    return m_ns_suffix;
}

bool IDLExport::save
    ( ostream& stream
    , Typelib::RegistryIterator const& type )
{
    if (! m_selected_types.empty())
    {
        if (m_selected_types.count(type->getName()) == 0)
        {
            return false;
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

            std::string type_namespace = getIDLAbsoluteNamespace(type.getNamespace(), *this);

            map<string, Type const*>::const_iterator already_exported =
                m_exported_typedefs.find(type.getName());
            if (already_exported != m_exported_typedefs.end())
            {
                if (*already_exported->second != *type)
                    throw UnsupportedType(*type, "the typedef name " + type.getName() + " is reserved by the IDL exporter");
                return false;
            }
            else if (type.getBasename().find_first_of("/<>") != std::string::npos)
            {
                return false;
            }

	    // Alias types using typedef, taking into account that the aliased type
	    // may not be in the same module than the new alias.
	    if (type->getCategory() == Type::Array)
	    {
		Array const& array_t = dynamic_cast<Array const&>(*type);
		stream 
		    << getIDLAbsolute(array_t.getIndirection())
		    << " " << type.getBasename() << "[" << array_t.getDimension() << "];";
	    }
            else if (type->getCategory() == Type::Container && type->getName() != "/std/string")
            {
                // Generate a sequence, regardless of the actual container type
                Container const& container_t = dynamic_cast<Container const&>(*type);
                stream << "sequence<" << getIDLAbsolute(container_t.getIndirection()) << "> " << type.getBasename() << ";";
            }
            else if (type->getCategory() == Type::Opaque)
            {
                if (marshalOpaquesAsAny())
                    stream << "any " << type.getBasename() << ";";
            }
            else if (type.getBasename().find_first_of(" ") == string::npos)
		stream << getIDLAbsolute(*type) << " " << type.getBasename() << ";";

            std::string def = stream.str();
            if (!def.empty())
                m_typedefs[type_namespace].push_back(def);
            return true;
	}
        else return false;
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
	
	if (m_blob_threshold && static_cast<int>(type->getSize()) > m_blob_threshold)
	{
            string target_namespace = getIDLAbsoluteNamespace(type.getNamespace(), *this);
            size_t ns_size = namespaceIndentLevel(target_namespace);
            string indent_string = string(ns_size * 4, ' ');

	    adaptNamespace(stream, target_namespace);
	    stream << indent_string << "typedef sequence<octet> " << type->getBasename() << ";\n";
            return true;
	}
	else
	{
	    IDLExportVisitor exporter(type.getRegistry(), *this, m_exported_typedefs);
	    exporter.apply(*type);

	    string result = exporter.getResult();
	    if (result.empty())
                return false;
            else
	    {
		adaptNamespace(stream, exporter.getTargetNamespace());
		stream << result;
                return true;
	    }
	}
    }
}

TYPELIB_REGISTER_IO1(idl, IDLExport)

