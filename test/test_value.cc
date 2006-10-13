#include "testsuite.hh"
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include <typelib/endianness.hh>
using namespace Typelib;

#include "test_cimport.1"

class TC_Value
{
public:
    TC_Value() { }
    ~TC_Value() { }

    // Tests simple value handling
    void test_simple()
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
    void test_struct()
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
            BOOST_CHECK(a.a == value_cast<long>(value_get_field(v_a, "a")));
            BOOST_CHECK(a.b == value_cast<long>(value_get_field(v_a, "b")));
            BOOST_CHECK(a.c == value_cast<char>(value_get_field(v_a, "c")));
            BOOST_CHECK(a.d == value_cast<short>(value_get_field(v_a, "d")));

            B b;
            b.a = a;
            for (int i = 0; i < 100; ++i)
                b.c[i] = static_cast<float>(i) / 10.0f;

            Value v_b(&b, *registry.get("/B"));
            Value v_b_a(value_get_field(v_b, "a"));
            BOOST_CHECK(a.a == value_cast<long>(value_get_field(v_b_a, "a")));
            BOOST_CHECK(a.b == value_cast<long>(value_get_field(v_b_a, "b")));
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
    void test_array()
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

    void test_endian_swap()
    {
        // Get the test file into repository
        Registry registry;
        PluginManager::self manager;
        Importer* importer = manager->importer("c");
        utilmm::config_set config;
        BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.1"), config, registry) );

	A a = { utilmm::endian_swap((long)10), utilmm::endian_swap((long)20), utilmm::endian_swap('b'), utilmm::endian_swap((short)52) };
	B b;
	b.a = a;
	for (int i = 0; i < 10; ++i)
	    b.c[i] = utilmm::endian_swap(static_cast<float>(i) / 10.0f);

	Value v_b(&b, *registry.get("/B"));
	Typelib::endian_swap(v_b);

	BOOST_REQUIRE_EQUAL(10, b.a.a);
	BOOST_REQUIRE_EQUAL(20, b.a.b);
	BOOST_REQUIRE_EQUAL('b', b.a.c);
	BOOST_REQUIRE_EQUAL(52, b.a.d);
	for (int i = 0; i < 10; ++i)
	    BOOST_REQUIRE_EQUAL(static_cast<float>(i) / 10.0f, b.c[i]);
    }
};

void test_value(test_suite* ts) {
    boost::shared_ptr<TC_Value> instance( new TC_Value );
    ts->add( BOOST_CLASS_TEST_CASE( &TC_Value::test_simple, instance ) );
    ts->add( BOOST_CLASS_TEST_CASE( &TC_Value::test_struct, instance ) );
    ts->add( BOOST_CLASS_TEST_CASE( &TC_Value::test_array, instance ) );
    ts->add( BOOST_CLASS_TEST_CASE( &TC_Value::test_endian_swap, instance ) );
}

