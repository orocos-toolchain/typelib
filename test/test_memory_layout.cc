#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <typelib/utilmm/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include <typelib/memory_layout.hh>

#include <typelib/value_ops.hh>
#include <test/test_cimport.1>
#include <string.h>

using namespace Typelib;
using namespace std;

BOOST_AUTO_TEST_CASE(test_layout_simple) {
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    auto_ptr<Importer> importer(manager->importer("tlb"));
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW(
        importer->load(TEST_DATA_PATH("test_cimport.tlb"), config, registry));

    /* Check a simple structure with alignment issues */
    {
        Type const &type = *registry.get("/A");
        MemoryLayout ops = Typelib::layout_of(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        A data;
        BOOST_REQUIRE_EQUAL(offsetof(A, d) + sizeof(data.d), ops[1]);
    }

    /* Check handling of simple arrays */
    {
        Type const &type = *registry.build("/char[100]");
        MemoryLayout ops = Typelib::layout_of(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(100U, ops[1]);
    }

    /* Check a structure with arrays */
    {
        Type const &type = *registry.get("/B");
        MemoryLayout ops = Typelib::layout_of(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        B data;
        BOOST_REQUIRE_EQUAL(offsetof(B, z) + sizeof(data.z), ops[1]);
    }

    /* Check a multidimensional array */
    {
        Type const &type = *registry.get("TestMultiDimArray");
        MemoryLayout ops = Typelib::layout_of(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(type.getSize(), ops[1]);
    }

    // Check a structure with pointer. Must throw by default, but accept if the
    // accept_pointers flag is set, in which case FLAG_MEMCPY is used for it.
    {
        Type const &type = *registry.get("/C");
        BOOST_CHECK_THROW(Typelib::layout_of(type), Typelib::NoLayout);

        MemoryLayout ops = Typelib::layout_of(type, true);
        BOOST_REQUIRE_EQUAL(2, ops.size());
        BOOST_REQUIRE_EQUAL(MemLayout::FLAG_MEMCPY, ops[0]);
        C data;
        BOOST_REQUIRE_EQUAL(offsetof(C, z) + sizeof(data.z), ops[1]);
    }

    // Check an opaque type
    {
        OpaqueType type("test_opaque", 10);
        BOOST_CHECK_THROW(Typelib::layout_of(type), Typelib::NoLayout);
    }
}

BOOST_AUTO_TEST_CASE(test_layout_arrays) {
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    auto_ptr<Importer> importer(manager->importer("tlb"));
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW(
        importer->load(TEST_DATA_PATH("test_cimport.tlb"), config, registry));

    {
        Type const &type = *registry.get("/Arrays");
        Compound const &str =
            static_cast<Compound const &>(*registry.get("/NS1/Test"));
        BOOST_REQUIRE(str.getSize() !=
                      str.getField("b")->getOffset() +
                          str.getField("b")->getType().getSize());

        Type const &v_str = *registry.get("/std/vector</NS1/Test>");
        Type const &v_double = *registry.get("/std/vector</double>");
        MemoryLayout ops = Typelib::layout_of(type);

        StdCollections test;

        Arrays arrays_v;
        size_t expected[] = {
            MemLayout::FLAG_MEMCPY,
            reinterpret_cast<uint8_t *>(&arrays_v.a_v_numeric) -
                reinterpret_cast<uint8_t *>(&arrays_v),
            MemLayout::FLAG_ARRAY, 10, MemLayout::FLAG_CONTAINER,
            reinterpret_cast<size_t>(&v_double), MemLayout::FLAG_MEMCPY,
            sizeof(double), MemLayout::FLAG_END, MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY,
            reinterpret_cast<uint8_t *>(&arrays_v.a_v_struct[0]) -
                reinterpret_cast<uint8_t *>(&arrays_v.padding3),
            MemLayout::FLAG_ARRAY, 10, MemLayout::FLAG_CONTAINER,
            reinterpret_cast<size_t>(&v_str), MemLayout::FLAG_MEMCPY,
            sizeof(NS1::Test), MemLayout::FLAG_END, MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY, sizeof(char)};

        for (size_t i = 0; i < ops.size(); ++i)
            if (expected[i] != ops[i]) {
                Typelib::display(cerr, ops.begin(), ops.end());
                std::cerr << "error at index " << i << std::endl;
                BOOST_REQUIRE_EQUAL(expected[i], ops[i]);
            }

        // Check that we have all the operations that we need
        size_t expected_size = sizeof(expected) / sizeof(size_t);
        BOOST_REQUIRE_EQUAL(expected_size, ops.size());
    }
}

BOOST_AUTO_TEST_CASE(test_layout_containers) {
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    auto_ptr<Importer> importer(manager->importer("tlb"));
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW(
        importer->load(TEST_DATA_PATH("test_cimport.tlb"), config, registry));

    {
        Type const &type = *registry.get("/Collections");
        Compound const &str =
            static_cast<Compound const &>(*registry.get("/NS1/Test"));
        BOOST_REQUIRE(str.getSize() !=
                      str.getField("b")->getOffset() +
                          str.getField("b")->getType().getSize());

        Type const &v_str = *registry.get("/std/vector</NS1/Test>");
        Type const &v_v_str =
            *registry.get("/std/vector</std/vector</NS1/Test>>");
        Type const &v_double = *registry.get("/std/vector</double>");
        Type const &v_v_double =
            *registry.get("/std/vector</std/vector</double>>");
        MemoryLayout ops = Typelib::layout_of(type);

        Collections value;

        size_t expected[] = {
            MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_double),
            MemLayout::FLAG_MEMCPY, sizeof(double), MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY,
            reinterpret_cast<uint8_t *>(&value.v_struct) -
                reinterpret_cast<uint8_t *>(&value.padding1),
            MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_str),
            MemLayout::FLAG_MEMCPY, sizeof(NS1::Test), MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY,
            reinterpret_cast<uint8_t *>(&value.v_v_numeric) -
                reinterpret_cast<uint8_t *>(&value.padding2),
            MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_v_double),
            MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_double),
            MemLayout::FLAG_MEMCPY, sizeof(double), MemLayout::FLAG_END,
            MemLayout::FLAG_END, MemLayout::FLAG_MEMCPY,
            reinterpret_cast<uint8_t *>(&value.v_v_struct) -
                reinterpret_cast<uint8_t *>(&value.padding3),
            MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_v_str),
            MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_str),
            MemLayout::FLAG_MEMCPY, sizeof(NS1::Test), MemLayout::FLAG_END,
            MemLayout::FLAG_END, MemLayout::FLAG_MEMCPY, sizeof(char)};

        for (size_t i = 0; i < ops.size(); ++i)
            if (expected[i] != ops[i]) {
                Typelib::display(cerr, ops.begin(), ops.end());
                std::cerr << "error at index " << i << std::endl;
                BOOST_REQUIRE_EQUAL(expected[i], ops[i]);
            }

        // Check that we have all the operations that we need
        size_t expected_size = sizeof(expected) / sizeof(size_t);
        BOOST_REQUIRE_EQUAL(expected_size, ops.size());
    }
}
