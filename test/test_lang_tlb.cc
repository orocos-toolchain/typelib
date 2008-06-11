#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
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

BOOST_AUTO_TEST_CASE( test_tlb_import_export )
{
    PluginManager::self manager;
    
    // Load the C file into a registry. Dump it as a tlb and reload it, then
    // compare the result.
    Registry registry;
    {
        static const char* test_file = TEST_DATA_PATH("test_cimport.h");
        utilmm::config_set config;
	config.set("include", TEST_DATA_PATH(".."));
	config.set("define", "GOOD");
	manager->load("c", test_file, config, registry);
    }

    utilmm::config_set config;
    BOOST_REQUIRE_THROW(manager->save("tlb", registry), ExportError);

    // istringstream io(result);
    // Registry* reloaded = manager->load("tlb", io, config);
    // std::string new_result = manager->save("tlb", *reloaded);

    // BOOST_REQUIRE_EQUAL(result, new_result);
    // BOOST_REQUIRE(registry.isSame(*reloaded));
}

BOOST_AUTO_TEST_CASE( test_tlb_import )
{
    PluginManager::self manager;
    Importer* importer = manager->importer("tlb");
    utilmm::config_set config;

    {
	string empty_tlb = "<?xml version=\"1.0\"?>\n<typelib>\n</typelib>";
	istringstream stream(empty_tlb);
	Registry registry;
	BOOST_CHECK_NO_THROW( importer->load(stream, config, registry); );
    }

    { 
	ifstream file(TEST_DATA_PATH("rflex.tlb"));
	Registry registry;
	BOOST_CHECK_NO_THROW( importer->load(file, config, registry); );

	BOOST_CHECK( registry.get("/custom_null") );
	BOOST_CHECK( registry.get("/custom_null")->isNull() );
    }
}

