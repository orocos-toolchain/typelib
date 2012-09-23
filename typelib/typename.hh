#ifndef __TYPELIB_TYPENAME_H__
#define __TYPELIB_TYPENAME_H__

#include <string>
#include <list>
#include <boost/tokenizer.hpp>

namespace Typelib
{
    typedef std::string::value_type   NamespaceMarkType;
    static const NamespaceMarkType    NamespaceMark = '/';
    static const NamespaceMarkType    TemplateMark  = '<';
    static const char*                NamespaceMarkString = "/";

    struct NameSeparator : public boost::char_separator<NamespaceMarkType>
    {
        NameSeparator()
            : boost::char_separator<NamespaceMarkType>(NamespaceMarkString, "") {}
    };
    typedef boost::tokenizer<NameSeparator> NameTokenizer;

    
    bool isAbsoluteName(const std::string& name);
    bool isValidNamespace(const std::string& name, bool absolute);
    bool isValidTypename(const std::string& name, bool absolute);
    bool isInNamespace(const std::string& type, const std::string& nspace, bool recursive = false);

    /** Splits a type name into its components. It is non-validating, i.e. is
     * guaranteed to return a valid value only for names for which
     * isValidTypename returns true
     */
    std::list<std::string> splitTypename(std::string const& name);
    
    /** Returns +name+ as a namespace name, valid to build an absolute type
     * name by simple concatenation
     *
     * It is guaranteed to end with the NamespaceMark
     */
    std::string getNormalizedNamespace(const std::string& name);
    /** Removes the namespace part of \c full_name and returns it */
    std::string getTypename (const std::string& full_name);
    /** Returns the normalized namespace part of \c full_name. The returned
     * namespace name starts and ends with the namespace separator '/'. */
    std::string getNamespace(const std::string& full_name);
    /** Returns the relative part of +name+, relative to the given namespace.
     * The type name must be included in the namespace. Use
     * getMinimalPathTo to generate the minimal namespace specification reaching
     * a type from a namespace.
     *
     * The returned name does not have any leading namespace separator. */
    std::string getRelativeName(std::string const& full_name, std::string const& ns);
    /** Computes the minimal namespace specification needed to reach a type from
     * a namespace. The specification takes into account possible ambiguities,
     * i.e.
     *
     *   getMinimalPathTo("/A/A/Type", "/A/B/A")
     *
     * will return  "/A/A/Type" as "A/Type" would be ambiguous.
     */
    std::string getMinimalPathTo(std::string const& full_name, std::string const& ns);
    /** Returns true if \c name1 is either in a more in-depth namespace than
     * name2 (i.e. name2 == /A/B/class and name1 == /A/B/C/class2 or if 
     * name2 < name1 (lexicographic sort). Otherwise, returns false
     */
    bool nameSort(std::string const& name1, std::string const& name2);
};

#endif


