#include "import.hh"

#include "plugin.hh"

#include "typelib/registry.hh"
#include "typelib/registryiterator.hh"
#include "typelib/pluginmanager.hh"
#include "typelib/exporter.hh"
#include "typelib/importer.hh"

#include "utilmm/configfile/commandline.hh"
using utilmm::command_line;
#include "utilmm/configfile/configset.hh"
using utilmm::config_set;

#include "cimportplugin.hh"
#include "tlbimportplugin.hh"

#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <list>

#include <boost/filesystem/operations.hpp>

using namespace std;
using namespace Typelib;

Import::Import()
    : Mode("import") 
{ 
    addPlugin( new CImportPlugin );
    addPlugin( new TlbImportPlugin );
}

bool Import::apply(int argc, char* const argv[])
{
    if (argc < 2) 
    {
        help(cerr);
        return false;
    }
    string type = argv[0];
    
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
    options.push_back(":output=string|Output file");
    command_line commandline(options);
    if (!commandline.parse(argc - 1, argv + 1, config))
        return false;

    std::string nspace = config.get<string>("nspace", "/");
    if (!registry.setDefaultNamespace( nspace ))
    {
        cerr << "Invalid namespace option " << nspace << endl;
        return false;
    }

    list<string> remaining = commandline.remaining();
    if (remaining.size() > 2)
    {
        cerr << "More than one file specified on command line" << endl;
        help(cerr);
        return false;
    }

    string base_tlb;
    if (remaining.size() == 2)
        base_tlb = remaining.back();

    string output_tlb = config.get<string>("output", base_tlb);
    if (output_tlb.empty())
        output_tlb = "-";

    // Load the base_tlb if it exists
    if (! base_tlb.empty() && boost::filesystem::exists(base_tlb))
    {
        auto_ptr<Importer> read_db(PluginManager::self()->importer("tlb"));
        if (! read_db->load(base_tlb, config, registry))
        { 
            cerr << "Error loading registry " << base_tlb << endl; 
            return false;
        }
    }

    if (! plugin->apply(remaining, config, registry))
        return false;

    // Get the output stream object. It is either an ofstream on output_tlb,
    // or cout if --output=- was provided
    std::auto_ptr<ofstream> filestream; // if the output is a file
    ostream* outstream = 0;
    if (output_tlb == "-")
        outstream = &cout;
    else
    {
        filestream.reset( new ofstream(output_tlb.c_str()) );
        if (! filestream->is_open())
        {
            cerr << "Cannot open " << output_tlb << " for writing" << endl;
            return false;
        }
        outstream = filestream.get();
    }

    try
    {
        auto_ptr<Exporter> exporter(PluginManager::self()->exporter("tlb"));
        exporter->save(*outstream, registry);
    }
    catch(...)
    {
        cerr << "Error when writing the type data base " << output_tlb << endl;
        return false;
    }
    
    return true;    
}

void Import::help(std::ostream& out) const
{
    out << 
        "Usage: typelib import <input-type> [options] input-file [base]\n"
        "\n"
        "Imports the input-file contents, of type <input-type>, into base (if provided)\n"
        "and saves it back into either base or the file specified by --output\n"
        "If neither base nor --output is provided, print on standard output\n"
        "\tSupported input types are: ";

    list<string> plugins = getPluginNames();
    copy(plugins.begin(), plugins.end(), ostream_iterator<string>(out, " "));
    out << endl;

    out << 
        "and allowed options are :\n"
        "\t--nspace=NAME           namespace to import new types into\n"
        "\t--output=FILE           save the resulting database in FILE instead of using base\n"
        "\t                        use --output=- to get the result on standard output\n"
        "\t--dry-run               only display new types, do not save them" << endl;
}

