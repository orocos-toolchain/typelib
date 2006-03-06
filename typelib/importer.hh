#ifndef TYPELIB_IMPORTER_HH
#define TYPELIB_IMPORTER_HH

#include <string>
#include "pluginmanager.hh"

namespace utilmm { class config_set; }

namespace Typelib
{
    class Registry;

    class Importer
    {
    public:
        virtual ~Importer() {}

        virtual void load
            ( std::istream& stream
            , utilmm::config_set const& config
            , Registry& registry) = 0;

        virtual void load
            ( std::string const& path
            , utilmm::config_set const& config
            , Registry& registry);
    };


}

#endif

