#ifndef __GENOM_MODULE_H
#define __GENOM_MODULE_H

#include "importer.hh"

class CImport : public Typelib::Importer
{
public:
    /** Load the C file at \c path into \c registry
     * The available options are:
     * <ul>
     *  <li> \c defines the define arguments to cpp (-D options)
     *  <li> \c includes the include arguments to cpp (-I options)
     * </ul>
     */
    virtual bool load
        (std::string const& path
        , utilmm::config_set const& config
        , Typelib::Registry& registry);
};

#endif

