#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include <typelib/memory_layout.hh>

#include <test/test_cimport.1>
#include <string.h>

using namespace Typelib;
using namespace std;

BOOST_AUTO_TEST_CASE( test_layout_simple )
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.1"), config, registry) );

    /* Check a simple structure with alignment issues */
    {
        Type const& type = *registry.get("/struct A");
        MemoryLayout ops = Typelib::layout_of(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(sizeof(A), ops[1]);
    }

    /* Check handling of simple arrays */
    {
        Type const& type = *registry.build("/char[100]");
        MemoryLayout ops = Typelib::layout_of(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(100U, ops[1]);
    }

    /* Check a structure with arrays */
    {
        Type const& type = *registry.get("/struct B");
        MemoryLayout ops = Typelib::layout_of(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(type.getSize(), ops[1]);
    }

    /* Check a multidimensional array */
    {
        Type const& type = *registry.get("TestMultiDimArray");
        MemoryLayout ops = Typelib::layout_of(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(type.getSize(), ops[1]);
    }

    // Check a structure with pointer. Must throw by default, but accept if the
    // accept_pointers flag is set, in which case FLAG_MEMCPY is used for it.
    {
        Type const& type = *registry.get("/C");
        BOOST_CHECK_THROW(Typelib::layout_of(type), Typelib::NoLayout);

        MemoryLayout ops = Typelib::layout_of(type, true);
        BOOST_REQUIRE_EQUAL(2, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(sizeof(C), ops[1]);
    }
 
    // Check an opaque type
    {
        OpaqueType type("test_opaque", 10);
        BOOST_CHECK_THROW(Typelib::layout_of(type), Typelib::NoLayout);
    }
}

BOOST_AUTO_TEST_CASE(test_layout_containers)
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.1"), config, registry) );

    {
        Type const& type      = *registry.get("/struct StdCollections");
        Type const& v_double  = *registry.get("/std/vector</double>");
        Type const& v_of_v_double  = *registry.get("/std/vector</std/vector</double>>");
        MemoryLayout ops = Typelib::layout_of(type);

        StdCollections test;

        size_t expected[] = {
            MemLayout::FLAG_MEMCPY, reinterpret_cast<uint8_t*>(&test.dbl_vector) - reinterpret_cast<uint8_t*>(&test),
            MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_double),
                MemLayout::FLAG_MEMCPY, sizeof(double),
            MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY, reinterpret_cast<uint8_t*>(&test.v_of_v) - reinterpret_cast<uint8_t*>(&test.v8),
            MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_of_v_double),
                MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_double),
                    MemLayout::FLAG_MEMCPY, sizeof(double),
                MemLayout::FLAG_END,
            MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY, 16
        };

        size_t expected_size = sizeof(expected) / sizeof(size_t);
        BOOST_REQUIRE_EQUAL(expected_size, ops.size());
        for (size_t i = 0; i < expected_size; ++i)
            BOOST_REQUIRE_EQUAL(expected[i], ops[i]);
    }
}

