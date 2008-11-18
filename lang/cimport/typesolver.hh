#ifndef __TYPESOLVER_H__
#define __TYPESOLVER_H__

#include <CPPParser.hpp>

#include <typelib/typemodel.hh>
#include <typelib/typebuilder.hh>
#include <utilmm/stringtools.hh>
#include <list>
#include <sstream>

namespace Typelib { class Registry; }
   
class TypeSolver : public CPPParser
{
    Typelib::Type*  m_class_object;
    TypeSpecifier   m_class_type;

    utilmm::stringlist m_namespace;

    std::list<CurrentTypeDefinition> m_current;

    std::map<std::string, int> m_constants;

    Typelib::Type& buildClassObject();
    Typelib::Registry& m_registry;

    bool m_cxx_mode;
    bool m_ignore_opaques;

    Typelib::Type const& buildCurrentType();
    void setTypename(std::string const& name);
    void beginTypeDefinition(TypeSpecifier class_type, const std::string& name);

public:
    struct UnsupportedClassType : public std::runtime_error
    {
        const int type;
        UnsupportedClassType(int type_)
            : std::runtime_error("internal error: found a type which is classified neither as struct nor union")
            , type(type_) {}
    };

    struct NestedTypeDefinition : public std::runtime_error
    {
        NestedTypeDefinition(std::string const& inside, std::string const& outside)
            : std::runtime_error("found nested type definition: " + inside + " is defined in " + outside) {}
    };

    TypeSolver(antlr::TokenStream& lexer, Typelib::Registry& registry, bool cxx_mode, bool ignore_opaques);
    TypeSolver(const antlr::ParserSharedInputState& state, Typelib::Registry& registry, bool cxx_mode, bool ignore_opaques);
    
    virtual void beginClassDefinition(TypeSpecifier class_type, const std::string& name);
    virtual void endClassDefinition();
    virtual void beginEnumDefinition(const std::string& name);
    virtual void enumElement(const std::string& name, bool has_value, int value);
    virtual void endEnumDefinition();
    virtual int getIntConstant(std::string const& name);
    virtual int getTypeSize(CurrentTypeDefinition const& def);
    virtual void beginTypedef();

    virtual void enterNamespace(std::string const& name);
    virtual void exitNamespace();

    virtual void beginFieldDeclaration();
    virtual void endFieldDeclaration();
    virtual void foundSimpleType(const std::list<std::string>& full_type);
    virtual void classForwardDeclaration(TypeSpecifier ts, DeclSpecifier,const std::string& name);
    virtual void end_of_stmt();
    virtual void declaratorID(const std::string& name, QualifiedItem);
    virtual void declaratorArray(int size);

    virtual void resetPointerLevel();
    virtual void incrementPointerLevel();

    virtual CurrentTypeDefinition popType();
    virtual CurrentTypeDefinition& pushNewType();
    virtual int getStackSize() const;
    void setTemplateArguments(int count);
};

#endif

