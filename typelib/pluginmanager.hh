#ifndef TYPELIB_PLUGINMANAGER_HH
#define TYPELIB_PLUGINMANAGER_HH

#include <map>
#include <string>
#include "utilmm/singleton/use.hh"

namespace Typelib
{
    // We should create a NamedObject class somewhere... :p
    
    class Exporter;
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

    class Importer;
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
    
    /** The plugin manager 
     * 
     * It is a singleton, using utilmm::singleton
     * You have to access it using
     * <code>
     *  utilmm::singleton::use<PluginManager> manager;
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
        bool add(ExportPlugin* plugin);
        bool add(ImportPlugin* plugin);
        
        Importer* importer(std::string const& name) const;
        Exporter* exporter(std::string const& name) const;

        typedef utilmm::singleton::use<PluginManager> self;
    private:
        friend class utilmm::singleton::wrapper<PluginManager>;
    };
}

#endif

