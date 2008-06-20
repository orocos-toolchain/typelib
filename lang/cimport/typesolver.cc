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
using utilmm::join;

ostream& operator << (ostream& io, TypeSolver::CurrentTypeDefinition const& def)
{
    return io << "  " << join(def.name, " ") << " [" << join(def.array, ", ") << "] " << def.pointer_level;
}

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
        CurrentTypeDefinition def;
	def.name.push_back("enum");
	def.name.push_back(name);
        m_current.push_front( def );
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

void TypeSolver::beginTypedef()
{
    pushNewType();
    CPPParser::beginTypedef();
}

int TypeSolver::getIntConstant(std::string const& name)
{
    map<string, int>::iterator it = m_constants.find(name);
    if (it != m_constants.end())
        return it->second;
    else
        throw InvalidConstantName(name);
}

int TypeSolver::getTypeSize(CurrentTypeDefinition const& def)
{
    string def_name = join(def.name, " ");
    Type const* t = m_registry.build(def_name);
    if (t)
        return t->getSize();
    else
        throw InvalidTypeName(def_name);
}

Type& TypeSolver::buildClassObject()
{
    m_current.push_front( CurrentTypeDefinition() );
    m_current.front().name.push_back(m_class_object->getName());

    Type* object = m_class_object;
    m_class_object = 0;
    m_class_type = 0;
    return *object;
}

void TypeSolver::beginFieldDeclaration()
{ 
    pushNewType();
    CPPParser::beginFieldDeclaration();
}
void TypeSolver::endFieldDeclaration()
{ CPPParser::endFieldDeclaration(); }


void TypeSolver::foundSimpleType(const std::list<std::string>& full_type)
{
    if (!m_current.empty() && m_current.front().name.empty())
        m_current.front().name = full_type;

    CPPParser::foundSimpleType(full_type);
}

void TypeSolver::classForwardDeclaration(TypeSpecifier ts, DeclSpecifier ds, const std::string& name)
{
    CurrentTypeDefinition def;
    if (ts == tsSTRUCT)
        def.name.push_back("struct");
    else if (ts == tsUNION)
        def.name.push_back("union");

    def.name.push_back(name);
    m_current.push_front( def );

    CPPParser::classForwardDeclaration(ts, ds, name);
}

Typelib::Type const& TypeSolver::buildCurrentType()
{
    if (m_current.empty())
    {
        *(int*)0 = 10;
        throw TypeStackEmpty();
    }

    CurrentTypeDefinition type_def = m_current.front();

    TypeBuilder builder(m_registry, type_def.name);
    if (type_def.pointer_level)
	builder.addPointer(type_def.pointer_level);

    while (!type_def.array.empty())
    {
	builder.addArrayMinor(type_def.array.front());
	type_def.array.pop_front();
    }

    return builder.getType();
}

void TypeSolver::resetPointerLevel()
{
    if (!m_current.empty())
        m_current.front().pointer_level = 0;
}

void TypeSolver::incrementPointerLevel()
{
    m_current.front().pointer_level++;
}

TypeSolver::CurrentTypeDefinition TypeSolver::popType()
{
    if (m_current.empty())
        throw TypeStackEmpty();

    CurrentTypeDefinition def = m_current.front();
    m_current.pop_front();
    //cerr << "removed " << def << endl;
    return def;
}
void TypeSolver::pushNewType()
{ m_current.push_front( CurrentTypeDefinition() ); }

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
            CurrentTypeDefinition def = m_current.front();

            Compound& compound     = dynamic_cast<Compound&>(*m_class_object);
            Type const& field_type = buildCurrentType();
            if (m_class_type == tsUNION)
                compound.addField(name, field_type, 0);
            else if (m_class_type == tsSTRUCT)
            {
                size_t offset = Packing::getOffsetOf( compound, field_type );
                compound.addField( name, field_type, offset );
            }
            else 
                throw UnsupportedClassType(m_class_type);

            //cerr << "resetting " << m_current.front() << endl;
            m_current.front().pointer_level = 0;
            m_current.front().array.clear();
        }
    }
    else if (qi == qiType)
    {
        Type const& type = buildCurrentType();
        std::string new_name = m_registry.getFullName(name);
        std::string old_name = m_registry.getFullName(type.getName());
        m_registry.alias( old_name, new_name );
    }

    // if (!m_current.empty())
    // {
    //     std::cerr << "WARNING: the set of pending type definitions is not empty" << std::endl;
    //     std::cerr << "WARNING: name == " << name << ", qiType == " << qiType << std::endl;


    //     list<CurrentTypeDefinition>::const_iterator it = m_current.begin(),
    //         end = m_current.end();
    //     for (; it != end; ++it)
    //         cerr << "  " << *it << endl;

    // }

    // if (! m_pending_array.empty())
    // {
    //     std::cerr << "WARNING: the set of pending array dimensions is not empty" << std::endl;
    //     std::cerr << "WARNING: name == " << name << ", qiType == " << qiType << std::endl;
    //     std::cerr << "WARNING: got";
    //     list<size_t>::const_iterator it, end = m_pending_array.end();
    //     for (it = m_pending_array.begin(); it != end; ++it)
    //         std::cerr << " " << *it;
    //     std::cerr << std::endl;
    // }

    // if (pointer_level)
    // {
    //     std::cerr << "WARNING: the pointer level is not zero" << std::endl;
    //     std::cerr << "WARNING: name == " << name << ", qiType == " << qiType << std::endl;
    //     std::cerr << "WARNING: got pointer_level == " << pointer_level << std::endl;
    // }


    CPPParser::declaratorID(name, qi);
}

void TypeSolver::declaratorArray(int size)
{
    if (!m_current.empty())
        m_current.front().array.push_back(size);
    CPPParser::declaratorArray(size);
}

void TypeSolver::end_of_stmt()
{ 
    if (! m_current.empty())
        popType();
    CPPParser::end_of_stmt(); 
}


