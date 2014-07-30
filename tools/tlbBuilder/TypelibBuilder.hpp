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
class ConstantArrayType;
class Type;
class ASTContext;
class QualType;
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
    bool registerBuildIn(const std::string& canonicalTypeName, const clang::BuiltinType* builtin, clang::ASTContext& context);
    
    
    void buildRegistry();
    
private:
    bool addFieldsToCompound( Typelib::Compound &compound, const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);
    bool addRecord(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);
    bool addEnum(const std::string& canonicalTypeName, const clang::EnumDecl* decl);
    bool addArray(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context);
    
    const Typelib::Type *checkRegisterType(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context);
    
    std::string getTypelibNameForQualType(const clang::QualType &type);
    
    typedef std::map<std::string, TemporaryType> TypeMap;
    std::map<std::string, TemporaryType> typeMap;
    Typelib::Registry registry;
};

#endif // TYPELIBBUILDER_H