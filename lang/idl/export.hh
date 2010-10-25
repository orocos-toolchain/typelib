#ifndef TYPELIB_EXPORT_IDL_HH
#define TYPELIB_EXPORT_IDL_HH

#include <typelib/exporter.hh>

/* Exports types or a whole registry in the CORBA Interface Definition Language (IDL).
 * This exporter accepts the following options:
 * * \c namespace_prefix: a namespace to prepend to the type namespace. Useful
 *      to have the normal C types coexist with the CORBA-generated types.
 * * \c namespace_suffix: a namespace to add to the types suffixes. Useful
 *      to have the normal C types coexist with the CORBA-generated types.
 * * \c blob_threshold: if set to a nonzero value, a type which is bigger than
 *      the given value will be exported as a sequence<octet>. Useful if
 *      performance is important, and if type safety is not an issue (for
 *      instance if the software using CORBA implements its own type safety
 *      mechanisms)
 */
class IDLExport : public Typelib::Exporter
{
    std::string m_namespace;
    std::string m_ns_prefix;
    std::string m_ns_suffix;
    std::string m_indent;
    int m_blob_threshold;
    bool m_opaque_as_any;

    std::set<std::string> m_selected_types;

    struct TypedefSpec
    {
        std::string name_space;
        std::string declaration;
    };
    typedef std::map<std::string, std::list<std::string> > TypedefMap;
    TypedefMap m_typedefs;

    /** The set of typedefs that have been exported during the generation
     * process, -- i.e. before the end of generation
     */
    std::map<std::string, Typelib::Type const*> m_exported_typedefs;


    void closeNamespaces(std::ostream& stream, int levels);
    void adaptNamespace(std::ostream& stream, std::string const& ns);
    void generateTypedefs(std::ostream& stream);

protected:
    /** Called by save to add data after saving all registry types */
    virtual void end  (std::ostream& stream, Typelib::Registry const& registry);

public:
    IDLExport();

    bool marshalOpaquesAsAny() const;

    std::string getNamespacePrefix() const;
    std::string getNamespaceSuffix() const;

    std::string getIDLAbsolute(Typelib::Type const& type) const;
    std::string getIDLRelative(Typelib::Type const& type, std::string const& relative_to) const;

    virtual void save
        ( std::ostream& stream
	, utilmm::config_set const& config
        , Typelib::Registry const& type);

    virtual bool save
        ( std::ostream& stream
        , Typelib::RegistryIterator const& type);

    static void checkType(Typelib::Type const& type);
};

#endif

