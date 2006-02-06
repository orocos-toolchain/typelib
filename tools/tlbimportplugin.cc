#include "tlbimportplugin.hh"

#include "typelib/importer.hh"
#include "typelib/registry.hh"
#include "typelib/pluginmanager.hh"

#include "utilmm/configfile/configset.hh"
#include <iostream>

using namespace std;
using namespace boost;
using utilmm::config_set;
using Typelib::Registry;
using Typelib::PluginManager;
using Typelib::Importer;

TlbImportPlugin::TlbImportPlugin()
    : Plugin("tlb", "import") {}

bool TlbImportPlugin::apply(const OptionList& remaining, const config_set& options, Registry& registry)
{
    if (remaining.empty())
    {
        cerr << "No file found on command line. Aborting" << endl;
        return false;
    }
    string const file(remaining.front());

    try
    {
        auto_ptr<Importer> importer(PluginManager::self()->importer("tlb"));
        if (! importer.get())
        {
            cerr << "Cannot find an I/O plugin for tlb" << endl;
            return false;
        }

        importer->load(file, options, registry);
        return true;
    }
    catch(Typelib::RegistryException& e)
    { cerr << "Error in type management: " << e.toString() << endl; }
    catch(Typelib::ImportError e)
    { cerr << e.toString() << endl; }
    catch(std::exception& e)
    { cerr << "Error parsing file " << file << ":\n\t" << typeid(e).name() << endl; }
    return false;
}

