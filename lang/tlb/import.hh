#ifndef TYPELIB_LANG_TLB_IMPORT_HH
#define TYPELIB_LANG_TLB_IMPORT_HH

#include "importer.hh"

class TlbImport : public Typelib::Importer
{
public:
    virtual bool load
        ( std::string const& path
        , utilmm::config_set const& config
        , Typelib::Registry& registry);
};

#endif

