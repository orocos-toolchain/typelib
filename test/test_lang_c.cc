#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <utilmm/configfile/configset.hh>
#include <utilmm/stringtools.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/typedisplay.hh>
#include <typesolver.hh>
#include "test_cimport.1"
#include <iostream>

#include <antlr/ANTLRException.hpp>
using namespace Typelib;
using std::string;
using utilmm::split;
using utilmm::join;

static void import_test_types(Registry& registry)
{
    static const char* test_file = TEST_DATA_PATH("test_cimport.h");

    utilmm::config_set config;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    config.set("include", TEST_DATA_PATH(".."));
    config.set("define", "GOOD");
    importer->load(test_file, config, registry);
}

BOOST_AUTO_TEST_CASE( test_strict_c_import )
{
    static const char* test_file = TEST_DATA_PATH("test_cimport.h");
    PluginManager::self manager;
    Importer* importer = manager->importer("c");

    utilmm::config_set config;
    config.set("cxx", "false");
    config.set("include", TEST_DATA_PATH(".."));
    config.set("define", "GOOD");
    config.insert("define", "VALID_STRICT_C");
    Registry registry;
    importer->load(test_file, config, registry);

    BOOST_CHECK( !registry.has("/NS1/NS2/Test") );
    BOOST_CHECK( !registry.has("/NS1/Test") );
    BOOST_CHECK( !registry.has("/NS1/Bla/Test") );
    BOOST_CHECK( registry.has("/NS1/NS2/struct Test") );
    BOOST_CHECK( registry.has("/NS1/struct Test") );
    BOOST_CHECK( registry.has("/NS1/Bla/struct Test") );

    BOOST_CHECK( registry.has("/enum INPUT_OUTPUT_MODE") );
    BOOST_CHECK( registry.has("/INPUT_OUTPUT_MODE") );
    BOOST_CHECK_EQUAL( "/enum INPUT_OUTPUT_MODE", registry.get("/INPUT_OUTPUT_MODE")->getName() );
}

static Type const& check_type(Registry const& registry, string const& name, size_t expected_size)
{
    // Replace the '::' in +name+ by '/'
    string const typelib_name = join(split(name, "::"), "/");

    Type const* object = registry.get(typelib_name);
    BOOST_REQUIRE_MESSAGE(object, name << " is not defined");
    BOOST_REQUIRE_MESSAGE(object->getSize() == expected_size, "size mismatch for " << name << " " << object->getSize() << " != (expected) " << expected_size << ": " << *object);
    return *object;
}
#define CHECK_TYPE(type) check_type(registry, #type, sizeof(type))

static void check_field(Registry const& registry, string const& compound_name, string const& field_name,
        size_t expected_offset, string const& expected_type, size_t expected_size)
{
    string const typelib_compound_name = join(split(compound_name, "::"), "/");
    string const typelib_expected_name = join(split(expected_type, "::"), "/");

    Compound const& compound_object = dynamic_cast<Compound const&>(*registry.get(typelib_compound_name));
    Field const* field_object = compound_object.getField(field_name);
    BOOST_REQUIRE(field_object);
    BOOST_REQUIRE_EQUAL(field_object->getOffset(), expected_offset);
    BOOST_REQUIRE_MESSAGE(
            (&(field_object->getType()) == registry.get(typelib_expected_name)),
            "expecting " << typelib_expected_name << ", got " <<
            field_object->getType().getName() << " for " << compound_name << "." << field_name);
    BOOST_REQUIRE_EQUAL(field_object->getType().getSize(), expected_size);
}

#define CHECK_FIELD(compound, field, expected_type, expected_name) \
{ compound v; \
    size_t offset = reinterpret_cast<uint8_t*>(&v.field) - reinterpret_cast<uint8_t*>(&v); \
    check_field(registry, #compound, #field, offset, (expected_name ? expected_name : #expected_type), sizeof(expected_type)); }

BOOST_AUTO_TEST_CASE( test_c_import )
{
    Registry registry;
    static const char* test_file = TEST_DATA_PATH("test_cimport.h");

    PluginManager::self manager;
    Importer* importer = manager->importer("c");

    {
	Registry temp_registry;
	utilmm::config_set config;
	config.set("include", TEST_DATA_PATH(".."));
	config.set("define", "GOOD");
	importer->load(test_file, config, temp_registry);
    }
    {
	Registry temp_registry;
	utilmm::config_set config;
	config.insert("rawflag", "-I" TEST_DATA_PATH(".."));
	config.insert("rawflag", "-DGOOD");
	importer->load(test_file, config, temp_registry);
    }

    utilmm::config_set config;
    // Load the file in registry
    BOOST_CHECK_THROW( importer->load("does_not_exist", config, registry), ImportError);
    BOOST_CHECK_THROW( importer->load(test_file, config, registry), ImportError );
    config.set("include", TEST_DATA_PATH(".."));
    config.set("define", "GOOD");
    importer->load(test_file, config, registry);

    // Check that the types are defined
    CHECK_TYPE(struct A);
    CHECK_TYPE(struct B);
    CHECK_TYPE(ADef);
    CHECK_TYPE(B);
    CHECK_TYPE( struct A );
    CHECK_TYPE( struct B );
    CHECK_TYPE( ADef );
    Compound const& b = dynamic_cast<Compound const&>(CHECK_TYPE( B ));
    CHECK_TYPE( OPAQUE_TYPE );
    CHECK_TYPE( NS1::NS2::Test );
    CHECK_TYPE( NS1::Bla::Test );
    CHECK_FIELD(NS1::Test, b, NS1::Bla::Test, 0);
    CHECK_TYPE( NS1::Test );
    CHECK_TYPE( NS1::NS2::Test );
    CHECK_TYPE( NS1::Test );
    CHECK_TYPE( NS1::Bla::Test );

    // In C++ mode, the main type is without any leading prefix
    BOOST_CHECK_EQUAL( "/NS1/NS2/Test", registry.get("/NS1/NS2/struct Test")->getName());

    // Check that the size of B.a is the same as A
    CHECK_FIELD(B, a, ADef, 0);

    // Check the type of c (array of floats)
    CHECK_FIELD(B, c, float[100], 0);

    // Check the sizes for various ways of defining integer constants
    CHECK_FIELD(B, d, float[1], 0);
    CHECK_FIELD(B, e, float[1], 0);
    CHECK_FIELD(B, f, float[3], 0);
    CHECK_FIELD(B, g, float[2], 0);
    CHECK_FIELD(B, h, A[4], 0);
    CHECK_FIELD(B, i, float[20][10], 0);
    CHECK_FIELD(B, x, float, 0);
    CHECK_FIELD(B, y, float, 0);
    CHECK_FIELD(B, z, float, 0);

    // order of indexes of multi-dimensional arrays are reverse than the 
    // ones in C because we always read dimensions from right to left
    // (i.e. b.i is supposed to be a (array of 10 elements of (array of 
    // 20 elements of floats))
    Field const* b_i = b.getField("i");
    BOOST_CHECK_EQUAL( &(b_i->getType()), registry.get("/float[20][10]") );

    Compound const* c   = static_cast<Compound const*>(registry.get("/C"));
    Field const* c_x = c->getField("x");
    BOOST_CHECK_EQUAL( &(c_x->getType()), registry.get("/float[4]") );
    Field const* c_y = c->getField("y");
    BOOST_CHECK_EQUAL( &(c_y->getType()), registry.get("/float*") );
    Field const* c_z = c->getField("z");
    BOOST_CHECK_EQUAL( &(c_z->getType()), registry.get("/float") );

    // Check the array indirection
    Array const& b_c_array(dynamic_cast<Array const&>(b.getField("c")->getType()));
    BOOST_CHECK_EQUAL( &b_c_array.getIndirection(), registry.get("/float") );
    BOOST_CHECK_EQUAL( b_c_array.getDimension(), 100UL );

    // Test the forms of DEFINE_STR and DEFINE_ID (anonymous structure and pointer-to-struct)
    BOOST_CHECK( registry.has("/DEFINE_STR") );
    BOOST_CHECK( registry.has("/DEFINE_ID") );
    BOOST_CHECK_EQUAL(Type::Compound, registry.get("/DEFINE_STR")->getCategory());
    BOOST_CHECK_EQUAL(Type::Pointer, registry.get("/DEFINE_ID")->getCategory());
    BOOST_CHECK( *registry.get("/DEFINE_STR") == static_cast<Pointer const*>(registry.get("/DEFINE_ID"))->getIndirection());

    // Test definition of recursive structures
    BOOST_CHECK( registry.has("/Recursive") );
    BOOST_CHECK_EQUAL(Type::Compound, registry.get("/Recursive")->getCategory());
    Type const& recursive_ptr = static_cast<Compound const*>(registry.get("/Recursive"))->getField("next")->getType();
    BOOST_CHECK_EQUAL(Type::Pointer, recursive_ptr.getCategory());
    BOOST_CHECK( *registry.get("/Recursive") == static_cast<Pointer const&>(recursive_ptr).getIndirection());
    
    // Test the forms of ANONYMOUS_ENUM and ANONYMOUS_ENUM_PTR
    BOOST_CHECK( registry.has("/ANONYMOUS_ENUM") );
    BOOST_CHECK( registry.has("/ANONYMOUS_ENUM_PTR") );
    BOOST_CHECK_EQUAL(Type::Enum, registry.get("/ANONYMOUS_ENUM")->getCategory());
    BOOST_CHECK_EQUAL(Type::Pointer, registry.get("/ANONYMOUS_ENUM_PTR")->getCategory());
    BOOST_CHECK( *registry.get("/ANONYMOUS_ENUM") == static_cast<Pointer const*>(registry.get("/ANONYMOUS_ENUM_PTR"))->getIndirection());

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
        { "E_FROM_SYMBOL", E_FIRST + E_HEX },
        { "E_FROM_SIZEOF_STD",  sizeof(int32_t) },
        { "E_FROM_SIZEOF_SPEC", sizeof(B) },
	{ 0, 0 }
    };
	
    for (ExpectedValue* exp_it = expected; exp_it->name; ++exp_it)
    {
	Enum::ValueMap::const_iterator it = map.find(exp_it->name);
	BOOST_REQUIRE( it != map.end() );
	BOOST_CHECK_EQUAL(exp_it->value, it->second);
    }
}

BOOST_AUTO_TEST_CASE( test_std_collections )
{
    Registry registry;
    import_test_types(registry);

    CHECK_TYPE(StdCollections);
    CHECK_FIELD(StdCollections, iv, int, 0);
    CHECK_FIELD(StdCollections, dbl_vector, std::vector<double>, "/std/vector</double>");
    CHECK_FIELD(StdCollections, v8, int8_t, 0);
    //CHECK_FIELD(StdCollections, float_set, std::set<float>, "/std/set</float>");
    CHECK_FIELD(StdCollections, v16, int16_t, 0);
    CHECK_FIELD(StdCollections, bv, int64_t, 0);
    
    // Unfortunately, cannot check this one automatically
    // Compound const& collections = dynamic_cast<Compound const&>(*registry.get("StdCollections"));
    // Field const* field = collections.getField("int_map");
    // BOOST_REQUIRE(field);

    // BOOST_REQUIRE_MESSAGE(&field->getType() == registry.get("/std/map</int,/int>"),
    //         field->getType().getName());
}

BOOST_AUTO_TEST_CASE( test_c_array_typedefs )
{
    Registry registry;
    import_test_types(registry);

    BOOST_REQUIRE(( registry.get("array_typedef") ));
    Array const* array_t = dynamic_cast<Typelib::Array const*>(registry.get("array_typedef"));
    BOOST_REQUIRE(( array_t ));
    BOOST_REQUIRE_EQUAL(256, array_t->getDimension());
    BOOST_REQUIRE_EQUAL(registry.get("int"), &array_t->getIndirection());

    BOOST_REQUIRE(( registry.get("multi_array_typedef") ));
    array_t = dynamic_cast<Typelib::Array const*>(registry.get("multi_array_typedef"));
    BOOST_REQUIRE(( array_t ));
    BOOST_REQUIRE_EQUAL(256, array_t->getDimension());
    array_t = dynamic_cast<Typelib::Array const*>(&array_t->getIndirection());
    BOOST_REQUIRE_EQUAL(512, array_t->getDimension());
    BOOST_REQUIRE_EQUAL(registry.get("int"), &array_t->getIndirection());
}

BOOST_AUTO_TEST_CASE( test_import_validation )
{
    utilmm::config_set config;
    PluginManager::self manager;

    static const char* test1 = TEST_DATA_PATH("data/nested_types/struct_in_struct.h");
    BOOST_REQUIRE_THROW( manager->load("c", test1, config), TypeSolver::NestedTypeDefinition );
    static const char* test2 = TEST_DATA_PATH("data/nested_types/enum_in_struct.h");
    BOOST_REQUIRE_THROW( manager->load("c", test2, config), TypeSolver::NestedTypeDefinition );
}

BOOST_AUTO_TEST_CASE( test_import_problematic_headers )
{
    utilmm::config_set config;
    PluginManager::self manager;

    {
        static const char* test_file = TEST_DATA_PATH("data/posterLib.h");
        delete manager->load("c", test_file, config);
    }

    {
        static const char* test_file = TEST_DATA_PATH("data/stdheaders.h");
        delete manager->load("c", test_file, config);
    }
}

BOOST_AUTO_TEST_CASE( test_import_and_merge )
{
    static const char* test_file = TEST_DATA_PATH("test_cimport.h");

    utilmm::config_set config;
    config.set("include", TEST_DATA_PATH(".."));
    config.set("define", "GOOD");
    PluginManager::self manager;

    { std::auto_ptr<Registry> r1(manager->load("c", test_file, config));
        std::auto_ptr<Registry> r2(manager->load("c", test_file, config));
        r1->merge(*r2);
    }
    { Registry target;
        std::auto_ptr<Registry> r1(manager->load("c", test_file, config));
        std::auto_ptr<Registry> r2(manager->load("c", test_file, config));
        target.merge(*r1);
        target.merge(*r2);
    }
}

