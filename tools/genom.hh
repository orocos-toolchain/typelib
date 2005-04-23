#ifndef TYPELIB_GENOM_H
#define TYPELIB_GENOM_H

#include "plugin.hh"

class GenomPlugin : public Plugin
{
public:
    GenomPlugin();

    virtual OptionList getOptions() const;
    virtual bool apply(const OptionList& remaining
            , const utilmm::config_set& options
            , Typelib::Registry* registry);
};

#endif

