#include "pluginmanager.hh"
#include "ioplugins.hh"
#include "importer.hh"
#include "exporter.hh"

using namespace std;
using namespace Typelib;
using namespace utilmm;

namespace
{
    using namespace std;
    
    template<typename Map, typename Object>
    bool add_plugin(Map& plugin_map, Object* object)
    { return (plugin_map.insert( make_pair(object->getName(), object) ).second); }

    template<typename Object>
    Object* get_plugin( map<string, Object*> const& plugin_map, string const& name)
    {
        typename map<string, Object*>::const_iterator it = plugin_map.find(name);
        if (it == plugin_map.end())
            throw PluginNotFound();
        return it->second;
    }

    template<typename Container>
    void clear(Container& container)
    {
        for (typename Container::iterator it = container.begin(); it != container.end(); ++it)
            delete it->second;
        container.clear();
    }
}

PluginManager::PluginManager()
{
    registerIOPlugins(*this);
}

PluginManager::~PluginManager()
{
    clear(m_importers);
    clear(m_exporters);
}

bool PluginManager::add(ExportPlugin* plugin)
{ return add_plugin(m_exporters, plugin); }
bool PluginManager::add(ImportPlugin* plugin)
{ return add_plugin(m_importers, plugin); }

Importer* PluginManager::importer(string const& name) const
{ return get_plugin(m_importers, name)->create(); }
Exporter* PluginManager::exporter(string const& name) const
{ return get_plugin(m_exporters, name)->create(); }


string PluginManager::save(string const& kind, Registry const& registry)
{
    ostringstream stream;
    save(kind, registry, stream);
    return stream.str();
}
void PluginManager::save(string const& kind, Registry const& registry, ostream& into)
{
    Exporter* exporter = PluginManager::self()->exporter(kind);
    exporter->save(into, registry);
}

Registry* PluginManager::load(string const& kind, istream& stream, config_set const& config )
{
    auto_ptr<Registry> registry(new Registry);
    load(kind, stream, config, *registry.get());
    return registry.release();
}
void PluginManager::load(string const& kind, istream& stream, config_set const& config
        , Registry& into )
{
    Importer* importer = PluginManager::self()->importer(kind);
    importer->load(stream, config, into);
}
Registry* PluginManager::load(string const& kind, string const& file, config_set const& config)
{
    auto_ptr<Registry> registry(new Registry);
    load(kind, file, config, *registry.get());
    return registry.release();
}
void PluginManager::load(string const& kind, string const& file, config_set const& config
        , Registry& into)
{
    Importer* importer = PluginManager::self()->importer(kind);
    importer->load(file, config, into);
}

