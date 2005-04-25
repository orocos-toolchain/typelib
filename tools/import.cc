#include "import.hh"

#include "plugin.hh"

#include "registry.hh"
#include "utilmm/configfile/commandline.hh"
using utilmm::command_line;
#include "utilmm/configfile/configset.hh"
using utilmm::config_set;

#include "cimportplugin.hh"

#include <algorithm>
#include <iterator>
#include <iostream>

using namespace std;
using Typelib::Registry;

Import::Import()
    : Mode("import") 
{ 
    addPlugin( new CImportPlugin );
}

bool Import::apply(int argc, char* const argv[])
{
    if (argc < 2) 
    {
        help(cerr);
        return false;
    }
    string type = argv[1];
    
    Plugin* plugin = getPlugin(type);
    if (! plugin)
    {
        cerr << "Cannot handle input type " << type << endl;
        return false;
    }

    Registry  registry;
    config_set config;

    std::list<string> options = plugin->getOptions();
    options.push_back(":nspace=string|Namespace to import types into");
    options.push_back("dry:dry-run|Do not save the registry, just display the imported types");
    command_line commandline(options);
    if (!commandline.parse(argc - 1, argv + 1, &config))
        return false;

    std::string nspace = config.get_string("nspace", "/");
    if (!registry.setDefaultNamespace( nspace ))
    {
        cerr << "Invalid namespace option " << nspace << endl;
        return false;
    }

    std::string output = config.get_string("output", "");
    bool dry = config.get_bool("dry", false);

    list<string> remaining = commandline.remaining();
    if (remaining.size() > 2)
    {
        cerr << "More than one file specified on command line" << endl;
        help(cerr);
        return false;
    }
    else if (remaining.size() < 2 && !dry)
    {
        cerr << "Only one file specified on command line" << endl;
        help(cerr);
        return false;
    }

    if (! plugin->apply(remaining, config, registry))
        return false;

    if (dry)
        registry.dump(std::cout, Registry::AllType, "*");
    else if (! registry.save(remaining.back(), false))
        cerr << "Error saving registry " << output << endl;
    
    return true;    
}

void Import::help(std::ostream& out) const
{
    out << 
        "Usage: typelib import <input-type> [options] input-file output-file\n"
        "\twhere input-type is one of: ";

    list<string> plugins = getPluginNames();
    copy(plugins.begin(), plugins.end(), ostream_iterator<string>(out));
    out << endl;

    out << 
        "and allowed options are :\n"
        "\t--nspace=NAME           namespace to import new types into\n"
        "\t--dry-run               only display new types, do not save them" << endl;
}

