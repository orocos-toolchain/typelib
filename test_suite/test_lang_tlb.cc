#include "testsuite.hh"
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <iostream>
#include <sstream>
#include <fstream>
using namespace Typelib;
using namespace std;

class TC_TlbImport
{
public:
    TC_TlbImport() { }
    ~TC_TlbImport() { }

    void test_import()
    {
        PluginManager::self manager;
        Importer* importer = manager->importer("tlb");
        utilmm::config_set config;

        {
            string empty_tlb = "<?xml version=\"1.0\"?>\n<typelib>\n</typelib>";
            istringstream stream(empty_tlb);
            Registry registry;
            BOOST_REQUIRE_NO_THROW( importer->load(stream, config, registry); );
        }

        { 
            ifstream file(TEST_DATA_PATH("rflex.tlb"));
            Registry registry;
            BOOST_REQUIRE_NO_THROW( importer->load(file, config, registry); );
        }
    }
};

void test_lang_tlb(test_suite* ts) {
    boost::shared_ptr<TC_TlbImport> instance( new TC_TlbImport );
    ts->add( BOOST_CLASS_TEST_CASE( &TC_TlbImport::test_import, instance ) );
}

