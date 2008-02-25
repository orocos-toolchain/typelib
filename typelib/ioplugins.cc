#include "ioplugins.hh"

#include "pluginmanager.hh"
#include "lang/tlb/import.hh"
#include "lang/tlb/export.hh"
#include "lang/cimport/import.hh"
#include "lang/idl/export.hh"

namespace
{
    typedef Typelib::GenericIOPlugin<TlbImport> TlbImportPlugin;
    typedef Typelib::GenericIOPlugin<TlbExport> TlbExportPlugin;
    typedef Typelib::GenericIOPlugin<CImport>   CImportPlugin;
    typedef Typelib::GenericIOPlugin<IDLExport> IDLExportPlugin;
}

void Typelib::registerIOPlugins(PluginManager& manager)
{
    manager.add(new TlbImportPlugin("tlb"));
    manager.add(new TlbExportPlugin("tlb"));
    manager.add(new CImportPlugin("c"));
    manager.add(new IDLExportPlugin("idl"));
}

