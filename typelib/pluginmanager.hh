#ifndef TYPELIB_PLUGINMANAGER_HH
#define TYPELIB_PLUGINMANAGER_HH

#include <map>
#include <string>
#include <vector>
#include <utilmm/singleton/use.hh>
#include <utilmm/configfile/configset.hh>
#include <stdexcept>

namespace Typelib
{
    class Registry;
    
    class Exporter;
    class ExportPlugin;

    class Importer;
    class ImportPlugin;
    
    /** Exception thrown when an unknown plugin is found */
    struct PluginNotFound : std::runtime_error
    {
        PluginNotFound() : std::runtime_error("plugin not found") { }
    };

    /** Generic error for problems during export */
    struct ExportError : std::runtime_error
    { 
        ExportError(std::string const& msg) : std::runtime_error(msg) {}
    };

    /** The plugin manager 
     * 
     * It is a singleton, using utilmm::singleton
     * You have to access it using
     * <code>
     *  PluginManager::self manager;
     *
     *  manager->importer()
     * </code>
     *
     * The object is destroyed when the last of the use<> objects
     * is, and created back when a new use<> object is built.
     */
    class PluginManager
    {
        std::map<std::string, ExportPlugin*> m_exporters;
        std::map<std::string, ImportPlugin*> m_importers;
        std::vector<void*> m_library_handles;
        bool loadPlugin(std::string const& path);

        typedef void (*PluginEntryPoint)(PluginManager&);

        PluginManager();
        ~PluginManager();
        
    public:
	/** Registers a new exporter */
        bool add(ExportPlugin* plugin);

	/** Build a new import plugin from its plugin name
	 * @throws PluginNotFound */
        Importer* importer(std::string const& name) const;

	/** Registers a new importer */
        bool add(ImportPlugin* plugin);

	/** Build a new export plugin from its plugin name 
	 * @throws PluginNotFound */
        Exporter* exporter(std::string const& name) const;

	/** \overload
	 * This is provided for backward compatibility only
	 */
        static std::string save
	    ( std::string const& kind
	    , Registry const& registry);

	/** \overload
	 */
        static std::string save
	    ( std::string const& kind
	    , utilmm::config_set const& config
	    , Registry const& registry);

       	/** \overload
	 * This is provided for backward compatibility only
	 */
	static void save
	    ( std::string const& kind
	    , Registry const& registry
	    , std::ostream& into);

       	/** Exports a registry to an ostream object
	 * @arg kind	    the output format. It has to be a valid exporter name
	 * @arg config      format-specific configuration. See each exporter documentation for details.
	 * @arg registry    the registry to export
	 * @arg into	    the ostream object to export to
	 * @throws PluginNotFound if \c kind is invalid
	 * @throws UnsupportedType if a specific type cannot be exported into this format
	 * @throws ExportError if another error occured during the export
	 */
	static void save
	    ( std::string const& kind
	    , utilmm::config_set const& config
	    , Registry const& registry
	    , std::ostream& into);

	/** \overload
	 */
        static Registry* load
            ( std::string const& kind
            , std::istream& stream );
	
       	/** Imports types from a istream object to an already existing registry
	 */
	static void load
            ( std::string const& kind
            , std::istream& stream
            , Registry& into );

       	/** Creates a registry from a file
	 * @see Importer::load
	 */
        static Registry* load
            ( std::string const& kind
            , std::string const& file );

       	/** Imports types from a file into an already existing registry
	 * @see Importer::load
	 */
        static void load
            ( std::string const& kind
            , std::string const& file
            , Registry& into );


	/** \overload
	 */
        static Registry* load
            ( std::string const& kind
            , std::istream& stream
            , utilmm::config_set const& config );
	
       	/** Imports types from a istream object to an already existing registry
	 */
	static void load
            ( std::string const& kind
            , std::istream& stream
            , utilmm::config_set const& config
            , Registry& into );

       	/** Creates a registry from a file
	 * @see Importer::load
	 */
        static Registry* load
            ( std::string const& kind
            , std::string const& file
            , utilmm::config_set const& config );

       	/** Imports types from a file into an already existing registry
	 * @see Importer::load
	 */
        static void load
            ( std::string const& kind
            , std::string const& file
            , utilmm::config_set const& config
            , Registry& into );

	/** The one PluginManager object. See main PluginManager documentation
	 * for its use.
	 */
        typedef utilmm::singleton::use<PluginManager> self;

    private:
        friend class utilmm::singleton::wrapper<PluginManager>;
    };
}

#endif

