#include "testsuite.hh"
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
using namespace Typelib;

class TC_CImport
{
public:
    TC_CImport() { }
    ~TC_CImport() { }

    void test_import()
    {
        Registry registry;
        static const char* test_file = TEST_DATA_PATH("test_cimport.1");
        
        // Load the file in registry
        PluginManager::self manager;
        Importer* importer = manager->importer("c");
        utilmm::config_set config;
        BOOST_REQUIRE( !importer->load("does_not_exist", config, registry) );
        BOOST_REQUIRE( importer->load(test_file, config, registry) );

        // Check that the types are defined
        BOOST_REQUIRE( registry.has("/struct A") );
        BOOST_REQUIRE( registry.has("/struct B") );
        BOOST_REQUIRE( registry.has("/ADef") );
        BOOST_REQUIRE( registry.has("/B") );

        // Check that the size of B.a is the same as A
        Compound const* B   = static_cast<Compound const*>(registry.get("/B"));
        Field  const* B_a = B->getField("a");
        BOOST_REQUIRE(B);
        BOOST_REQUIRE_EQUAL( &(B_a->getType()), registry.get("/ADef") );
    }
};

void test_lang_c(test_suite* ts) {
    boost::shared_ptr<TC_CImport> instance( new TC_CImport );
    ts->add( BOOST_CLASS_TEST_CASE( &TC_CImport::test_import, instance ) );
}

