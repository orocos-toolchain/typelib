#include "CPPParser.hpp"
#include "CPPLexer.hpp"
#include "typesolver.hh"
#include <typelib/registry.hh>
#include <utilmm/stringtools.hh>
#include "packing.hh"

#include <iostream>
#include <numeric>
#include <sstream>
#include <cassert>

using namespace std;
using namespace Typelib;

TypeSolver::TypeSolver(const antlr::ParserSharedInputState& state, Registry& registry, bool cxx_mode)
    : CPPParser(state), m_class_object(0), m_registry(registry), m_cxx_mode(cxx_mode) {}

TypeSolver::TypeSolver(antlr::TokenStream& lexer, Registry& registry, bool cxx_mode)
    : CPPParser(lexer), m_class_object(0), m_registry(registry), m_cxx_mode(cxx_mode) {}

void TypeSolver::setTypename(std::string const& name)
{
    std::string type_name = name, alias_name = name;
    // add "struct", "enum" or "union" prefixes to the right name, according to
    // the value of m_cxx_mode
    { std::string* prefixed_name;
        if (m_cxx_mode) prefixed_name = &alias_name;
        else            prefixed_name = &type_name;

        if (m_class_type == tsENUM)
            *prefixed_name = "enum " + *prefixed_name;
        else if (m_class_type == tsUNION)
            *prefixed_name = "union " + *prefixed_name;
        else if (m_class_type == tsSTRUCT)
            *prefixed_name = "struct " + *prefixed_name;
    }

    m_class_object->setName(m_registry.getFullName(type_name));
    m_registry.add(m_class_object);
    if (m_cxx_mode)
        m_registry.alias(m_class_object->getName(), m_registry.getFullName(alias_name));
}

void TypeSolver::beginTypeDefinition(TypeSpecifier class_type, const std::string& name)
{
    m_class_type = class_type;
    if (m_class_type == tsENUM)
        m_class_object = new Enum("");
    else
        m_class_object = new Compound("");

    if (name != "" && name != "anonymous")
        setTypename(name);
}

void TypeSolver::beginClassDefinition(TypeSpecifier class_type, const std::string& name)
{
    beginTypeDefinition(class_type, name);
    CPPParser::beginClassDefinition(class_type, name);
}

void TypeSolver::endClassDefinition()
{
    if (!m_class_object->getName().empty()) // typedef struct {} bla handled later in declarationID
        buildClassObject();

    CPPParser::endClassDefinition();
}

void TypeSolver::enterNamespace(std::string const& name)
{
    m_namespace.push_back(name);
    m_registry.setDefaultNamespace( "/" + utilmm::join(m_namespace, "/") );
}

void TypeSolver::exitNamespace()
{
    m_namespace.pop_back();
    m_registry.setDefaultNamespace( "/" + utilmm::join(m_namespace, "/") );
}

void TypeSolver::beginEnumDefinition(const std::string& name)
{
    if (m_class_object)
    {
	m_fieldtype.push_back("enum");
	m_fieldtype.push_back(name);
    }
    else
        beginTypeDefinition(tsENUM, name);
    CPPParser::beginEnumDefinition(name);
}

void TypeSolver::enumElement(const std::string& name, bool has_value, int value)
{
    Enum& enum_type = dynamic_cast<Enum&>(*m_class_object);
    if (!has_value)
        value = enum_type.getNextValue();
    
    enum_type.add(name, value);
    m_constants[name] = value;
    CPPParser::enumElement(name, has_value, value);
}

void TypeSolver::endEnumDefinition()
{
    CPPParser::endEnumDefinition();

    if (m_class_type != tsENUM)
	return;

    if (! m_class_object->getName().empty()) // typedef union {} bla handled later in declarationID
        buildClassObject();
}

int TypeSolver::getIntConstant(std::string const& name)
{
    map<string, int>::iterator it = m_constants.find(name);
    if (it != m_constants.end())
        return it->second;
    else
        throw InvalidConstantName(name);
}

int TypeSolver::getTypeSize(std::string const& name)
{
    Type const* t = m_registry.build(name);
    if (t)
        return t->getSize();
    else
        throw InvalidTypeName(name);
}

Type& TypeSolver::buildClassObject()
{
    m_fieldtype.clear();
    m_fieldtype.push_back(m_class_object->getName());

    Type* object = m_class_object;
    m_class_object = 0;
    m_class_type = 0;
    return *object;
}

void TypeSolver::beginFieldDeclaration()
{
    m_fieldtype.clear();
    CPPParser::beginFieldDeclaration();
}

void TypeSolver::foundSimpleType(const std::list<std::string>& full_type)
{
    m_fieldtype = full_type;
    CPPParser::foundSimpleType(full_type);
}

void TypeSolver::classForwardDeclaration(TypeSpecifier ts, DeclSpecifier ds, const std::string& name)
{
    m_fieldtype.clear();

    if (ts == tsSTRUCT)
        m_fieldtype.push_back("struct");
    else if (ts == tsUNION)
        m_fieldtype.push_back("union");

    m_fieldtype.push_back(name);
    CPPParser::classForwardDeclaration(ts, ds, name);
}

TypeBuilder TypeSolver::initializeTypeBuilder()
{
    TypeBuilder builder(m_registry, m_fieldtype);
    if (pointer_level)
	builder.addPointer(pointer_level);

    while (!m_pending_array.empty())
    {
	builder.addArrayMinor(m_pending_array.front());
	m_pending_array.pop_front();
    }

    pointer_level = 0;
    return builder;
}

void TypeSolver::declaratorID(const std::string& name, QualifiedItem qi)
{
    if (m_class_object && m_class_object->getName().empty() && qi == qiType) // typedef (struct|union|enum) { } NAME;
    {
        setTypename(name);
        Type& new_type = buildClassObject();
        if (! m_cxx_mode)
            m_registry.alias(new_type.getName(), m_registry.getFullName(name), "");
    }
    else if (m_class_object) // we are building a compound
    {
        if (m_class_type != tsENUM)
        {
            Compound& compound = dynamic_cast<Compound&>(*m_class_object);
            TypeBuilder builder = initializeTypeBuilder();
            Type const& field_type = builder.getType();
            if (m_class_type == tsUNION)
                compound.addField(name, field_type, 0);
            else if (m_class_type == tsSTRUCT)
            {
                size_t offset = Packing::getOffsetOf( compound, field_type );
                compound.addField( name, field_type, offset );
            }
            else 
                throw UnsupportedClassType(m_class_type);
        }
    }
    else if (qi == qiType)
    {
        TypeBuilder builder = initializeTypeBuilder();
        std::string new_name = m_registry.getFullName(name);
        std::string old_name = m_registry.getFullName(builder.getType().getName());
        m_registry.alias( old_name, new_name );
    }
    else
    {
        // we were parsing either a function definition or an argument
        // definition. In that case, just ignore the pending array values.
        m_pending_array.clear();
    }

    if (! m_pending_array.empty())
    {
	std::cerr << "WARNING: the set of pending array dimensions is not empty" << std::endl;
	std::cerr << "WARNING: name == " << name << ", qiType == " << qiType << std::endl;
        std::cerr << "WARNING: got";
        list<size_t>::const_iterator it, end = m_pending_array.end();
        for (it = m_pending_array.begin(); it != end; ++it)
            std::cerr << " " << *it;
        std::cerr << std::endl;
    }

    CPPParser::declaratorID(name, qi);
}

void TypeSolver::declaratorArray(int size)
{
    m_pending_array.push_back(size);
    CPPParser::declaratorArray(size);
}

void TypeSolver::end_of_stmt()
{ 
    m_fieldtype.clear();
    CPPParser::end_of_stmt(); 
}


