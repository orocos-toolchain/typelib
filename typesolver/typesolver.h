#ifndef __TYPESOLVER_H__
#define __TYPESOLVER_H__

#include "CPPParser.hpp"

#include "type.h"
#include "typebuilder.h"
#include <list>
#include <sstream>

class Registry;
   
class TypeSolver : public CPPParser
{
    bool m_class;
    std::string m_class_name;
    TypeSpecifier m_class_type;
    std::list<std::string> m_fieldtype;

    typedef std::pair<std::string, TypeBuilder> FieldBuilder;
    typedef std::list< FieldBuilder > FieldList;
    FieldList m_fields;

    void buildClassObject(bool define_type);

    Registry* m_registry;

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
    
    TypeSolver(antlr::TokenStream& lexer, Registry* registry);
    TypeSolver(const antlr::ParserSharedInputState& state, Registry* registry);
    
    virtual void beginClassDefinition(TypeSpecifier class_type, const std::string& name);
    virtual void endClassDefinition();
    virtual void beginEnumDefinition(const std::string& name);
    virtual void endEnumDefinition();

    virtual void beginFieldDeclaration();
    virtual void foundSimpleType(const std::list<std::string>& full_type);
    virtual void classForwardDeclaration(TypeSpecifier ts, DeclSpecifier,const std::string& name);
    virtual void end_of_stmt();
    virtual void declaratorID(const std::string& name, QualifiedItem);
    virtual void declaratorArray(int size);
};

#endif

