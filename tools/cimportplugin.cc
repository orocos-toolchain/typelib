#include "cimportplugin.hh"
#include "typelib/pluginmanager.hh"
#include "typelib/importer.hh"

#include <iostream>

#include "utilmm/configfile/configset.hh"
#include "registry.hh"

using namespace std;
using namespace boost;
using utilmm::config_set;
using Typelib::Registry;
using Typelib::PluginManager;

CImportPlugin::CImportPlugin()
    : Plugin("C", "import") {}

list<string> CImportPlugin::getOptions() const
{
    static const char* arguments[] = 
    { "*:include,I=string|include search path",
      "*:define,D=string|Define this symbol" };
    return list<string>(arguments, arguments + 2);
}

bool CImportPlugin::apply(const OptionList& remaining, const config_set& options, Registry& registry)
{
    if (remaining.empty())
    {
        cerr << "No file found on command line. Aborting" << endl;
        return false;
    }
    string const file(remaining.front());

    try
    {
        auto_ptr<Typelib::Importer> importer(PluginManager::self()->importer("c"));
        if (! importer.get())
        {
            cerr << "Cannot find the import I/O plugin for C" << std::endl;
            return false;
        }
                
        importer->load(file, options, registry);
        return true;
    }
    catch(Typelib::RegistryException& e)
    { cerr << "error: " << e.toString() << endl; }
    catch(Typelib::ImportError e)
    { cerr << e.toString() << endl; }
    catch(std::exception& e)
    { cerr << "Error parsing file " << file << ":\n\t" << typeid(e).name() << endl; }
    return false;
}

