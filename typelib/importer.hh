#ifndef TYPELIB_IMPORTER_HH
#define TYPELIB_IMPORTER_HH

#include <string>

namespace utilmm
{
    class config_set;
}

namespace Typelib
{
    class Registry;

    class Importer
    {
        public:
            virtual bool import
                (std::string const& path
                , utilmm::config_set const& config
                , Typelib::Registry& registry) = 0;
    };
}

#endif

