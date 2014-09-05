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
class ConstantArrayType;
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
    Typelib::Type const *registerType(const std::string &canonicalTypeName,
                                    const clang::Type *type,
                                    clang::ASTContext &context);

    /** add builtins to database: basically ints+floats as Typelib::Numeric
     *
     * attention, will silently rename "char" into "[u]int8_t" depending on
     * signedness and create the apropriate alias.
     *
     * @param canonicalTypeName
     * @param builtin
     * @param context
     * @return NULL if nothing was added
     */
    Typelib::Type const *registerBuiltIn(const std::string &canonicalTypeName,
                                         const clang::BuiltinType *builtin,
                                         clang::ASTContext &context);

    /** add members and base-classes of a class to the given compound
     *
     * calls itself recursively for all the base-classes.
     *
     * does not test for private-ness anywhere.
     *
     * @return true if everything worked. false if some member-variable failed to be added
     */
    bool addMembersOfClassToCompound(Typelib::Compound &compound,
                                     const std::string &canonicalTypeName,
                                     const clang::CXXRecordDecl *decl);

    Typelib::Type const *addRecord(const std::string &canonicalTypeName,
                                   const clang::CXXRecordDecl *decl);

    /** add enum to database
     *
     * also add string-value pairs and some metadata
     *
     * @return ptr to added Type in database
     */
    Typelib::Type const *addEnum(const std::string &canonicalTypeName,
                                 const clang::EnumDecl *decl);

    /** add array to database if the element-type can be added
     *
     * @return true if smth was added to database
     */
    Typelib::Type const *addArray(const std::string &canonicalTypeName,
                                  const clang::ConstantArrayType *type,
                                  clang::ASTContext &context);

    /** add entry to database if the given type is a "Typelib::Container"
     *
     * a "Typelib::Container" handle a small number of special records in typelib like
     * "std::vector" or "std::string".
     * fields or records containing these should be added to the database nevertheless.
     *
     * @return true if the given decl could be added as "Container"
     */
    Typelib::Type const *
    checkRegisterContainer(const std::string &canonicalTypeName,
                           const clang::CXXRecordDecl *decl);

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
