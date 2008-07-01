#ifndef __GENOM_MODULE_H
#define __GENOM_MODULE_H

#include <typelib/importer.hh>

class CImport : public Typelib::Importer
{
public:
    /** Load the C file at \c path into \c registry
     * The available options are:
     * <ul>
     *  <li> \c defines  the define arguments to cpp (-D options)
     *  <li> \c includes the include arguments to cpp (-I options)
     *  <li> \c rawflags flags to be passed as-is to cpp 
     *  <li> \c debug output debugging information on stdout, and keep preprocessed output
     * </ul>
     */
    virtual void load
        ( std::string const& path
        , utilmm::config_set const& config
        , Typelib::Registry& registry);

    /** Import the types defined in \c stream into the registry
     * The stream is not passed through cpp
     */
    virtual void load
        ( std::istream& stream
        , utilmm::config_set const& config
        , Typelib::Registry& registry );
};

#endif

