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

TypeSolver::TypeSolver(const antlr::ParserSharedInputState& state, Registry& registry, bool cxx_mode, bool ignore_opaques)
    : CPPParser(state), m_class_object(0), m_registry(registry), m_cxx_mode(cxx_mode), m_ignore_opaques(ignore_opaques) {}

TypeSolver::TypeSolver(antlr::TokenStream& lexer, Registry& registry, bool cxx_mode, bool ignore_opaques)
    : CPPParser(lexer), m_class_object(0), m_registry(registry), m_cxx_mode(cxx_mode), m_ignore_opaques(ignore_opaques) {}

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
    if (m_class_object)
        throw NestedTypeDefinition(name, m_class_object->getName());

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
        CurrentTypeDefinition& def = pushNewType();
	def.name.push_back("enum");
	def.name.push_back(name);
    }
    else
        beginTypeDefinition(tsENUM, name);
    CPPParser::beginEnumDefinition(name);
}

void TypeSolver::enumElement(const std::string& name, bool has_value, int value)
{
    if (m_class_type != tsENUM)
        throw NestedTypeDefinition(m_current.front().name.back(), m_class_object->getName());

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
    CurrentTypeDefinition& def = pushNewType();
    def.name.push_back(m_class_object->getName());

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
    CurrentTypeDefinition& def = pushNewType();
    if (ts == tsSTRUCT)
        def.name.push_back("struct");
    else if (ts == tsUNION)
        def.name.push_back("union");

    def.name.push_back(name);

    CPPParser::classForwardDeclaration(ts, ds, name);
}

BOOST_STATIC_ASSERT( sizeof(vector<int8_t>) == sizeof(vector<double>) );
BOOST_STATIC_ASSERT( sizeof(set<int8_t>) == sizeof(set<double>) );
BOOST_STATIC_ASSERT( sizeof(map<int8_t,int16_t>) == sizeof(map<double,float>) );
struct TemplateDef {
    char const* name;
    size_t size;
};
TemplateDef const ALLOWED_TEMPLATES[] = {
    { "std/vector", sizeof(vector<int>) },
    { "std/set", sizeof(set<int>) },
    { "std/map", sizeof(map<int, int>) },
    { 0, 0 }
};

Typelib::Type const& TypeSolver::buildCurrentType()
{
    if (m_current.empty())
        throw TypeStackEmpty("buildCurrentType");

    CurrentTypeDefinition type_def = popType();

    // Check if type_def.name is an allowed container (vector, map, set). If it
    // is the case, make sure that it is already defined in the registry
    if (!type_def.template_args.empty())
    {
        typedef Container::AvailableContainers Containers;
        typedef std::list<std::string> stringlist;

        string fullname = join(type_def.name, "/");
        if (fullname[0] != '/')
            fullname = "/" + fullname;

        Containers containers = Container::availableContainers();
        Containers::const_iterator factory = containers.find(fullname);
        if (factory == containers.end())
            throw Undefined(fullname);
        
        std::list<Type const*> template_args;
        for (stringlist::const_iterator it = type_def.template_args.begin();
                it != type_def.template_args.end(); ++it)
        {
            template_args.push_back( m_registry.build( *it ) );
        }

        return (*factory->second)(m_registry, template_args);
    }

    TypeBuilder builder(m_registry, type_def.name);
    if (type_def.pointer_level)
	builder.addPointer(type_def.pointer_level);

    while (!type_def.array.empty())
    {
	builder.addArrayMinor(type_def.array.front());
	type_def.array.pop_front();
    }

    Type const& type = builder.getType();
    return type;
}

void TypeSolver::resetPointerLevel()
{
    if (!m_current.empty())
        m_current.front().pointer_level = 0;
}

void TypeSolver::incrementPointerLevel()
{
    if (!m_current.empty())
        m_current.front().pointer_level++;
}

TypeSolver::CurrentTypeDefinition TypeSolver::popType()
{
    if (m_current.empty())
        throw TypeStackEmpty("popType");

    CurrentTypeDefinition def = m_current.front();
    m_current.pop_front();
    return def;
}
TypeSolver::CurrentTypeDefinition& TypeSolver::pushNewType()
{
    m_current.push_front( CurrentTypeDefinition() );
    return m_current.front();
}
int TypeSolver::getStackSize() const { return m_current.size(); }

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
            // Keep it here to readd it afterwards. This handles definitions
            // like
            //   TYPE a, *b, c[10], d;
            // I.e. TYPE remains the same but the modifiers are reset.
            CurrentTypeDefinition def = m_current.front();

            Compound& compound     = dynamic_cast<Compound&>(*m_class_object);
            Type const& field_type = buildCurrentType();
            if (m_class_type == tsUNION)
                compound.addField(name, field_type, 0);
            else if (m_class_type == tsSTRUCT)
            {
                size_t offset = 0;
                if (field_type.getCategory() != Type::Opaque || !m_ignore_opaques)
                    offset = Packing::getOffsetOf( compound, field_type );
                else
                {
                    Compound::FieldList const& fields(compound.getFields());
                    if (fields.empty()) offset = 0;
                    else
                    {
                        Field const& last_field = fields.back();
                        offset = last_field.getOffset() + last_field.getType().getSize();
                    }
                }

                compound.addField( name, field_type, offset );
            }
            else 
                throw UnsupportedClassType(m_class_type);

            compound.setSize( Packing::getSizeOfCompound(compound) );
            def.pointer_level = 0;
            def.array.clear();
            m_current.push_front(def);
        }
    }
    else if (qi == qiType)
    {
        Type const& type = buildCurrentType();
        string base_name = m_registry.getFullName(type.getName());
        string new_name  = m_registry.getFullName(name);

        Type const* base_type = m_registry.get(base_name);
        if (! base_type)
            throw Undefined(base_name);

        // Ignore cases where the aliased type already exists and is equivalent
        // to the source type.
        Type const* aliased_type = m_registry.get(new_name);
        if (aliased_type)
        {
            if (!aliased_type->isSame(*base_type))
                throw AlreadyDefined(new_name);
        }
        else
            m_registry.alias( base_name, new_name );

    }

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
    while (! m_current.empty())
        popType();
    CPPParser::end_of_stmt(); 
}

void TypeSolver::setTemplateArguments(int count)
{
    if (m_current.empty())
        throw TypeStackEmpty("setTemplateArguments");

    std::list<std::string> template_args;
    while (count)
    {
        Type const& arg_type = buildCurrentType();
        template_args.push_front(arg_type.getName());
        --count;
    }

    m_current.front().template_args = template_args;
}

