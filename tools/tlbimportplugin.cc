#include "tlbimportplugin.hh"
#include "lang/tlb/import.hh"

#include <iostream>

#include "utilmm/configfile/configset.hh"
#include "registry.hh"

using namespace std;
using namespace boost;
using utilmm::config_set;
using Typelib::Registry;

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
        TlbImport importer;
        importer.load(file, options, registry);

        return true;
    }
    catch(Typelib::RegistryException& e)
    {
        cerr << "Error in type management: " << e.toString() << endl;
        return false;
    }
    catch(std::exception& e)
    {
        cerr << "Error parsing file " << file << ":\n\t"
            << typeid(e).name() << endl;
        return false;
    }
}

