#include "import.hh"
#include "containers.hh"
#include <typelib/plugins.hh>
#include <typelib/registry.hh>

namespace
{
    struct TypeRegistrar : public Typelib::TypeDefinitionPlugin
    {
        void registerTypes(Typelib::Registry& registry)
        {
            registry.add(new String(registry));
        }
    };
}

extern "C" void registerPlugins(Typelib::PluginManager& manager)
{
    Typelib::Container::registerContainer("/std/vector", Vector::factory);
    Typelib::Container::registerContainer("/std/string", String::factory);
    manager.add(new Typelib::GenericIOPlugin<CImport>("c"));
    // manager.add(new TypeRegistrar());
}

