#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include <typelib/marshalling.hh>

#include <test/test_cimport.1>
#include <string.h>

using namespace Typelib;
using namespace std;

BOOST_AUTO_TEST_CASE( test_marshalops_simple )
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
        MarshalOps ops;
	MarshalOpsVisitor compiled(ops);
	compiled.apply(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MarshalOpsVisitor::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(sizeof(A), ops[1]);
    }

    /* Check handling of simple arrays */
    {
        Type const& type = *registry.build("/char[100]");
        MarshalOps ops;
	MarshalOpsVisitor compiled(ops);
	compiled.apply(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MarshalOpsVisitor::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(100U, ops[1]);
    }

    /* Check a structure with arrays */
    {
        Type const& type = *registry.get("/struct B");
        MarshalOps ops;
	MarshalOpsVisitor compiled(ops);
	compiled.apply(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MarshalOpsVisitor::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(type.getSize(), ops[1]);
    }

    /* Check a multidimensional array */
    {
        Type const& type = *registry.get("TestMultiDimArray");
        MarshalOps ops;
	MarshalOpsVisitor compiled(ops);
	compiled.apply(type);

        BOOST_REQUIRE_EQUAL(2U, ops.size());
        BOOST_REQUIRE_EQUAL(MarshalOpsVisitor::FLAG_MEMCPY, ops[0]);
        BOOST_REQUIRE_EQUAL(type.getSize(), ops[1]);
    }

    // Check a structure with pointer (must throw)
    {
        Type const& type = *registry.get("/C");
        MarshalOps ops;
	MarshalOpsVisitor compiled(ops);
        BOOST_CHECK_THROW(compiled.apply(type), Typelib::UnsupportedMarshalling);
    }
 
    // Check an opaque type
    {
        OpaqueType type("test_opaque", 10);
        MarshalOps ops;
	MarshalOpsVisitor compiled(ops);
        BOOST_CHECK_THROW(compiled.apply(type), Typelib::UnsupportedMarshalling);
    }
}

BOOST_AUTO_TEST_CASE( test_marshalapply_simple )
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.1"), config, registry) );

    /* Check a simple structure which translates into MEMCPY */
    {
        Type const& type = *registry.get("/struct A");
        A a;
        memset(&a, 1, sizeof(A));
        a.a = 10000;
        a.b = 1000;
        a.c = 100;
        a.d = 10;
        vector<uint8_t> buffer = dump_to_memory(Value(&a, type));

        BOOST_REQUIRE_EQUAL( buffer.size(), sizeof(a));
        BOOST_REQUIRE( !memcmp(&buffer[0], &a, sizeof(a)) );

        A reloaded;
        load_from_memory(Value(&reloaded, type), buffer);
        BOOST_REQUIRE( !memcmp(&reloaded, &a, sizeof(a)) );

        // Try (in order)
        //  - smaller type
        //  - bigger type
        //  - bigger buffer
        //  - smaller buffer
        BOOST_REQUIRE_THROW(load_from_memory(Value(&reloaded, *registry.build("/int[200]")), buffer), MarshalledTypeMismatch);
        BOOST_REQUIRE_THROW(load_from_memory(Value(&reloaded, *registry.get("/int")), buffer), std::runtime_error);
        buffer.resize(buffer.size() + 2);
        BOOST_REQUIRE_THROW(load_from_memory(Value(&reloaded, type), buffer), std::runtime_error);
        buffer.resize(buffer.size() - 4);
        BOOST_REQUIRE_THROW(load_from_memory(Value(&reloaded, type), buffer), std::runtime_error);
    }

    /* Now, insert SKIPS into it */
    {
        A a;
        int align1 = offsetof(A, b) - sizeof(a.a);
        int align2 = offsetof(A, c) - sizeof(a.b) - offsetof(A, b);
        int align3 = offsetof(A, d) - sizeof(a.c) - offsetof(A, c);
        int align4 = sizeof(A)      - sizeof(a.d) - offsetof(A, d);
        size_t raw_ops[] = {
            MarshalOpsVisitor::FLAG_MEMCPY, sizeof(long long),
            MarshalOpsVisitor::FLAG_SKIP, align1,
            MarshalOpsVisitor::FLAG_MEMCPY, sizeof(int),
            MarshalOpsVisitor::FLAG_SKIP, align2,
            MarshalOpsVisitor::FLAG_MEMCPY, sizeof(char),
            MarshalOpsVisitor::FLAG_SKIP, align3,
            MarshalOpsVisitor::FLAG_MEMCPY, sizeof(short)
        };

        MarshalOps ops;
        ops.insert(ops.end(), raw_ops, raw_ops + 14);

        Type const& type = *registry.get("/struct A");
        memset(&a, 1, sizeof(A));
        a.a = 10000;
        a.b = 1000;
        a.c = 100;
        a.d = 10;
        vector<uint8_t> buffer;
        dump_to_memory(Value(&a, type), buffer, ops);
        BOOST_REQUIRE_EQUAL( sizeof(A) - align1 - align2 - align3 - align4, buffer.size());
        BOOST_REQUIRE_EQUAL( *reinterpret_cast<long long*>(&buffer[0]), a.a );

        A reloaded;
        memset(&reloaded, 2, sizeof(A));
        load_from_memory(Value(&reloaded, type), buffer, ops);
        BOOST_REQUIRE_EQUAL(-1, memcmp(&a, &reloaded, sizeof(A)));
        BOOST_REQUIRE_EQUAL(a.a, reloaded.a);
        BOOST_REQUIRE_EQUAL(a.b, reloaded.b);
        BOOST_REQUIRE_EQUAL(a.c, reloaded.c);
        BOOST_REQUIRE_EQUAL(a.d, reloaded.d);
    }

    // And now check the array semantics
    {
        B b;
        size_t raw_ops[] = {
            MarshalOpsVisitor::FLAG_MEMCPY, offsetof(B, c),
            MarshalOpsVisitor::FLAG_ARRAY, 100,
                MarshalOpsVisitor::FLAG_MEMCPY, sizeof(b.c[0]),
            MarshalOpsVisitor::FLAG_END,
            MarshalOpsVisitor::FLAG_MEMCPY, sizeof(B) - offsetof(B, d)
        };

        MarshalOps ops;
        ops.insert(ops.end(), raw_ops, raw_ops + 9);

        Type const& type = *registry.get("/struct B");
        vector<uint8_t> buffer;
        dump_to_memory(Value(&b, type), buffer, ops);
        BOOST_REQUIRE_EQUAL( sizeof(B), buffer.size());
        BOOST_REQUIRE(!memcmp(&buffer[0], &b, sizeof(B)));

        B reloaded;
        load_from_memory(Value(&reloaded, type), buffer, ops);
        BOOST_REQUIRE(!memcmp(&b, &reloaded, sizeof(B)));
    }
}

BOOST_AUTO_TEST_CASE(test_marshalops_containers)
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
        MarshalOps ops;
	MarshalOpsVisitor compiled(ops);
	compiled.apply(type);

        size_t expected[] = {
            MarshalOpsVisitor::FLAG_MEMCPY, 8,
            MarshalOpsVisitor::FLAG_CONTAINER, reinterpret_cast<size_t>(&v_double),
                MarshalOpsVisitor::FLAG_MEMCPY, sizeof(double),
            MarshalOpsVisitor::FLAG_END,
            MarshalOpsVisitor::FLAG_MEMCPY, 16
        };

        size_t expected_size = sizeof(expected) / sizeof(size_t);
        BOOST_REQUIRE_EQUAL(expected_size, ops.size());
        for (size_t i = 0; i < expected_size; ++i)
            BOOST_REQUIRE_EQUAL(expected[i], ops[i]);
    }
}

BOOST_AUTO_TEST_CASE(test_marshalapply_containers)
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.1"), config, registry) );

    {
        StdCollections data;
        data.iv = 10;
        data.dbl_vector.resize(100);
        for (int i = 0; i < 100; ++i)
            data.dbl_vector[i] = 0.01 * i;
        data.v8  = -106;
        data.v16 = 5235;
        data.v64 = 5230971546;

        Type const& type      = *registry.get("/struct StdCollections");
        vector<uint8_t> buffer = dump_to_memory(Value(&data, type));
        BOOST_REQUIRE_EQUAL( buffer.size(), sizeof(StdCollections) - sizeof(std::vector<double>) + sizeof(double) * 100 + sizeof(uint64_t));
        BOOST_REQUIRE(! memcmp(&data.iv, &buffer[0], sizeof(data.iv)));
        BOOST_REQUIRE_EQUAL(100, *reinterpret_cast<uint64_t*>(&buffer[8]));
        BOOST_REQUIRE(! memcmp(&data.dbl_vector[0], &buffer[16], sizeof(double) * 100));

        StdCollections reloaded;
        load_from_memory(Value(&reloaded, type), buffer);
        BOOST_REQUIRE( data.iv         == reloaded.iv );
        BOOST_REQUIRE( data.dbl_vector == reloaded.dbl_vector );
        BOOST_REQUIRE( data.v8         == reloaded.v8 );
        BOOST_REQUIRE( data.v16        == reloaded.v16 );
        BOOST_REQUIRE( data.v64        == reloaded.v64 );
    }
}

