#ifndef NAMINGCONVERSIONS_H
#define NAMINGCONVERSIONS_H

#include <string>

std::string cxxToTyplibName(const std::string& name);

/** converts a "typelib" name to a "c++" name
 *
 * for example:
 *    /base/Joints -> base::Joints
 * FIXME: add more concrete examples
 *
 * @param typelibName the name as used in the typelib-side of the portal
 * @return the string converted to c++ lingo
 */
std::string typlibtoCxxName(const std::string& typelibName);

#endif /*NAMINGCONVERSIONS_H*/
