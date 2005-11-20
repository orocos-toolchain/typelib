#include "pluginmanager.hh"

#include "ioplugins.hh"

using namespace std;
using namespace Typelib;

namespace
{
    using namespace std;
    
    template<typename Map, typename Object>
    bool add_plugin(Map& map, Object* object)
    { return (map.insert( make_pair(object->getName(), object) ).second); }

    template<typename Object>
    Object* get_plugin( std::map<string, Object*> const& map, string const& name)
    {
        typename std::map<string, Object*>::const_iterator it = map.find(name);
        if (it == map.end())
            return 0;
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

Importer* PluginManager::importer(std::string const& name) const
{ return get_plugin(m_importers, name)->create(); }
Exporter* PluginManager::exporter(std::string const& name) const
{ return get_plugin(m_exporters, name)->create(); }



