#ifndef TYPELIB_CIMPORTPLUGIN_H
#define TYPELIB_CIMPORTPLUGIN_H

#include "plugin.hh"

class CImportPlugin : public Plugin
{
public:
    CImportPlugin();

    virtual OptionList getOptions() const;
    virtual bool apply(const OptionList& remaining
            , const utilmm::config_set& options
            , Typelib::Registry& registry);
};

#endif

