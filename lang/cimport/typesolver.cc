#include "CPPParser.hh"
#include "CPPLexer.hh"
#include "typesolver.hh"
#include "registry.hh"

#include <iostream>
#include <numeric>
#include <sstream>
#include <cassert>

using namespace std;
using namespace Typelib;

TypeSolver::TypeSolver(const antlr::ParserSharedInputState& state, Registry& registry)
    : CPPParser(state), m_class(0), m_registry(registry) {}

TypeSolver::TypeSolver(antlr::TokenStream& lexer, Registry& registry)
    : CPPParser(lexer), m_class(0), m_registry(registry) {}

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
        buildClassObject(false);

    CPPParser::endClassDefinition();
}

void TypeSolver::beginEnumDefinition(const std::string& name)
{
    m_class = true;
    m_class_name = name;
    m_class_type = tsENUM;
    CPPParser::beginEnumDefinition(name);
}

void TypeSolver::endEnumDefinition()
{
    if (! m_class_name.empty())
        buildClassObject(false);

    CPPParser::endEnumDefinition();
}

void TypeSolver::buildClassObject(bool define_type)
{
    Type* object;
    if (m_class_type == tsENUM)
        object = new Enum(m_registry.getFullName(m_class_name));
    else
    {
        Compound* compound;
        if (m_class_type == tsUNION)
            compound = new Union(m_registry.getFullName(m_class_name), define_type);
        if (m_class_type == tsSTRUCT)
            compound = new Struct(m_registry.getFullName(m_class_name), define_type);
        else 
            throw UnsupportedClassType(m_class_type);

        for (FieldList::iterator it = m_fields.begin(); it != m_fields.end(); ++it)
            compound -> addField( it -> first, it -> second.getType() );
        m_fields.clear();
        object = compound;
    }

    m_registry.add(object);

    m_fieldtype.clear();
    m_fieldtype.push_back(object->getName());
    m_class = false;
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

void TypeSolver::declaratorID(const std::string& name, QualifiedItem qi)
{
    if (m_class && m_class_name.empty() && qi == qiType)
    {
        if (name != "anonymous")
        {
            m_class_name = name;
            buildClassObject(true);
        }
    }
    else if (m_class)
    {
        m_fields.push_back( make_pair(name, TypeBuilder(m_registry, m_fieldtype)) );
        if (pointer_level)
            m_fields.back().second.addPointer(pointer_level);
    }
    else if (qi == qiType)
    {
        TypeBuilder builder(m_registry, m_fieldtype);
        if (pointer_level)
            builder.addPointer(pointer_level);
        
        std::string new_name = m_registry.getFullName(name);
        std::string old_name = m_registry.getFullName(builder.getType().getName());
        m_registry.alias( old_name, new_name );
    }
    CPPParser::declaratorID(name, qi);
}

void TypeSolver::declaratorArray(int size)
{
    if (m_class && !m_fields.empty())
        m_fields.back().second.addArray(size);
        
    CPPParser::declaratorArray(size);
}

void TypeSolver::end_of_stmt()
{ 
    m_fieldtype.clear();
    CPPParser::end_of_stmt(); 
}


