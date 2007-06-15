#include <boost/test/auto_unit_test.hpp>

#include "testsuite.hh"
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include "test_cimport.1"
using namespace Typelib;

BOOST_AUTO_TEST_CASE( test_c_import )
{
    Registry registry;
    static const char* test_file = TEST_DATA_PATH("test_cimport.h");

    PluginManager::self manager;
    Importer* importer = manager->importer("c");

    {
	Registry temp_registry;
	utilmm::config_set config;
	config.set("include", "../");
	config.set("define", "GOOD");
	BOOST_CHECK_NO_THROW( importer->load("test_cimport.h", config, temp_registry) );
    }
    {
	Registry temp_registry;
	utilmm::config_set config;
	config.insert("rawflag", "-I../");
	config.insert("rawflag", "-DGOOD");
	BOOST_CHECK_NO_THROW( importer->load("test_cimport.h", config, temp_registry) );
    }

    utilmm::config_set config;
    // Load the file in registry
    BOOST_CHECK_THROW( importer->load("does_not_exist", config, registry), ImportError);
    BOOST_CHECK_THROW( importer->load("test_cimport.h", config, registry), ImportError );
    config.set("include", "../");
    config.set("define", "GOOD");
    BOOST_REQUIRE_NO_THROW( importer->load(test_file, config, registry) );

    // Check that the types are defined
    BOOST_CHECK( registry.has("/struct A") );
    BOOST_CHECK( registry.has("/struct B") );
    BOOST_CHECK( registry.has("/ADef") );
    BOOST_CHECK( registry.has("/B") );
    BOOST_CHECK( registry.has("/OpaqueType") );

    // Check that the size of B.a is the same as A
    Compound const* b   = static_cast<Compound const*>(registry.get("/B"));
    Field  const* b_a = b->getField("a");
    BOOST_CHECK_EQUAL( &(b_a->getType()), registry.get("/ADef") );

    // Check the type of c (array of floats)
    Field  const* b_c = b->getField("c");
    BOOST_CHECK_EQUAL( &(b_c->getType()), registry.get("/float[100]") );
    BOOST_CHECK_EQUAL( b_c->getType().getCategory(), Type::Array );

    // Check the sizes for d, e, f
    Field const* b_d = b->getField("d");
    BOOST_CHECK_EQUAL( &(b_d->getType()), registry.get("/float[1]") );
    Field const* b_e = b->getField("e");
    BOOST_CHECK_EQUAL( &(b_e->getType()), registry.get("/float[1]") );
    Field const* b_f = b->getField("f");
    BOOST_CHECK_EQUAL( &(b_f->getType()), registry.get("/float[3]") );
    Field const* b_g = b->getField("g");
    BOOST_CHECK_EQUAL( &(b_g->getType()), registry.get("/float[2]") );
    Field const* b_h = b->getField("h");
    BOOST_CHECK_EQUAL( &(b_h->getType()), registry.get("/struct A[4]") );
    Field const* b_i = b->getField("i");
    // order of indexes of multi-dimensional arrays are reverse than the 
    // ones in C because we always read dimensions from right to left
    // (i.e. b.i is supposed to be a (array of 10 elements of (array of 
    // 20 elements of floats))
    BOOST_CHECK_EQUAL( &(b_i->getType()), registry.get("/float[20][10]") );

    // Check the array indirection
    Array const& b_c_array(dynamic_cast<Array const&>(b_c->getType()));
    BOOST_CHECK_EQUAL( &b_c_array.getIndirection(), registry.get("/float") );

    // Check the sizes
    BOOST_CHECK_EQUAL( registry.get("/struct A")->getSize(), sizeof(A) );
    BOOST_CHECK_EQUAL( b->getSize(), sizeof(B) );
    BOOST_CHECK_EQUAL( b_c_array.getDimension(), 100UL );

    // Test the forms of DEFINE_STR and DEFINE_ID
    BOOST_CHECK( registry.has("/DEFINE_STR") );
    BOOST_CHECK( registry.has("/DEFINE_ID") );
    BOOST_CHECK_EQUAL(Type::Compound, registry.get("/DEFINE_STR")->getCategory());
    BOOST_CHECK_EQUAL(Type::Pointer, registry.get("/DEFINE_ID")->getCategory());
    BOOST_CHECK( *registry.get("/DEFINE_STR") == static_cast<Pointer const*>(registry.get("/DEFINE_ID"))->getIndirection());

    // Check the enum behaviour
    BOOST_CHECK( registry.has("/E") );
    BOOST_CHECK_EQUAL(registry.get("/E")->getCategory(), Type::Enum);
    Enum const& e(dynamic_cast<Enum const&>(*registry.get("/E")));
    Enum::ValueMap const& map = e.values();

    // Check that the values in Enum are the ones we are expecting
    struct ExpectedValue
    {
	char const* name;
	int         value;
    };
    ExpectedValue expected[] = {
	{ "E_FIRST",  E_FIRST },
	{ "E_SECOND", E_SECOND },
	{ "E_SET",    E_SET },
	{ "E_PARENS", E_PARENS },
	{ "E_HEX",    E_HEX },
	{ "E_OCT",    E_OCT },
	{ "LAST", LAST },
	{ 0, 0 }
    };
	
    for (ExpectedValue* exp_it = expected; exp_it->name; ++exp_it)
    {
	Enum::ValueMap::const_iterator it = map.find(exp_it->name);
	BOOST_REQUIRE( it != map.end() );
	BOOST_CHECK_EQUAL(exp_it->value, it->second);
    }
}

