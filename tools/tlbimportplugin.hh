#ifndef TYPELIB_TLBIMPORTPLUGIN_H
#define TYPELIB_TLBIMPORTPLUGIN_H

#include "plugin.hh"

class TlbImportPlugin : public Plugin
{
public:
    TlbImportPlugin();

    virtual bool apply(const OptionList& remaining
            , const utilmm::config_set& options
            , Typelib::Registry& registry);
};

#endif

