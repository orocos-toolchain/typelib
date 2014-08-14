#include "containers.hh"
#include <typelib/pluginmanager.hh>

extern "C" void registerPlugins(Typelib::PluginManager& manager)
{
    Typelib::Container::registerContainer("/std/vector", Vector::factory);
    Typelib::Container::registerContainer("/std/basic_string", String::factory);
}

