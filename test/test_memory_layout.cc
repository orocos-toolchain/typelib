#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <typelib/utilmm/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include <typelib/memory_layout.hh>
#include <lang/csupport/standard_types.hh>


#include <typelib/value_ops.hh>
#include <test/test_cimport.1>
#include <string.h>

using namespace Typelib;
using namespace Typelib::MemLayout;
using namespace std;

void REQUIRE_LAYOUT_EQUALS(size_t* begin, size_t* end, MemoryLayout const& layout)
{
    BOOST_REQUIRE_EQUAL(end - begin, layout.ops.size());
    for (size_t* it = begin; it != end; ++it)
    {
        BOOST_TEST_MESSAGE("checking layout element " << it - begin);
        BOOST_REQUIRE_EQUAL(*it, layout.ops[it - begin]);
    }
}

void REQUIRE_INIT_EQUALS(size_t* begin, size_t* end, MemoryLayout const& layout)
{
    BOOST_REQUIRE_EQUAL(end - begin, layout.init_ops.size());
    for (size_t* it = begin; it != end; ++it)
    {
        BOOST_TEST_MESSAGE("checking init element " << it - begin);
        BOOST_REQUIRE_EQUAL(*it, layout.init_ops[it - begin]);
    }
}

void REQUIRE_INIT_EMPTY(MemoryLayout const& layout)
{
    BOOST_REQUIRE( layout.init_ops.empty() );
}

static Registry& getRegistry()
{
    static Registry registry;

    PluginManager::self manager;
    unique_ptr<Importer> importer(manager->importer("tlb"));
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.tlb"), config, registry) );
    return registry;
}

BOOST_AUTO_TEST_CASE( test_it_generates_the_layout_of_a_simple_structure )
{
    Type const& type = *getRegistry().get("/A");
    MemoryLayout ops = Typelib::layout_of(type);
    A data;
    size_t expected[] = { FLAG_MEMCPY, offsetof(A, d) + sizeof(data.d) };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, ops);
    REQUIRE_INIT_EMPTY(ops);
}

BOOST_AUTO_TEST_CASE( test_it_generates_the_layout_of_simple_arrays )
{
    Type const& type = *getRegistry().build("/uint16_t[100]");
    MemoryLayout ops = Typelib::layout_of(type);
    size_t expected[] = { FLAG_MEMCPY, 200 };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, ops);
    REQUIRE_INIT_EMPTY(ops);
}

BOOST_AUTO_TEST_CASE( test_it_generates_the_layout_of_arrays_of_structures )
{
    Type const& type = *getRegistry().build("/B[100]");
    MemoryLayout ops = Typelib::layout_of(type);
    size_t expected[] = { FLAG_MEMCPY, sizeof(B) * 100 };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, ops);
    REQUIRE_INIT_EMPTY(ops);
}

BOOST_AUTO_TEST_CASE( test_it_generates_the_layout_of_simple_multidimensional_arrays )
{
    Type const& type = *getRegistry().get("TestMultiDimArray");
    MemoryLayout ops = Typelib::layout_of(type);

    size_t expected[] = { FLAG_MEMCPY, type.getSize() };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, ops);
    REQUIRE_INIT_EMPTY(ops);
}

BOOST_AUTO_TEST_CASE( test_it_rejects_creating_layouts_for_structures_with_pointers_by_default )
{
    Type const& type = *getRegistry().get("/C");
    BOOST_CHECK_THROW(Typelib::layout_of(type), Typelib::NoLayout);
}

BOOST_AUTO_TEST_CASE( test_it_generates_layout_for_structures_with_pointers_if_asked_for_it )
{
    Type const& type = *getRegistry().get("/C");
    MemoryLayout ops = Typelib::layout_of(type, true);
    C data;
    size_t expected[] = { FLAG_MEMCPY, offsetof(C, z) + sizeof(data.z) };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, ops);
    REQUIRE_INIT_EMPTY(ops);
}

BOOST_AUTO_TEST_CASE( test_it_rejects_creating_layouts_for_opaques )
{
    OpaqueType type("test_opaque", 10);
    BOOST_CHECK_THROW(Typelib::layout_of(type), Typelib::NoLayout);
}

BOOST_AUTO_TEST_CASE(test_layout_arrays)
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    unique_ptr<Importer> importer(manager->importer("tlb"));
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.tlb"), config, registry) );

    {
        Type const& type      = *registry.get("/Arrays");
        Compound const& str       = static_cast<Compound const&>(*registry.get("/NS1/Test"));
        BOOST_REQUIRE(str.getSize() != str.getField("b")->getOffset() + str.getField("b")->getType().getSize());

        Type const& v_str   = *registry.get("/std/vector</NS1/Test>");
        Type const& v_double  = *registry.get("/std/vector</double>");
        MemoryLayout ops = Typelib::layout_of(type);

        StdCollections test;

        Arrays arrays_v;
        size_t expected[] = {
            MemLayout::FLAG_MEMCPY,
               static_cast<size_t>(reinterpret_cast<uint8_t*>(&arrays_v.a_v_numeric) - reinterpret_cast<uint8_t*>(&arrays_v)),
            MemLayout::FLAG_ARRAY,
                10,
                MemLayout::FLAG_CONTAINER,
                    reinterpret_cast<size_t>(&v_double),
                    MemLayout::FLAG_MEMCPY,
                    sizeof(double),
                MemLayout::FLAG_END,
            MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY,
               static_cast<size_t>(reinterpret_cast<uint8_t*>(&arrays_v.a_v_struct[0]) - reinterpret_cast<uint8_t*>(&arrays_v.padding3)),
            MemLayout::FLAG_ARRAY,
                10,
                MemLayout::FLAG_CONTAINER,
                    reinterpret_cast<size_t>(&v_str),
                    MemLayout::FLAG_MEMCPY,
                    sizeof(NS1::Test),
                MemLayout::FLAG_END,
            MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY,
            sizeof(char)
        };

        size_t init_expected[] = {
            MemLayout::FLAG_INIT_SKIP,
               static_cast<size_t>(reinterpret_cast<uint8_t*>(&arrays_v.a_v_numeric) - reinterpret_cast<uint8_t*>(&arrays_v)),
            MemLayout::FLAG_INIT_REPEAT, 10,
                MemLayout::FLAG_INIT_CONTAINER,
                    reinterpret_cast<size_t>(&v_double),
            MemLayout::FLAG_INIT_END,
            MemLayout::FLAG_INIT_SKIP,
               static_cast<size_t>(reinterpret_cast<uint8_t*>(&arrays_v.a_v_struct[0]) - reinterpret_cast<uint8_t*>(&arrays_v.padding3)),
            MemLayout::FLAG_INIT_REPEAT, 10,
                MemLayout::FLAG_INIT_CONTAINER,
                    reinterpret_cast<size_t>(&v_str),
            MemLayout::FLAG_INIT_END
        };

        MemoryLayout raw_ops = Typelib::raw_layout_of(type);
        size_t expected_size = sizeof(expected) / sizeof(size_t);
        REQUIRE_LAYOUT_EQUALS(expected, expected + expected_size, ops);
        size_t init_expected_size = sizeof(init_expected) / sizeof(size_t);
        REQUIRE_INIT_EQUALS(init_expected, init_expected + init_expected_size, ops);
    }
}

BOOST_AUTO_TEST_CASE(test_layout_array_of_containers)
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    unique_ptr<Importer> importer(manager->importer("tlb"));
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.tlb"), config, registry) );

    {
        Type const& type  = *registry.get("/std/vector</double>[10]");
        Type const& v_double = static_cast<Indirect const&>(type).getIndirection();
        MemoryLayout ops = Typelib::layout_of(type);

        size_t expected[] = {
            MemLayout::FLAG_ARRAY, 10,
                MemLayout::FLAG_CONTAINER,
                    reinterpret_cast<size_t>(&v_double),
                    MemLayout::FLAG_MEMCPY,
                    sizeof(double),
                MemLayout::FLAG_END,
            MemLayout::FLAG_END
        };

        size_t init_expected[] = {
            MemLayout::FLAG_ARRAY, 10,
                MemLayout::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_double),
            MemLayout::FLAG_END
        };

        size_t expected_size = sizeof(expected) / sizeof(size_t);
        REQUIRE_LAYOUT_EQUALS(expected, expected + expected_size, ops);
        size_t init_expected_size = sizeof(init_expected) / sizeof(size_t);
        REQUIRE_INIT_EQUALS(init_expected, init_expected + init_expected_size, ops);
    }
}

BOOST_AUTO_TEST_CASE(test_layout_containers)
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    unique_ptr<Importer> importer(manager->importer("tlb"));
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.tlb"), config, registry) );

    {
        Type const& type      = *registry.get("/Collections");
        Compound const& str       = static_cast<Compound const&>(*registry.get("/NS1/Test"));
        BOOST_REQUIRE(str.getSize() != str.getField("b")->getOffset() + str.getField("b")->getType().getSize());

        Type const& v_str     = *registry.get("/std/vector</NS1/Test>");
        Type const& v_v_str   = *registry.get("/std/vector</std/vector</NS1/Test>>");
        Type const& v_double  = *registry.get("/std/vector</double>");
        Type const& v_v_double  = *registry.get("/std/vector</std/vector</double>>");
        MemoryLayout ops = Typelib::layout_of(type);

        Collections value;

        size_t expected[] = {
            MemLayout::FLAG_CONTAINER,
                reinterpret_cast<size_t>(&v_double),
                MemLayout::FLAG_MEMCPY,
                sizeof(double),
            MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY,
               static_cast<size_t>(reinterpret_cast<uint8_t*>(&value.v_struct) - reinterpret_cast<uint8_t*>(&value.padding1)),
            MemLayout::FLAG_CONTAINER,
                reinterpret_cast<size_t>(&v_str),
                MemLayout::FLAG_MEMCPY,
                sizeof(NS1::Test),
            MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY,
               static_cast<size_t>(reinterpret_cast<uint8_t*>(&value.v_v_numeric) - reinterpret_cast<uint8_t*>(&value.padding2)),
            MemLayout::FLAG_CONTAINER,
                reinterpret_cast<size_t>(&v_v_double),
                MemLayout::FLAG_CONTAINER,
                    reinterpret_cast<size_t>(&v_double),
                    MemLayout::FLAG_MEMCPY,
                    sizeof(double),
                MemLayout::FLAG_END,
            MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY,
               static_cast<size_t>(reinterpret_cast<uint8_t*>(&value.v_v_struct) - reinterpret_cast<uint8_t*>(&value.padding3)),
            MemLayout::FLAG_CONTAINER,
                reinterpret_cast<size_t>(&v_v_str),
                MemLayout::FLAG_CONTAINER,
                    reinterpret_cast<size_t>(&v_str),
                    MemLayout::FLAG_MEMCPY,
                    sizeof(NS1::Test),
                MemLayout::FLAG_END,
            MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY,
            sizeof(char)
        };

        size_t expected_size = sizeof(expected) / sizeof(size_t);
        REQUIRE_LAYOUT_EQUALS(expected, expected + expected_size, ops);
    }
}

BOOST_AUTO_TEST_CASE( test_require_init_enums )
{
    Enum enum_t("/Test", 0);
    enum_t.add("TEST", 10);
    Compound compound_t("/CTest");
    compound_t.addField("test", enum_t, 5);

    MemoryLayout ops = Typelib::layout_of(compound_t);

    size_t expected[] = {
        MemLayout::FLAG_INIT_SKIP, 5,
        MemLayout::FLAG_INIT, sizeof(Enum::integral_type), 0, 0, 0, 0, 0, 0, 0, 0 };
    size_t expected_size = 4 + sizeof(Enum::integral_type);
    *reinterpret_cast<Enum::integral_type*>(expected + 4) = 10;
    REQUIRE_INIT_EQUALS(expected, expected + expected_size, ops);
}


BOOST_AUTO_TEST_CASE( test_simplifies_merges_consecutive_memcpy )
{
    MemoryLayout layout;
    layout.pushMemcpy(10);
    layout.pushMemcpy(20);
    MemoryLayout result = layout.simplify(false, false);
    size_t expected[] = { FLAG_MEMCPY, 30 };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, result);
}

BOOST_AUTO_TEST_CASE( test_it_does_not_mistake_an_element_that_looks_like_a_skip_when_removing_trailing_skips )
{
    MemoryLayout first_element;
    first_element.pushMemcpy(FLAG_SKIP);

    MemoryLayout layout;
    layout.pushArray(10, first_element);

    layout = layout.simplify(true, true);
    size_t expected[] = { FLAG_MEMCPY, 10 * FLAG_SKIP };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, layout);
}

BOOST_AUTO_TEST_CASE( test_it_does_not_mistake_something_looking_like_a_typecode_when_simplifying )
{
    MemoryLayout first_element;
    std::vector<uint8_t> init_data;
    init_data.resize(1, 0);
    first_element.pushMemcpy(1, init_data);
    first_element.pushMemcpy(MemLayout::FLAG_ARRAY);

    MemoryLayout second_element;
    second_element.pushMemcpy(10);

    MemoryLayout layout;
    layout.pushArray(10, first_element);
    layout.pushArray(10, second_element);
    layout.pushMemcpy(20);

    MemoryLayout result = layout.simplify(true, false);
    size_t expected[] = { FLAG_MEMCPY, 10 * (MemLayout::FLAG_ARRAY + 1) + 10 * 10 + 20 };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, result);
    size_t init_expected[] = {
        FLAG_INIT_REPEAT, 10,
            FLAG_INIT, 1, 0,
            FLAG_SKIP, 1,
        FLAG_END
    };
    REQUIRE_INIT_EQUALS(init_expected, init_expected + 8, result);
}

BOOST_AUTO_TEST_CASE( test_simplifies_converts_simple_arrays_to_memcpy )
{
    MemoryLayout layout;
    layout.pushMemcpy(10);
    layout.pushGenericOp(MemLayout::FLAG_ARRAY, 10);
        layout.pushMemcpy(20);
        layout.pushMemcpy(21);
    layout.pushEnd();
    layout.pushMemcpy(22);
    MemoryLayout result = layout.simplify(false, false);
    size_t expected[] = { FLAG_MEMCPY, 442 };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, result);
}

BOOST_AUTO_TEST_CASE( test_simplifies_removes_empty_arrays )
{
    MemoryLayout layout;
    layout.pushMemcpy(10);
    MemoryLayout array_ops;
    layout.pushArray(10, array_ops);
    layout.pushMemcpy(22);
    MemoryLayout result = layout.simplify(false, false);
    size_t expected[] = { FLAG_MEMCPY, 32 };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, result);
    REQUIRE_INIT_EMPTY(result);
}

BOOST_AUTO_TEST_CASE( test_simplifies_converts_recursive_arrays_to_memcpy )
{
    MemoryLayout layout;
    layout.pushMemcpy(10);
    layout.pushGenericOp(MemLayout::FLAG_ARRAY, 10);
        layout.pushMemcpy(20);
        layout.pushMemcpy(21);
            layout.pushGenericOp(MemLayout::FLAG_ARRAY, 11);
            layout.pushMemcpy(10);
            layout.pushEnd();
        layout.pushMemcpy(15);
    layout.pushEnd();
    layout.pushMemcpy(22);
    MemoryLayout result = layout.simplify(false, false);
    size_t expected[] = { FLAG_MEMCPY, 1692 };
    REQUIRE_LAYOUT_EQUALS(expected, expected + 2, result);
}

