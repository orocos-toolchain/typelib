#ifndef TYPELIBBUILDER_H
#define TYPELIBBUILDER_H
#include <map>
#include <string>
#include <stdint.h>
#include <vector>
#include <typelib/registry.hh>

namespace clang {
class CXXRecordDecl;
class TypedefType;
class BuiltinType;
class NamedDecl;
class TypeDecl;
class EnumDecl;
class Type;
class ASTContext;
}
class TypelibBuilder
{
    class TemporaryFieldType {
    public:
        std::string typeName;
        std::string fieldName;
        uint32_t offsetInBits;
    };
    
    class TemporaryType {
    public:
        std::string typeName;
        
        uint32_t sizeInBytes;
        
        std::vector<TemporaryFieldType> fields;
        
    };
public:
    bool registerType(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context);
    void registerTypeDef(const clang::TypedefType *type);
    void registerNamedDecl(const clang::TypeDecl *decl);
    std::string cxxToTyplibName(const std::string name);
    bool isConstant(const std::string name, size_t pos);
    void registerBuildIn(const std::string& canonicalTypeName, const clang::BuiltinType* builtin, clang::ASTContext& context);
    
    
    void buildRegistry();
    
private:
    void addRecord(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);
    void addEnum(const std::string& canonicalTypeName, const clang::EnumDecl* decl);
    
    typedef std::map<std::string, TemporaryType> TypeMap;
    std::map<std::string, TemporaryType> typeMap;
    Typelib::Registry registry;
};

#endif // TYPELIBBUILDER_H
