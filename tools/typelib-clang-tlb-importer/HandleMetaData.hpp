#ifndef HANDLEMETADATA_H
#define HANDLEMETADATA_H

namespace Typelib {
class Type;
}
namespace clang {
class Decl;
class NamedDecl;
}

// extract source-loc of given clang::Decl as "/path/to/file:lineNumber"
// and store in into the meta-data system of the given Typelib::Type
void setMetaDataSourceFileLine(const clang::Decl *decl, Typelib::Type *type);
// add the chain of includes leading to this type as "orogen_include"
// entries to the metadatabase
void setMetaDataOrogenInclude(const clang::Decl *decl, Typelib::Type *type);
// add all base-classes of the given clang::Decl into the meta-data of the
// Typelib::Type
void setMetaDataBaseClasses(const clang::Decl *decl, Typelib::Type *type);
// extract comments attached given clang::Decl and add them as meta-data to
// the Typelib::Type
void setMetaDataDoc(const clang::Decl *decl, Typelib::Type *type);
// for debugging and pretty-printing well add the actual qualified name as
// additional metadata
void setMetaDataCxxname(const clang::NamedDecl *decl, Typelib::Type *type);

#endif /*HANDLEMETADATA_H*/
