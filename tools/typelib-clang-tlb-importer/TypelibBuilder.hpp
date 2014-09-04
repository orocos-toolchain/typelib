#ifndef TYPELIBBUILDER_H
#define TYPELIBBUILDER_H

#include <typelib/registry.hh>
#include <string>

namespace clang {
class CXXRecordDecl;
class BuiltinType;
class TypeDecl;
class EnumDecl;
class Type;
class ASTContext;
class TypedefNameDecl;
class Decl;
}

class TypelibBuilder
{
public:
    void registerTypedefNameDecl(const clang::TypedefNameDecl *decl);
    void registerTypeDecl(const clang::TypeDecl *decl);
    
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

    /** fill the internal registry-database with informations extracted from a tlb-file
     *
     * should be used to pre-load opaque-informations. might throw if something
     * fishy detected.
     *
     * @param filename path to tlb-file to load
     */
    void loadRegistry(const std::string &filename);

    /** write internal registry to file
     *
     * @param filename path to tlb-file to save
     */
    void saveRegistry(const std::string &filename);
    
    /** accessor for internal registry
     *
     * @return const-ref of internal registry, for exporting for example
     */
    const Typelib::Registry &getRegistry()
    {
        return registry;
    }
    
private:
    bool registerType(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context);
    bool registerBuildIn(const std::string& canonicalTypeName, const clang::BuiltinType* builtin, clang::ASTContext& context);

    bool addFieldsToCompound( Typelib::Compound &compound, const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);
    bool addBaseClassToCompound( Typelib::Compound &compound, const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);
    bool addRecord(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);
    bool addEnum(const std::string& canonicalTypeName, const clang::EnumDecl* decl);
    bool addArray(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context);
    
    const Typelib::Type *checkRegisterType(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context);
    
    bool checkRegisterContainer(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl);

    // extract source-loc of given clang::Decl as "/path/to/file:lineNumber"
    // and store in into the meta-data system of the given Typelib::Type
    void setMetaDataSourceFileLine(const clang::Decl *decl,
                                   Typelib::Type *type);
    // add all base-classes of the given clang::Decl into the meta-data of the
    // Typelib::Type
    void setMetaDataBaseClasses(const clang::Decl *decl, Typelib::Type *type);
    // extract comments attached given clang::Decl and add them as meta-data to
    // the Typelib::Type
    void setMetaDataDoc(const clang::Decl *decl, Typelib::Type *type);

    Typelib::Registry registry;
};

#endif // TYPELIBBUILDER_H
