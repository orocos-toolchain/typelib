#include "pluginmanager.hh"
#include "plugins.hh"
#include "importer.hh"
#include "exporter.hh"

#include <boost/filesystem.hpp>
#include <dlfcn.h>

using namespace std;
using namespace Typelib;
using namespace utilmm;
using namespace boost::filesystem;

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
    // Load plugins from standard path
    if (! exists(TYPELIB_PLUGIN_PATH))
        return;
    path plugin_dir(TYPELIB_PLUGIN_PATH);

    directory_iterator end_it;
    for (directory_iterator it(plugin_dir); it != end_it; ++it)
    {
        if (it->path().extension() == ".so")
            loadPlugin(it->path().file_string());
    }
}

PluginManager::~PluginManager()
{
    clear(m_importers);
    clear(m_exporters);
    for (std::vector<TypeDefinitionPlugin*>::iterator it = m_definition_plugins.begin();
            it != m_definition_plugins.end(); ++it)
        delete *it;
    m_definition_plugins.clear();
    //for (vector<void*>::iterator it = m_library_handles.begin(); it != m_library_handles.end(); ++it)
    //    dlclose(*it);
}

bool PluginManager::loadPlugin(std::string const& path)
{
    void* libhandle = dlopen(path.c_str(), RTLD_LAZY);
    if (!libhandle)
    {
        cerr << "typelib: cannot load plugin " << path << ": " << dlerror() << endl;
        return false;
    }

    void* libentry  = dlsym(libhandle, "registerPlugins");
    if (!libentry)
    {
        cerr << "typelib: " << libentry << " does not seem to be a valid typelib plugin" << endl;
        return false;
    }

    PluginEntryPoint function = reinterpret_cast<PluginEntryPoint>(libentry);
    function(*this);
    m_library_handles.push_back(libhandle);
    return true;
}

bool PluginManager::add(ExportPlugin* plugin)
{ return add_plugin(m_exporters, plugin); }
bool PluginManager::add(ImportPlugin* plugin)
{ return add_plugin(m_importers, plugin); }
void PluginManager::add(TypeDefinitionPlugin* plugin)
{ return m_definition_plugins.push_back(plugin); }

void PluginManager::registerPluginTypes(Registry& registry)
{
    for (vector<TypeDefinitionPlugin*>::iterator it = m_definition_plugins.begin();
            it != m_definition_plugins.end(); ++it)
    {
        (*it)->registerTypes(registry);
    }
}

Importer* PluginManager::importer(std::string const& name) const
{ return get_plugin(m_importers, name)->create(); }
Exporter* PluginManager::exporter(std::string const& name) const
{ return get_plugin(m_exporters, name)->create(); }


std::string PluginManager::save(std::string const& kind, Registry const& registry)
{
    utilmm::config_set config;
    return save(kind, config, registry);
}

std::string PluginManager::save(std::string const& kind, utilmm::config_set const& config, Registry const& registry)
{
    ostringstream stream;
    save(kind, config, registry, stream);
    return stream.str();
}
void PluginManager::save(std::string const& kind, Registry const& registry, std::ostream& into)
{
    utilmm::config_set config;
    save(kind, config, registry, into);
}
void PluginManager::save(std::string const& kind, utilmm::config_set const& config, Registry const& registry, std::ostream& into)
{
    Exporter* exporter = PluginManager::self()->exporter(kind);
    exporter->save(into, config, registry);
}

Registry* PluginManager::load(std::string const& kind, std::istream& stream)
{
    utilmm::config_set config;
    return load(kind, stream, config);
}
void PluginManager::load(std::string const& kind, std::istream& stream, Registry& into )
{
    utilmm::config_set config;
    return load(kind, stream, config, into);
}
Registry* PluginManager::load(std::string const& kind, std::string const& file)
{
    utilmm::config_set config;
    return load(kind, file, config);
}
void PluginManager::load(std::string const& kind, std::string const& file, Registry& into)
{
    utilmm::config_set config;
    return load(kind, file, config, into);
}

Registry* PluginManager::load(std::string const& kind, std::istream& stream, utilmm::config_set const& config )
{
    auto_ptr<Registry> registry(new Registry);
    load(kind, stream, config, *registry.get());
    return registry.release();
}
void PluginManager::load(std::string const& kind, std::istream& stream, utilmm::config_set const& config
        , Registry& into )
{
    std::auto_ptr<Importer> importer(PluginManager::self()->importer(kind));
    importer->load(stream, config, into);
}
Registry* PluginManager::load(std::string const& kind, std::string const& file, utilmm::config_set const& config)
{
    auto_ptr<Registry> registry(new Registry);
    load(kind, file, config, *registry.get());
    return registry.release();
}
void PluginManager::load(std::string const& kind, std::string const& file, utilmm::config_set const& config
        , Registry& into)
{
    auto_ptr<Importer> importer(PluginManager::self()->importer(kind));
    importer->load(file, config, into);
}

