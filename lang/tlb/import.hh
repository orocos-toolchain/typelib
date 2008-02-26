#ifndef TYPELIB_LANG_TLB_IMPORT_HH
#define TYPELIB_LANG_TLB_IMPORT_HH

#include <typelib/importer.hh>

class TlbImport : public Typelib::Importer
{
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

