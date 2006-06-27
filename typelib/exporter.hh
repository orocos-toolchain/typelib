#ifndef TYPELIB_EXPORTER_HH
#define TYPELIB_EXPORTER_HH

#include <iosfwd>
#include "pluginmanager.hh"
#include "registryiterator.hh"

namespace Typelib
{
    class Type;
    class Registry;

    class ExportPlugin
    {
        std::string m_name;

    public:
        ExportPlugin(std::string const& name)
            : m_name(name) {}
        virtual ~ExportPlugin() {}

        std::string getName() const { return m_name; }
        virtual Exporter* create() = 0;
    };

    /** Base class for export objects */
    class Exporter
    {
    protected:
        /** Called by save to add a preamble before saving all registry types 
	 * @see save
	 */
        virtual bool begin(std::ostream& stream, Registry const& registry);
        /** Called by save to add data after saving all registry types
	 * @see save
	 */
        virtual bool end  (std::ostream& stream, Registry const& registry);

    public:
        virtual ~Exporter() {}

        /** Serialize a whole registry using this exporter
         * 
         * @arg stream   the stream to write to
         * @arg registry the registry to be saved
	 *
       	 * The default implementation does
	 * <code>
	 *   begin(stream, registry)
	 *   save(ostream, iterator) for each element in registry
	 *   end(stream, registry)
	 * </code>
	 */
        virtual bool save
            ( std::ostream& stream
            , Registry const& registry );

        /** Serialize one type in \c stream. It is called by Registry::save(ostream&, Registry const&) 
	 * @arg stream	the stream to write to
	 * @arg type	the type to be serialized */
        virtual bool save
            ( std::ostream& stream
            , RegistryIterator const& type ) = 0;
    };
}

#endif

