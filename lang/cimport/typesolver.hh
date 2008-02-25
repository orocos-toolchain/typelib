#ifndef __TYPESOLVER_H__
#define __TYPESOLVER_H__

#include <CPPParser.hpp>

#include "typemodel.hh"
#include "typebuilder.hh"
#include <utilmm/stringtools.hh>
#include <list>
#include <sstream>

namespace Typelib { class Registry; }
   
class TypeSolver : public CPPParser
{
    bool            m_class;
    std::string     m_class_name;
    TypeSpecifier   m_class_type;

    utilmm::stringlist m_namespace;

    // For Compound
    typedef std::pair<std::string, Typelib::TypeBuilder> FieldBuilder;
    typedef std::list< FieldBuilder > FieldList;
    std::list<std::string> m_fieldtype;
    FieldList       m_fields;

    // For Enums
    typedef std::list< std::pair<std::string, int> > ValueMap;
    ValueMap        m_enum_values;

    void buildClassObject(bool define_type);
    Typelib::Registry& m_registry;

    bool m_cxx_mode;

public:
    class UnsupportedClassType
    {
        int m_type;
    public:
        UnsupportedClassType(int type) : m_type(type) {}
        std::string toString() 
        { 
            std::ostringstream stream;
            stream << "unsupported class type " << m_type; 
            return stream.str();
        }
    };
    
    TypeSolver(antlr::TokenStream& lexer, Typelib::Registry& registry, bool cxx_mode);
    TypeSolver(const antlr::ParserSharedInputState& state, Typelib::Registry& registry, bool cxx_mode);
    
    virtual void beginClassDefinition(TypeSpecifier class_type, const std::string& name);
    virtual void endClassDefinition();
    virtual void beginEnumDefinition(const std::string& name);
    virtual void enumElement(const std::string& name, bool has_value, int value);
    virtual void endEnumDefinition();

    virtual void enterNamespace(std::string const& name);
    virtual void exitNamespace();

    virtual void beginFieldDeclaration();
    virtual void foundSimpleType(const std::list<std::string>& full_type);
    virtual void classForwardDeclaration(TypeSpecifier ts, DeclSpecifier,const std::string& name);
    virtual void end_of_stmt();
    virtual void declaratorID(const std::string& name, QualifiedItem);
    virtual void declaratorArray(int size);
};

#endif

