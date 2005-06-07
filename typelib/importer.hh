#ifndef TYPELIB_IMPORTER_HH
#define TYPELIB_IMPORTER_HH

#include <string>

namespace utilmm { class config_set; }

namespace Typelib
{
    class Registry;

    class ReadError : public std::exception {};
    
    class Importer
    {
        public:
            virtual bool load
                (std::string const& path
                , utilmm::config_set const& config
                , Registry& registry) = 0;
    };
}

#endif

