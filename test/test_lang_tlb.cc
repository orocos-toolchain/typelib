#include <boost/test/auto_unit_test.hpp>

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
    }
}

