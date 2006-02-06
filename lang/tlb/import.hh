#ifndef TYPELIB_LANG_TLB_IMPORT_HH
#define TYPELIB_LANG_TLB_IMPORT_HH

#include "importer.hh"
#include "xmltools.hh"

class TlbImport : public Typelib::Importer
{
    void parse(std::string const& source_id, xmlDocPtr doc, Typelib::Registry& registry);

public:
    virtual void load
        ( std::istream& stream
        , utilmm::config_set const& config
        , Typelib::Registry& registry);

    virtual void load
        ( std::string const& file
        , utilmm::config_set const& config
        , Typelib::Registry& registry);
};

#endif

