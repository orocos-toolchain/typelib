#ifndef TYPELIB_PLUGINMANAGER_HH
#define TYPELIB_PLUGINMANAGER_HH

#include <map>
#include <string>
#include <utilmm/singleton/use.hh>
#include <utilmm/configfile/configset.hh>

namespace Typelib
{
    class Registry;
    
    class Exporter;
    class ExportPlugin;

    /** Base class for exception thrown during import */
    class ImportError : public std::exception
    {
        std::string m_file;
        int m_line, m_column;
        std::string m_what;
        char* m_buffer;

    public:
        ImportError(const std::string& file, const std::string& what_ = "", int line = 0, int column = 0);
        virtual ~ImportError() throw();

        void setFile(const std::string& path);
        std::string getFile() const;
        int getLine() const;
        int getColumn() const;

        virtual char const* what() throw();
        virtual std::string toString () const;
    };

    class Importer;
    class ImportPlugin;
    
    /** Exception thrown when an unknown plugin is found */
    class PluginNotFound : public std::exception {};

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

	/** Converts a registry into string form
	 * @arg kind	the output format. It has to be a valid exporter name
	 * @arg registry the registry to export
	 * @throws PluginNotFound if \c kind is invalid
	 * @throws ExportError if an error occured during the export
	 */
        static std::string save(std::string const& kind, Registry const& registry);
       	/** Exports a registry to an ostream object
	 * @arg kind	    the output format. It has to be a valid exporter name
	 * @arg registry    the registry to export
	 * @arg into	    the ostream object to export to
	 * @throws PluginNotFound if \c kind is invalid
	 * @throws ExportError if an error occured during the export
	 */
	static void save(std::string const& kind, Registry const& registry, std::ostream& into);

	/** Creates a registry from a istream object
	 * @see Importer::load
	 */
        static Registry* load
            ( std::string const& kind
            , std::istream& stream
            , utilmm::config_set const& config );
	
       	/** Imports types from a istream object to an already existing registry
	 * @see Importer::load
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

