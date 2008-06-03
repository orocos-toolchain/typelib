#include "CPPParser.hpp"
#include "CPPLexer.hpp"
#include "typesolver.hh"
#include <typelib/registry.hh>
#include "packing.hh"

#include <iostream>
#include <numeric>
#include <sstream>
#include <cassert>

using namespace std;
using namespace Typelib;

TypeSolver::TypeSolver(const antlr::ParserSharedInputState& state, Registry& registry, bool cxx_mode)
    : CPPParser(state), m_class(0), m_registry(registry), m_cxx_mode(cxx_mode) {}

TypeSolver::TypeSolver(antlr::TokenStream& lexer, Registry& registry, bool cxx_mode)
    : CPPParser(lexer), m_class(0), m_registry(registry), m_cxx_mode(cxx_mode) {}

void TypeSolver::beginClassDefinition(TypeSpecifier class_type, const std::string& name)
{
    m_class = true;
    if (name == "anonymous")
        m_class_name = "";
    else
        m_class_name = name;
    m_class_type = class_type;

    CPPParser::beginClassDefinition(class_type, name);
}

void TypeSolver::endClassDefinition()
{
    if (!m_class_name.empty()) // typedef struct {} bla handled later in declarationID
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
    if (m_class)
    {
	m_fieldtype.push_back("enum");
	m_fieldtype.push_back(name);
    }
    else
    {
	m_class = true;
	m_class_name = name;
	m_class_type = tsENUM;
    }
    CPPParser::beginEnumDefinition(name);
}

void TypeSolver::enumElement(const std::string& name, bool has_value, int value)
{
    if (!has_value)
    {
        if (m_enum_values.empty())
            value = 0;
        else
            value = m_enum_values.back().second + 1;
    }
    
    m_enum_values.push_back(make_pair(name, value));
    CPPParser::enumElement(name, has_value, value);
}

void TypeSolver::endEnumDefinition()
{
    CPPParser::endEnumDefinition();

    if (m_class_type != tsENUM)
	return;

    if (! m_class_name.empty())
        buildClassObject();
}

Type& TypeSolver::buildClassObject()
{
    Type* object;

    std::string type_name = m_class_name, alias_name = m_class_name;
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

    if (m_class_type == tsENUM)
    {
        auto_ptr<Enum> enum_def(new Enum(m_registry.getFullName(type_name)));
        ValueMap::iterator it;
        for (it = m_enum_values.begin(); it != m_enum_values.end(); ++it)
            enum_def->add(it->first, it->second);
        
        m_enum_values.clear();
        object = enum_def.release();
    }
    else
    {
        Compound* compound = new Compound(m_registry.getFullName(type_name));
        if (m_class_type == tsUNION)
        {
            for (FieldList::iterator it = m_fields.begin(); it != m_fields.end(); ++it)
                compound->addField(it->first, it->second.getType(), 0);
        }
        else if (m_class_type == tsSTRUCT)
        {
            for (FieldList::iterator it = m_fields.begin(); it != m_fields.end(); ++it)
            {
                size_t offset = Packing::getOffsetOf( *compound, it->second.getType() );
                compound -> addField( it -> first, it -> second.getType(), offset );
            }
        }
        else 
            throw UnsupportedClassType(m_class_type);

        m_fields.clear();
        object = compound;
    }

    m_registry.add(object);

    if (m_cxx_mode)
        m_registry.alias(object->getName(), m_registry.getFullName(alias_name), "");

    m_fieldtype.clear();
    m_fieldtype.push_back(object->getName());
    m_class = false;

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
    if (m_class && m_class_name.empty() && qi == qiType)
    {
        if (name != "anonymous")
        {
            m_class_name = name;
	    Type& new_type = buildClassObject();
	    if (! m_cxx_mode)
		m_registry.alias(new_type.getName(), m_registry.getFullName(name), "");
        }
    }
    else if (m_class)
        m_fields.push_back( make_pair(name, initializeTypeBuilder()) );
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


