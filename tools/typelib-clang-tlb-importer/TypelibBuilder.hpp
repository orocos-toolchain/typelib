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
class TypedefNameDecl;
class Decl;
}
class TypelibBuilder
{
public:
    bool registerType(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context);
    void registerTypeDef(const clang::TypedefType *type);
    void registerTypeDef(const clang::TypedefNameDecl *decl);
    void registerNamedDecl(const clang::TypeDecl *decl);
    bool isConstant(const std::string name, size_t pos);
    bool registerBuildIn(const std::string& canonicalTypeName, const clang::BuiltinType* builtin, clang::ASTContext& context);
    
    void lookupOpaque(const clang::TypeDecl *decl);
    bool loadRegistry(const std::string &filename);
    
    const Typelib::Registry &getRegistry()
    {
        return registry;
    }
    
private:
    bool addFieldsToCompound( Typelib::Compound &compound, const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);
    bool addBaseClassToCompound( Typelib::Compound &compound, const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);
    bool addRecord(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);
    bool addEnum(const std::string& canonicalTypeName, const clang::EnumDecl* decl);
    bool addArray(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context);
    
    const Typelib::Type *checkRegisterType(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context);
    
    bool checkRegisterContainer(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);

    // extract source-loc of given clang::Decl as "/path/tpo/file:linenumber"
    // and store in into the metadata system of the given Typelib::Type
    void setHeaderPathForTypeFromDecl(const clang::Decl *decl,
                                      Typelib::Type &type);

    std::string getTypelibNameForQualType(const clang::QualType &type);

    void printCommentForDecl(const clang::Decl* decl) const;
    
    Typelib::Registry registry;
};

#endif // TYPELIBBUILDER_H
