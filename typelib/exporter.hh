#ifndef TYPELIB_EXPORTER_HH
#define TYPELIB_EXPORTER_HH

#include <iosfwd>
#include "registryiterator.hh"

namespace Typelib
{
    class Type;
    class Registry;
    class Exporter
    {
    protected:
        /** Called by save to add a prelude before saving all registry types */
        virtual bool begin(std::ostream& stream, Registry const& registry);
        /** Called by save to add data after saving all registry types */
        virtual bool end  (std::ostream& stream, Registry const& registry);

    public:
        virtual ~Exporter() {}

        /** Serialize a whole registry
         * It first calls begin, then save for all types in \c registry, and
         * finally end
         * 
         * @arg stream   the stream to write to
         * @arg registry the registry to be saved
         */
        virtual bool save
            ( std::ostream& stream
            , Registry const& registry );

        /** Serialize one type in \c stream. If recursive is true, it saves
         * also all types \c type references */
        virtual bool save
            ( std::ostream& stream
            , RegistryIterator const& type ) = 0;
    };
}

#endif

