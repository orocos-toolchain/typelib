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
    
    /** fill properties of a decl into Opaque of registry
     *
     * if a type was found in the parsed headers which is to be added as a
     * opaque. Will also fill some metadata of the Type:
     *
     * - 'source_file_line' the "path/to/file:columnNumber" of the opaque
     * - 'opaque_is_typedef' a bool saying wether this opaque is a typedef. If
     *   it is, an alias to the underlying type is also added to the registry.
     * - 'base_classes' if the opaque is a CXXRecord, all inherited
     *   child-classes will be noted in the registry
     *
     * this function expects the Opaques to be pre-loaded into the registry, by
     * loading the respective tlb-file for example.
     *
     * @param decl decl of the declaration to be added to the database as opaque
     */
    void registerOpaque(const clang::TypeDecl *decl);

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

    // extract source-loc of given clang::Decl as "/path/to/file:lineNumber"
    // and store in into the meta-data system of the given Typelib::Type
    void setHeaderPathForTypeFromDecl(const clang::Decl *decl,
                                      Typelib::Type *type);
    // add all base-classes of the given clang::Decl into the meta-data of the
    // Typelib::Type
    void setBaseClassesForTypeFromDecl(const clang::Decl *decl,
                                       Typelib::Type *type);
    // extract comments attached given clang::Decl and add them as meta-data to
    // the Typelib::Type
    void setDocStringForTypeFromDecl(const clang::Decl *decl,
                                     Typelib::Type *type);

    Typelib::Registry registry;
};

#endif // TYPELIBBUILDER_H
