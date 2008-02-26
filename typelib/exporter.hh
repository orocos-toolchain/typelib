#ifndef TYPELIB_EXPORTER_HH
#define TYPELIB_EXPORTER_HH

#include <iosfwd>
#include "pluginmanager.hh"
#include "registryiterator.hh"

namespace utilmm
{
    class config_set;
}

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

    class ExportError : public std::exception { };

    /** Base class for export objects */
    class Exporter
    {
    protected:
        /** Called by save to add a preamble before saving all registry types 
	 * @see save
	 */
        virtual void begin(std::ostream& stream, Registry const& registry);
        /** Called by save to add data after saving all registry types
	 * @see save
	 */
        virtual void end  (std::ostream& stream, Registry const& registry);

    public:
        virtual ~Exporter() {}

	/** Serialize a whole registry in a file */
	virtual void save
	    ( std::string const& file_name
	    , utilmm::config_set const& config
	    , Registry const& registry );

        /** Serialize a whole registry using this exporter
         * 
         * @arg stream   the stream to write to
	 * @arg config   configuration object if per-exporter configuration is needed
         * @arg registry the registry to be saved
	 *
       	 * The default implementation calls begin(), iterates on all elements
	 * in registry, taking into account dependencies (i.e. a type will be serialized
	 * before another type which references it) and finally calls end()
	 *
	 * @exception UnsupportedType if a type cannot be serialized in this format
	 * @exception ExportError for all export errors
	 */
        virtual void save
            ( std::ostream& stream
	    , utilmm::config_set const& config
            , Registry const& registry );

	/** \overload
	 * This version of \c save is provided for backward-compatibility only
	 *
	 * @return true if the export has succeeded, false otherwise
	 */
        virtual bool save
            ( std::ostream& stream
            , Registry const& registry );

        /** Serialize one type in \c stream. It is called by Registry::save(ostream&, Registry const&) 
	 * @arg stream	the stream to write to
	 * @arg type	the type to be serialized */
        virtual void save
            ( std::ostream& stream
            , RegistryIterator const& type ) = 0;
    };
}

#endif

