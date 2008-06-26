#ifndef TYPELIB_IMPORTER_HH
#define TYPELIB_IMPORTER_HH

#include <string>
#include "pluginmanager.hh"

namespace utilmm { class config_set; }

namespace Typelib
{
    class Registry;

    /** Base class for exception thrown during import */
    class ImportError : public std::runtime_error
    {
        std::string m_file;
        int m_line, m_column;
        std::string m_what;
        char* m_buffer;

    public:
        ImportError(const std::string& file, const std::string& what_ = "", int line = 0, int column = 0);
        ~ImportError() throw();

        void setFile(const std::string& path);
	/** The file in which the exception occured */
        std::string getFile() const;
	/** Line of error */
        int getLine() const;
	/** Column of error */
        int getColumn() const;
    };

    class ImportPlugin
    {
        std::string m_name;
        
    public:
        ImportPlugin(std::string const& name)
            : m_name(name) {}
        virtual ~ImportPlugin() {}

        std::string getName() const { return m_name; }
        virtual Importer* create() = 0;
    };

    /** Base class for import objects */
    class Importer
    {
    public:
        virtual ~Importer() {}

	/** Loads a registry from an input stream object 
	 * @arg stream	    the input stream
	 * @arg config	    the import configuration. Allowed values are importer-dependent 
	 * @arg registry    the registry the types are to be imported into
	 */
        virtual void load
            ( std::istream& stream
            , utilmm::config_set const& config
            , Registry& registry) = 0;

	/** Loads a registry from a file. The default implementation calls <tt>load(std::istream&)</tt>
	 * with the corresponding ifstream object
	 * @arg path	    the path of the file to load
	 * @arg config	    the import configuration. Allowed values are importer-dependent  
	 * @arg registry    the registry to import into
	 */
        virtual void load
            ( std::string const& path
            , utilmm::config_set const& config
            , Registry& registry);
    };
}

#endif

