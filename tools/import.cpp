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
    if (argc < 2) return false;
    string type = argv[1];
    if (string(type, 0, 8) != "--type=")
    {
        cerr << "Missing --type option" << endl;
        help(cerr);
        return false;
    }
    type = string(type, 8);

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
    options.push_back("output:output,o=string|output file");
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

    if (output.empty() && !dry)
    {
        cerr << "Expected --output=file" << endl;
        help(cerr);
        return false;
    }

    if (! plugin->apply(commandline.remaining(), config, &registry))
        return false;

    if (dry)
        registry.dump(std::cout, Registry::AllType, "*");
    else if (! registry.save(output, false))
        cerr << "Error saving registry " << output << endl;
    
    return true;    
}

void Import::help(std::ostream& out) const
{
    out << 
        "Usage: typelib register --type=<input-type> [options] input-file\n"
        "\twhere input-type is one of: ";

    list<string> plugins = getPluginNames();
    copy(plugins.begin(), plugins.end(), ostream_iterator<string>(out));
    out << endl;
}

