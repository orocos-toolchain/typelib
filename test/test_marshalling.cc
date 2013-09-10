#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include <typelib/value_ops.hh>

#include <test/test_cimport.1>
#include <string.h>

using namespace Typelib;
using namespace std;

BOOST_AUTO_TEST_CASE( test_marshalling_simple )
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    auto_ptr<Importer> importer(manager->importer("tlb"));
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.tlb"), config, registry) );

    /* Check a simple structure which translates into MEMCPY */
    {
        Type const& type = *registry.get("/A");
        A a;
        memset(&a, 1, sizeof(A));
        a.a = 10000;
        a.b = 1000;
        a.c = 100;
        a.d = 10;
        vector<uint8_t> buffer = dump(Value(&a, type));

        size_t expected_dump_size = offsetof(A, d) + sizeof(a.d);
        BOOST_REQUIRE_EQUAL( buffer.size(), expected_dump_size);
        BOOST_REQUIRE( !memcmp(&buffer[0], &a, expected_dump_size) );

        A reloaded;
        load(Value(&reloaded, type), buffer);
        BOOST_REQUIRE( !memcmp(&reloaded, &a, expected_dump_size) );

        // Try (in order)
        //  - smaller type
        //  - bigger type
        //  - bigger buffer
        //  - smaller buffer
        BOOST_REQUIRE_THROW(load(Value(&reloaded, *registry.build("/int[200]")), buffer), std::runtime_error);
        BOOST_REQUIRE_THROW(load(Value(&reloaded, *registry.get("/int")), buffer), std::runtime_error);
        buffer.resize(buffer.size() + 2);
        BOOST_REQUIRE_THROW(load(Value(&reloaded, type), buffer), std::runtime_error);
        buffer.resize(buffer.size() - 4);
        BOOST_REQUIRE_THROW(load(Value(&reloaded, type), buffer), std::runtime_error);
    }

    /* Now, insert SKIPS into it */
    {
        A a;
        size_t align1 = offsetof(A, b) - sizeof(a.a);
        size_t align2 = offsetof(A, c) - sizeof(a.b) - offsetof(A, b);
        size_t align3 = offsetof(A, d) - sizeof(a.c) - offsetof(A, c);
        size_t align4 = sizeof(A)      - sizeof(a.d) - offsetof(A, d);
        size_t raw_ops[] = {
            MemLayout::FLAG_MEMCPY, sizeof(long long),
            MemLayout::FLAG_SKIP, align1,
            MemLayout::FLAG_MEMCPY, sizeof(int),
            MemLayout::FLAG_SKIP, align2,
            MemLayout::FLAG_MEMCPY, sizeof(char),
            MemLayout::FLAG_SKIP, align3,
            MemLayout::FLAG_MEMCPY, sizeof(short)
        };

        MemoryLayout ops;
        ops.insert(ops.end(), raw_ops, raw_ops + 14);

        Type const& type = *registry.get("/A");
        memset(&a, 1, sizeof(A));
        a.a = 10000;
        a.b = 1000;
        a.c = 100;
        a.d = 10;
        vector<uint8_t> buffer;
        dump(Value(&a, type), buffer, ops);
        BOOST_REQUIRE_EQUAL( sizeof(A) - align1 - align2 - align3 - align4, buffer.size());
        BOOST_REQUIRE_EQUAL( *reinterpret_cast<long long*>(&buffer[0]), a.a );

        A reloaded;
        memset(&reloaded, 2, sizeof(A));
        load(Value(&reloaded, type), buffer, ops);
        BOOST_REQUIRE(memcmp(&a, &reloaded, sizeof(A)) < 0);
        BOOST_REQUIRE_EQUAL(a.a, reloaded.a);
        BOOST_REQUIRE_EQUAL(a.b, reloaded.b);
        BOOST_REQUIRE_EQUAL(a.c, reloaded.c);
        BOOST_REQUIRE_EQUAL(a.d, reloaded.d);
    }

    // And now check the array semantics
    {
        B b;
        for (unsigned int i = 0; i < sizeof(b); ++i)
            reinterpret_cast<uint8_t*>(&b)[i] = rand();

        size_t raw_ops[] = {
            MemLayout::FLAG_MEMCPY, offsetof(B, c),
            MemLayout::FLAG_ARRAY, 100,
                MemLayout::FLAG_MEMCPY, sizeof(b.c[0]),
            MemLayout::FLAG_END,
            MemLayout::FLAG_MEMCPY, sizeof(B) - offsetof(B, d)
        };

        MemoryLayout ops;
        ops.insert(ops.end(), raw_ops, raw_ops + 9);

        Type const& type = *registry.get("/B");
        vector<uint8_t> buffer;
        dump(Value(&b, type), buffer, ops);
        BOOST_REQUIRE_EQUAL( sizeof(B), buffer.size());
        BOOST_REQUIRE(!memcmp(&buffer[0], &b, sizeof(B)));

        B reloaded;
        load(Value(&reloaded, type), buffer, ops);
        BOOST_REQUIRE(!memcmp(&b, &reloaded, sizeof(B)));
    }
}

template<typename T>
size_t CHECK_SIMPLE_VALUE(vector<uint8_t> const& buffer, size_t offset, T value)
{
    BOOST_REQUIRE_EQUAL(value, *reinterpret_cast<T const*>(&buffer[offset]));
    return offset + sizeof(T);
}

template<typename T>
size_t CHECK_VECTOR_VALUE(vector<uint8_t> const& buffer, size_t offset, vector<T> const& value)
{
    // First, check for the size
    offset = CHECK_SIMPLE_VALUE(buffer, offset, static_cast<uint64_t>(value.size()));
    // Then for the elements
    for (size_t i = 0; i < value.size(); ++i)
        offset = CHECK_SIMPLE_VALUE(buffer, offset, value[i]);
    return offset;
}

BOOST_AUTO_TEST_CASE(test_marshalapply_containers)
{
    // Get the test file into repository
    Registry registry;
    PluginManager::self manager;
    auto_ptr<Importer> importer(manager->importer("tlb"));
    utilmm::config_set config;
    BOOST_REQUIRE_NO_THROW( importer->load(TEST_DATA_PATH("test_cimport.tlb"), config, registry) );

    StdCollections offset_discovery;
    uint8_t* base_ptr     = reinterpret_cast<uint8_t*>(&offset_discovery);
    size_t off_dbl_vector = reinterpret_cast<uint8_t*>(&offset_discovery.dbl_vector) - base_ptr;
    size_t off_v8         = reinterpret_cast<uint8_t*>(&offset_discovery.v8) - base_ptr;
    size_t off_v_of_v     = reinterpret_cast<uint8_t*>(&offset_discovery.v_of_v) - base_ptr;

    {
        StdCollections data;
        data.iv = 10;
        data.dbl_vector.resize(5);
        for (int i = 0; i < 5; ++i)
            data.dbl_vector[i] = 0.01 * i;
        data.v8  = -106;
        data.v_of_v.resize(5);
        for (int i = 0; i < 5; ++i)
        {
            data.v_of_v[i].resize(3);
            for (int j = 0; j < 3; ++j)
                data.v_of_v[i][j] = i * 10 + j;
        }
        data.v16 = 5235;
        data.v64 = 5230971546LL;

        Type const& type       = *registry.get("/StdCollections");
        vector<uint8_t> buffer = dump(Value(&data, type));

        size_t size_without_trailing_padding = 
            reinterpret_cast<uint8_t const*>(&data.padding) + sizeof(data.padding) - reinterpret_cast<uint8_t const*>(&data)
                - sizeof(std::vector<double>) - sizeof (std::vector< std::vector<double> >)
                + sizeof(double) * 20 // elements
                + 7 * sizeof(uint64_t);

        BOOST_REQUIRE_EQUAL( buffer.size(), size_without_trailing_padding );

        CHECK_SIMPLE_VALUE(buffer, 0, data.iv);
        size_t pos = CHECK_VECTOR_VALUE(buffer, off_dbl_vector, data.dbl_vector);
        CHECK_SIMPLE_VALUE(buffer, pos, data.v8);

        pos = CHECK_SIMPLE_VALUE(buffer, pos + off_v_of_v - off_v8, static_cast<uint64_t>(data.v_of_v.size()));
        for (int i = 0; i < 5; ++i)
            pos = CHECK_VECTOR_VALUE(buffer, pos, data.v_of_v[i]);

        StdCollections reloaded;
        load(Value(&reloaded, type), buffer);
        BOOST_REQUIRE( data.iv         == reloaded.iv );
        BOOST_REQUIRE( data.dbl_vector == reloaded.dbl_vector );
        BOOST_REQUIRE( data.v8         == reloaded.v8 );
        BOOST_REQUIRE( data.v16        == reloaded.v16 );
        BOOST_REQUIRE( data.v64        == reloaded.v64 );
        BOOST_REQUIRE( data.padding        == reloaded.padding );

        // Now, add the trailing bytes back. The load method should be OK with
        // it
        size_t size_with_trailing_padding =
            sizeof(StdCollections)
                - sizeof(std::vector<double>) - sizeof (std::vector< std::vector<double> >)
                + sizeof(double) * 20 // elements
                + 7 * sizeof(uint64_t); // element counts

        buffer.insert(buffer.end(), size_with_trailing_padding - size_without_trailing_padding, 0);
        {
            StdCollections reloaded;
            load(Value(&reloaded, type), buffer);
            BOOST_REQUIRE( data.iv         == reloaded.iv );
            BOOST_REQUIRE( data.dbl_vector == reloaded.dbl_vector );
            BOOST_REQUIRE( data.v8         == reloaded.v8 );
            BOOST_REQUIRE( data.v16        == reloaded.v16 );
            BOOST_REQUIRE( data.v64        == reloaded.v64 );
            BOOST_REQUIRE( data.padding        == reloaded.padding );
        }
    }

    {
        StdCollections data;
        
        data.iv = 0;
        data.v8 = 1;
        data.v_of_v.resize(5);
        data.v16 = 2;
        data.v64 = 3;

        Type const& type       = *registry.get("/StdCollections");
        vector<uint8_t> buffer = dump(Value(&data, type));
        size_t size_without_trailing_padding = 
            reinterpret_cast<uint8_t const*>(&data.padding) + sizeof(data.padding) - reinterpret_cast<uint8_t const*>(&data);
        BOOST_REQUIRE_EQUAL( buffer.size(),
                size_without_trailing_padding - sizeof(std::vector<double>) - sizeof (std::vector< std::vector<double> >)
                + 7 * sizeof(uint64_t)); // element counts

        CHECK_SIMPLE_VALUE(buffer, 0, data.iv);
        size_t pos = CHECK_VECTOR_VALUE(buffer, off_dbl_vector, data.dbl_vector);
        CHECK_SIMPLE_VALUE(buffer, pos, data.v8);

        pos = CHECK_SIMPLE_VALUE(buffer, pos + off_v_of_v - off_v8, static_cast<uint64_t>(data.v_of_v.size()));
        for (int i = 0; i < 5; ++i)
            pos = CHECK_VECTOR_VALUE(buffer, pos, data.v_of_v[i]);
    }
}

