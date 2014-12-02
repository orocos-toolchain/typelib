#ifndef NAMINGCONVERSIONS_H
#define NAMINGCONVERSIONS_H

#include <clang/AST/Type.h>
#include <clang/AST/Decl.h>

#include <string>
#include <vector>

/** convert name of a c++ decl into typelib syntax
 *
 * NOTE: if given the decl of a template-specialization the individual
 * parameters will be converted and added to the returned string altough the
 * name of the "CXXRecordDecl" does _not_ contain the template-parameters.
 *
 * @param decl object whose name is to be resolved
 * @return the resolved name of the cxx-type in typelib lingo
 */
std::string cxxToTypelibName(const clang::NamedDecl* decl);

/** convert name of a c++ type into typelib syntax
 *
 * @param type object whose name is to be resolved
 * @return the resolved name of the cxx-type in typelib lingo
 */
std::string cxxToTypelibName(const clang::QualType& type);

/** convert name of a c++ type into typelib syntax
 *
 * carefull, only use this function if you have to, as the other two versions
 * automatically handle template-conversion.
 *
 * @param cxxName the string to convert.
 * @return the resolved name of the cxx-type in typelib lingo
 */
std::string cxxToTypelibName(const std::string& cxxName);

/** converts a "typelib" name to a "c++" name. uses some assumptions.
 *
 * for example:
 *    /base/Joints -> base::Joints
 *
 * @param typelibName the name as used in the typelib-side of the portal
 * @return the string converted to c++ lingo
 */
std::string typelibToCxxName(const std::string& typelibName);

/** convert a typelibname like "/ns/object</float,/int>" into
 * someting like "std::pair('/ns/object',std::vector('/float','/int'))"
 *
 * NOTE: this is purely text-based by looking for "<", ">" and ","
 *
 * NOTE: no templates of templates allowed!
 */
std::pair<std::string, std::vector<std::string> >
typelibNameSplitTemplate(const std::string &typelibName);

#endif /*NAMINGCONVERSIONS_H*/
