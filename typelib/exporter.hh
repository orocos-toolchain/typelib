#ifndef TYPELIB_IMPORTER_HH
#define TYPELIB_IMPORTER_HH

namespace std    { class string; }
namespace utilmm { class config_set; }

namespace Typelib
{
    class Registry;
    class Exporter
    {
        public:
            virtual bool save
                (std::string const& path
                , utilmm::config_set const& config
                , Registry const& registry) = 0;
    };
}

#endif

