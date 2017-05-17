#include <boost/test/auto_unit_test.hpp>

#include <lang/csupport/standard_types.hh>

#include <test/testsuite.hh>
#include <typelib/utilmm/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/registryiterator.hh>
#include <typelib/typedisplay.hh>
#include <iostream>
#include <sstream>
#include <fstream>
using namespace Typelib;
using namespace std;

#ifdef HAS_INTERNAL_CPARSER
BOOST_AUTO_TEST_CASE( test_tlb_rejects_recursive_types )
{
    PluginManager &manager(PluginManager::getInstance());
    Registry registry;

    static const char* test_file = TEST_DATA_PATH("test_cimport.1");
    utilmm::config_set config;
    manager.load("c", test_file, config, registry);

    BOOST_REQUIRE_THROW(manager.save("tlb", registry), ExportError);
}
#endif

BOOST_AUTO_TEST_CASE( test_tlb_idempotent )
{
    PluginManager &manager(PluginManager::getInstance());
    Registry registry;

    static const char* test_file = TEST_DATA_PATH("test_cimport.tlb");
    {
        utilmm::config_set config;
        manager.load("tlb", test_file, config, registry);
    }

    string result;
    result = manager.save("tlb", registry);
    istringstream io(result);
    utilmm::config_set config;
    Registry* reloaded = NULL;
    reloaded = manager.load("tlb", io, config);

    if (!registry.isSame(*reloaded))
    {
        RegistryIterator it = registry.begin(),
                         end = registry.end();
        for (; it != end; ++it)
        {
            if (!reloaded->has(it->getName(), true))
                std::cerr << "reloaded does not have " << it->getName() << std::endl;
            else if (!(*it).isSame(*reloaded->build(it->getName())))
            {
                std::cerr << "versions of " << it->getName() << " differ" << std::endl;
                std::cerr << *it << std::endl << std::endl;
                std::cerr << *reloaded->get(it->getName()) << std::endl;
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( test_tlb_import )
{
    PluginManager &manager(PluginManager::getInstance());
    auto_ptr<Importer> importer(manager.importer("tlb"));
    utilmm::config_set config;

    {
	string empty_tlb = "<?xml version=\"1.0\"?>\n<typelib>\n</typelib>";
	istringstream stream(empty_tlb);
	Registry registry;
	importer->load(stream, config, registry);
    }

    {
	string invalid_element_type = "<?xml version=\"1.0\"?>\n<typelib>\n<invalid_thing name=\"fake\"/></typelib>";
	istringstream stream(invalid_element_type);
	Registry registry;
	BOOST_CHECK_THROW(importer->load(stream, config, registry), std::runtime_error);
    }
    {
	string missing_arg = "<?xml version=\"1.0\"?>\n<typelib>\n<invalid_thing /></typelib>";
	istringstream stream(missing_arg);
	Registry registry;
	BOOST_CHECK_THROW(importer->load(stream, config, registry), std::runtime_error);
    }


    { 
	ifstream file(TEST_DATA_PATH("rflex.tlb"));
	Registry registry;
        Typelib::CXX::addStandardTypes(registry);
	importer->load(file, config, registry);

	BOOST_CHECK( registry.get("/custom_null") );
	BOOST_CHECK( registry.get("/custom_null")->isNull() );
    }
}

