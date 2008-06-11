#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include <typelib/endianness.hh>
using namespace Typelib;

#include "test_cimport.1"

// Tests simple value handling
BOOST_AUTO_TEST_CASE( test_value_simple )
{
    Registry registry;

    float    f32  = 0.52;
    uint16_t ui16 = 15;
    int32_t  i32  = 456;

    BOOST_CHECK(f32 == value_cast<float>(&f32, *registry.get("float")));
    BOOST_CHECK(ui16 == value_cast<uint16_t>(&ui16, *registry.get("uint16_t")));
    BOOST_CHECK(i32 == value_cast<int32_t>(&i32, *registry.get("int32_t")));
    BOOST_CHECK_THROW(value_cast<float>(&i32, *registry.get("int32_t")), BadValueCast);
}

// Tests structure handling
BOOST_AUTO_TEST_CASE( test_value_struct )
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.1"), config, registry) );

    {
	A a = { 10, 20, 'b', 52 };
	Value v_a(&a, *registry.get("/struct A"));

	BOOST_CHECK_THROW( value_get_field(v_a, "does_not_exists"), FieldNotFound );
	BOOST_CHECK(a.a == value_cast<long long>(value_get_field(v_a, "a")));
	BOOST_CHECK(a.b == value_cast<int>(value_get_field(v_a, "b")));
	BOOST_CHECK(a.c == value_cast<char>(value_get_field(v_a, "c")));
	BOOST_CHECK(a.d == value_cast<short>(value_get_field(v_a, "d")));

	B b;
	b.a = a;
	for (int i = 0; i < 100; ++i)
	    b.c[i] = static_cast<float>(i) / 10.0f;

	Value v_b(&b, *registry.get("/B"));
	Value v_b_a(value_get_field(v_b, "a"));
	BOOST_CHECK(a.a == value_cast<long long>(value_get_field(v_b_a, "a")));
	BOOST_CHECK(a.b == value_cast<int>(value_get_field(v_b_a, "b")));
	BOOST_CHECK(a.c == value_cast<char>(value_get_field(v_b_a, "c")));
	BOOST_CHECK(a.d == value_cast<short>(value_get_field(v_b_a, "d")));
    }
}


struct TestArrayVisitor : public ValueVisitor
{
    size_t m_index;
    uint8_t* m_element;
    bool visit_(Value const& v, Array const& a)
    {
	m_element = static_cast<uint8_t*>(v.getData()) + m_index * a.getIndirection().getSize();
	return false;
    }

public:
    uint8_t* apply(int index, Value v)
    {
	m_element = 0;
	m_index = index;
	ValueVisitor::apply(v);
	return m_element;
    }
};
// Test array handling
BOOST_AUTO_TEST_CASE( test_value_array )
{
    // Get the test file into repository
    Registry registry;

    float test[10];

    Type const& array_type = *registry.build("/float[10]");
    TestArrayVisitor visitor;
    uint8_t* e_0 = visitor.apply(0, Value(test, array_type));
    uint8_t* e_9 = visitor.apply(9, Value(test, array_type));
    BOOST_REQUIRE_EQUAL(e_0, reinterpret_cast<uint8_t*>(&test[0]));
    BOOST_REQUIRE_EQUAL(e_9, reinterpret_cast<uint8_t*>(&test[9]));
}

BOOST_AUTO_TEST_CASE( test_value_endian_swap )
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.1"), config, registry) );

    A a = { 
	utilmm::endian::swap((long long)10), 
	utilmm::endian::swap((int)20), 
	utilmm::endian::swap('b'), 
	utilmm::endian::swap((short)52) 
    };
    B b;
    b.a = a;
    for (int i = 0; i < 10; ++i)
	b.c[i] = utilmm::endian::swap(static_cast<float>(i) / 10.0f);

    Value v_b(&b, *registry.get("/B"));
    Typelib::endian_swap(v_b);

    BOOST_REQUIRE_EQUAL(10, b.a.a);
    BOOST_REQUIRE_EQUAL(20, b.a.b);
    BOOST_REQUIRE_EQUAL('b', b.a.c);
    BOOST_REQUIRE_EQUAL(52, b.a.d);
    for (int i = 0; i < 10; ++i)
	BOOST_REQUIRE_EQUAL(static_cast<float>(i) / 10.0f, b.c[i]);
}

BOOST_AUTO_TEST_CASE( test_compile_endian_swap )
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.1"), config, registry) );

    size_t expected_a[]  = {
	CompileEndianSwapVisitor::FLAG_SWAP_8,
	CompileEndianSwapVisitor::FLAG_SWAP_4,
	CompileEndianSwapVisitor::FLAG_SKIP, 2, // char c plus one alignment byte
	15, 14
    };

    /* Check a simple structure with alignment issues */
    {
	Type const& a_t = *registry.get("/struct A");
	CompileEndianSwapVisitor compiled_a;
	compiled_a.apply(a_t);

	size_t expected_size = sizeof(expected_a) / sizeof(size_t);

	BOOST_REQUIRE_EQUAL(expected_size, compiled_a.m_compiled.size());
	for (size_t i = 0; i < sizeof(expected_a) / sizeof(size_t); ++i)
	    BOOST_REQUIRE_EQUAL(expected_a[i], compiled_a.m_compiled[i]);
    }

    /* Check handling of byte arrays */
    {
	Type const& type = *registry.build("/char[100]");
	CompileEndianSwapVisitor compiled;
	compiled.apply(type);

	BOOST_REQUIRE_EQUAL(2U, compiled.m_compiled.size());
	BOOST_REQUIRE_EQUAL(CompileEndianSwapVisitor::FLAG_SKIP, compiled.m_compiled[0]);
	BOOST_REQUIRE_EQUAL(100U, compiled.m_compiled[1]);
    }

    /* Check a structure with arrays */
    {
	Type const& type = *registry.get("/struct B");
	CompileEndianSwapVisitor compiled;
	compiled.apply(type);

	// Check the first field, it should be equal to expected_a
	size_t offset = sizeof(expected_a) / sizeof(size_t);
	for (size_t i = 0; i < offset; ++i)
	    BOOST_REQUIRE_EQUAL(expected_a[i], compiled.m_compiled[i]);

	// The second field is an array of floats, check it
	size_t expected[] = {
	    CompileEndianSwapVisitor::FLAG_ARRAY, 100, 4,
	    CompileEndianSwapVisitor::FLAG_SWAP_4,
	    CompileEndianSwapVisitor::FLAG_END
	};

	size_t expected_array_size = sizeof(expected) / sizeof(size_t);
	for (size_t i = 0; i < expected_array_size; ++i)
	    BOOST_REQUIRE_EQUAL(expected[i], compiled.m_compiled[i + offset]);
    }

    /* Check a multidimensional array */
    {
	Type const& type = *registry.get("TestMultiDimArray");
	CompileEndianSwapVisitor compiled;
	compiled.apply(type);

	size_t expected[] = {
	    CompileEndianSwapVisitor::FLAG_ARRAY, 100, 4,
	    CompileEndianSwapVisitor::FLAG_SWAP_4,
	    CompileEndianSwapVisitor::FLAG_END
	};
	size_t expected_size = sizeof(expected) / sizeof(size_t);
	BOOST_REQUIRE_EQUAL(expected_size, compiled.m_compiled.size());

	for (size_t i = 0; i < expected_size; ++i)
	    BOOST_REQUIRE_EQUAL(expected[i], compiled.m_compiled[i]);
    }
}

BOOST_AUTO_TEST_CASE( test_apply_endian_swap )
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.1"), config, registry) );

    A a = { 
	utilmm::endian::swap((long long)10), 
	utilmm::endian::swap((int)20), 
	utilmm::endian::swap('b'), 
	utilmm::endian::swap((short)52) 
    };
    B b;
    b.a = a;
    for (int i = 0; i < 10; ++i)
	b.c[i] = utilmm::endian::swap(static_cast<float>(i) / 10.0f);

    b.d[0] = utilmm::endian::swap<float>(42);

    for (int i = 0; i < 10; ++i)
	for (int j = 0; j < 10; ++j)
	    b.i[i][j] = utilmm::endian::swap<float>(i * 100 + j);

    Value v_b(&b, *registry.get("/B"));
    B swapped_b;
    Value v_swapped_b(&swapped_b, *registry.get("/B"));

    CompileEndianSwapVisitor compiled;
    compiled.apply(v_b.getType());
    compiled.swap(v_b, v_swapped_b);

    BOOST_REQUIRE_EQUAL(10, swapped_b.a.a);
    BOOST_REQUIRE_EQUAL(20, swapped_b.a.b);
    BOOST_REQUIRE_EQUAL('b', swapped_b.a.c);
    BOOST_REQUIRE_EQUAL(52, swapped_b.a.d);
    for (int i = 0; i < 10; ++i)
	BOOST_REQUIRE_EQUAL(static_cast<float>(i) / 10.0f, swapped_b.c[i]);
    BOOST_REQUIRE_EQUAL(42, swapped_b.d[0]);
    for (int i = 0; i < 10; ++i)
	for (int j = 0; j < 10; ++j)
	    BOOST_REQUIRE_EQUAL(i * 100 + j, swapped_b.i[i][j]);
}

