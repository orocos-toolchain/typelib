#ifndef TYPELIB_MANIP_HH
#define TYPELIB_MANIP_HH

namespace utilmm { class config_set; }

namespace Typelib
{
    class Registry;

    class Manip
    {
        virtual bool manip 
             ( Registry const& source
             , Registry& destination
             , utilmm::config_set const& config ); 
    };
}

#endif

