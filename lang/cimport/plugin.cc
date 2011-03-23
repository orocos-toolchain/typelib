#include "import.hh"
#include <typelib/plugins.hh>
#include <typelib/registry.hh>

extern "C" void registerPlugins(Typelib::PluginManager& manager)
{
    manager.add(new Typelib::GenericIOPlugin<CImport>("c"));
}

