#ifndef TYPELIB_GENOM_H
#define TYPELIB_GENOM_H

#include "plugin.h"

class GenomPlugin : public Plugin
{
public:
    GenomPlugin();

    virtual OptionList getOptions() const;
    virtual bool apply(const OptionList& remaining
            , const Utils::ConfigSet& options
            , Typelib::Registry* registry);
};

#endif

