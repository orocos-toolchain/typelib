#ifndef NAMINGCONVERSIONS_H
#define NAMINGCONVERSIONS_H

#include <clang/AST/Type.h>
#include <clang/AST/Decl.h>

#include <string>
#include <vector>

/** convert name of a c++ decl into typelib syntax
 *
 * @param decl object whose name is to be resolved
 * @return the resolved name of the cxx-type in typelib lingo
 */
std::string cxxToTyplibName(const clang::NamedDecl* decl);

/** convert name of a c++ type into typelib syntax
 *
 * @param type object whose name is to be resolved
 * @return the resolved name of the cxx-type in typelib lingo
 */
std::string cxxToTyplibName(const clang::QualType& type);

/** converts a "typelib" name to a "c++" name. uses some assumptions.
 *
 * for example:
 *    /base/Joints -> base::Joints
 *
 * @param typelibName the name as used in the typelib-side of the portal
 * @return the string converted to c++ lingo
 */
std::string typlibtoCxxName(const std::string& typelibName);

#endif /*NAMINGCONVERSIONS_H*/
