#ifndef TYPELIB_EXPORT_IDL_HH
#define TYPELIB_EXPORT_IDL_HH

#include <typelib/exporter.hh>

class IDLExport : public Typelib::Exporter
{
    std::string m_namespace;
    std::string m_indent;

    void closeNamespaces(std::ostream& stream, int levels);
    void adaptNamespace(std::ostream& stream, std::string const& ns);

protected:
    /** Called by save to add data after saving all registry types */
    virtual void end  (std::ostream& stream, Typelib::Registry const& registry);

public:
    IDLExport();

    virtual void save
        ( std::ostream& stream
        , Typelib::RegistryIterator const& type);

    static void checkType(Typelib::Type const& type);
};

#endif

