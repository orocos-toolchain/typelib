#include "ioplugins.hh"

#include "pluginmanager.hh"
#include "lang/tlb/import.hh"
#include "lang/tlb/export.hh"
#include "lang/cimport/import.hh"

namespace
{
    typedef Typelib::GenericIOPlugin<TlbImport> TlbImportPlugin;
    typedef Typelib::GenericIOPlugin<TlbExport> TlbExportPlugin;
    typedef Typelib::GenericIOPlugin<CImport>   CImportPlugin;
}

void Typelib::registerIOPlugins(PluginManager& manager)
{
    manager.add(new TlbImportPlugin("tlb"));
    manager.add(new TlbExportPlugin("tlb"));
    manager.add(new CImportPlugin("c"));
}

