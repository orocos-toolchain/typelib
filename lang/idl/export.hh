#ifndef TYPELIB_EXPORT_IDL_HH
#define TYPELIB_EXPORT_IDL_HH

#include "exporter.hh"

class IDLExport : public Typelib::Exporter
{
    std::string m_namespace;
    std::string m_indent;

    void closeNamespaces(std::ostream& stream, int levels);
    void adaptNamespace(std::ostream& stream, std::string const& ns);

protected:
    /** Called by save to add a prelude before saving all registry types */
    virtual bool begin(std::ostream& stream, Typelib::Registry const& registry);
    /** Called by save to add data after saving all registry types */
    virtual bool end  (std::ostream& stream, Typelib::Registry const& registry);

public:
    IDLExport();

    virtual bool save
        ( std::ostream& stream
        , Typelib::RegistryIterator const& type);

    static void checkType(Typelib::Type const& type);
};

#endif

