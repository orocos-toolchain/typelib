#ifndef TYPELIB_IOPLUGINS_HH
#define TYPELIB_IOPLUGINS_HH

#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/mpl/if.hpp>

namespace Typelib
{
    class ExportPlugin;
    class ImportPlugin;
    class Exporter;
    class Importer;

    template<typename Type>
    struct plugin_traits
    {
        typedef typename boost::mpl::if_
            < boost::is_base_and_derived<Exporter, Type>
            , ExportPlugin
            , ImportPlugin >::type   plugin_base;

        typedef typename boost::mpl::if_
            < boost::is_base_and_derived<Exporter, Type>
            , Exporter
            , Importer >::type   object_base;
    };

    template<typename Type>
    class GenericIOPlugin 
        : public plugin_traits<Type>::plugin_base
    {
    public:
        GenericIOPlugin(char const* name)
            : plugin_traits<Type>::plugin_base(name) {}
        typename plugin_traits<Type>::object_base* create()
        { return new Type; }
    };
}

#define TYPELIB_REGISTER_IO2(name, klass1, klass2) extern "C" void registerPlugins(Typelib::PluginManager& manager) {\
    manager.add(new Typelib::GenericIOPlugin<klass1>(#name)); \
    manager.add(new Typelib::GenericIOPlugin<klass2>(#name)); \
}

#define TYPELIB_REGISTER_IO1(name, klass1) extern "C" void registerPlugins(Typelib::PluginManager& manager) {\
    manager.add(new Typelib::GenericIOPlugin<klass1>(#name)); \
}

#endif

