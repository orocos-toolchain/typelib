#include "import.h"

#include "plugin.h"

#include "registry.h"
#include "commandline.h"
#include "configset.h"

#include <algorithm>
#include <iterator>
#include <iostream>

#include "genom.h"

using namespace std;
using namespace Utils;
using Typelib::Registry;

Import::Import()
    : Mode("import") 
{ 
    addPlugin(new GenomPlugin);
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
    ConfigSet config;

    std::list<string> options = plugin->getOptions();
    options.push_back(":nspace=string|Namespace to import types into");
    options.push_back("dry:dry-run|Do not save the registry, just display the imported types");
    Commandline commandline(options);
    if (!commandline.parse(argc - 1, argv + 1, &config))
        return false;

    std::string nspace = config.getString("nspace", "/");
    if (!registry.setDefaultNamespace( nspace ))
    {
        cerr << "Invalid namespace option " << nspace << endl;
        return false;
    }

    std::string output = config.getString("output", "");
    bool dry = config.getBool("dry", false);

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

    if (! plugin->apply(remaining, config, &registry))
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

