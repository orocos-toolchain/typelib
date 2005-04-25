#ifndef __GENOM_MODULE_H
#define __GENOM_MODULE_H

#include "importer.hh"

class CImporter : public Typelib::Importer
{
public:
    virtual bool import
        (const std::string& path
        , utilmm::config_set const& config
        , Typelib::Registry& registry);
};

#endif

