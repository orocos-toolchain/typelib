#include <boost/test/auto_unit_test.hpp>

#include <lang/csupport/standard_types.hh>

#include <test/testsuite.hh>
#include <typelib/typemodel.hh>
#include <typelib/typename.hh>
#include <typelib/registry.hh>
#include <typelib/typedisplay.hh>
#include <typelib/registryiterator.hh>
#include <typelib/pluginmanager.hh>
using namespace Typelib;

BOOST_AUTO_TEST_CASE( test_typename_validation )
{
    BOOST_CHECK(!isValidTypename("std::string", false));
    BOOST_CHECK(!isValidTypename("std::string", true));
    BOOST_CHECK(!isValidTypename("/std/string<double>", false));
    BOOST_CHECK(isValidTypename("/std/string</double>", false));
    BOOST_CHECK(isValidTypename("/std/string</double>", true));
    BOOST_CHECK(!isValidTypename("std/string<double>", false));
    BOOST_CHECK(isValidTypename("std/string</double>", false));
    BOOST_CHECK(!isValidTypename("std/string</double>", true));

    BOOST_CHECK(isValidTypename("/std/string</double,9,/std/string>", true));
    BOOST_CHECK(isValidTypename("/std/string<3>", true));

    BOOST_CHECK(isValidTypename("/double[3]", true));
    BOOST_CHECK(isValidTypename("/std/string</double[3]>", true));
    BOOST_CHECK(isValidTypename("/wrappers/Matrix</double,3,1>/Scalar", true));
    BOOST_CHECK(isValidTypename("/std/vector</wrappers/Matrix</double,3,1>>", true));
    BOOST_CHECK(isValidTypename("/std/vector</wrappers/Matrix</double,3,1>>[4]", true));

    BOOST_CHECK(isValidTypename("s", false));
    BOOST_CHECK(!isValidTypename(":blabla", false));

    BOOST_CHECK(isValidTypename("/std/map</std/string,/trigger/behaviour/Description,/std/less</std/string>,/std/allocator</std/pair</const std/basic_string</char,/std/char_traits</char>,/std/allocator</char>>,/trigger/behaviour/Description>>>", true));
}

BOOST_AUTO_TEST_CASE( test_typename_is_in_namespace )
{
    BOOST_CHECK(!isInNamespace("/A/B/Type" , "/A/C"    , false));
    BOOST_CHECK(!isInNamespace("/A/B/Type" , "/A/C/"   , false));
    BOOST_CHECK(!isInNamespace("/B/B/Type" , "/B/A"    , false));
    BOOST_CHECK(!isInNamespace("/B/B/Type" , "/B/A/"   , false));
    BOOST_CHECK(!isInNamespace("/A/B/Type" , "/A/B/C"  , false));
    BOOST_CHECK(!isInNamespace("/A/B/Type" , "/A/B/C/" , false));
    BOOST_CHECK(!isInNamespace("/A/B/Type" , "/A/C"    , true));
    BOOST_CHECK(!isInNamespace("/A/B/Type" , "/A/C/"   , true));
    BOOST_CHECK(!isInNamespace("/B/B/Type" , "/B/A"    , true));
    BOOST_CHECK(!isInNamespace("/B/B/Type" , "/B/A/"   , true));
    BOOST_CHECK(!isInNamespace("/A/B/Type" , "/A/B/C"  , true));
    BOOST_CHECK(!isInNamespace("/A/B/Type" , "/A/B/C/" , true));

    BOOST_CHECK(!isInNamespace("/A/B/C/Type" , "/A/B"  , false));
    BOOST_CHECK(!isInNamespace("/A/B/C/Type" , "/A/B/" , false));
    BOOST_CHECK( isInNamespace("/A/B/Type"   , "/A/B"  , false));
    BOOST_CHECK( isInNamespace("/A/B/Type"   , "/A/B/" , false));
    BOOST_CHECK( isInNamespace("/A/B/C/Type" , "/A/B"  , true));
    BOOST_CHECK( isInNamespace("/A/B/C/Type" , "/A/B/" , true));
    BOOST_CHECK( isInNamespace("/A/B/Type"   , "/A/B"  , true));
    BOOST_CHECK( isInNamespace("/A/B/Type"   , "/A/B/" , true));
}

BOOST_AUTO_TEST_CASE( test_nameSort )
{
    BOOST_CHECK(!nameSort("/A/B/Type" , "/A/B/Type"));

    BOOST_CHECK(nameSort("/A/B" , "/A/B/Type"));
    BOOST_CHECK(!nameSort("/A/B/Type" , "/A/B"));

    BOOST_CHECK(nameSort("/A/A" , "/A/B/Type"));
    BOOST_CHECK(!nameSort("/A/B/Type" , "/A/A"));

    BOOST_CHECK(nameSort("/A/B/Type" , "/AA"));
    BOOST_CHECK(!nameSort("/AA" , "/A/B/Type"));

    BOOST_CHECK(nameSort("/A/B/Ty" , "/A/B/Type"));
    BOOST_CHECK(!nameSort("/A/B/Type" , "/A/B/Ty"));
}

BOOST_AUTO_TEST_CASE( test_typename_manipulation )
{
    BOOST_CHECK_EQUAL("/NS2/", getNormalizedNamespace("/NS2"));
    BOOST_CHECK_EQUAL("/NS2/", getNormalizedNamespace("/NS2/"));
    BOOST_CHECK_EQUAL("NS2/", getNormalizedNamespace("NS2/"));
    BOOST_CHECK_EQUAL("NS2/", getNormalizedNamespace("NS2"));

    BOOST_CHECK_EQUAL("NS3/Test", getRelativeName("/NS2/NS3/Test", "/NS2"));

    BOOST_CHECK_EQUAL("/NS2/NS3/", getNamespace("/NS2/NS3/Test"));
    BOOST_CHECK_EQUAL("/wrappers/Matrix</double,3,1>/", getNamespace("/wrappers/Matrix</double,3,1>/Scalar"));
    BOOST_CHECK_EQUAL("Scalar", getTypename("/wrappers/Matrix</double,3,1>/Scalar"));
    BOOST_CHECK_EQUAL("/wrappers/Matrix</double,3,1>/Gaussian</double,3>/", getNamespace("/wrappers/Matrix</double,3,1>/Gaussian</double,3>/Scalar"));
    BOOST_CHECK_EQUAL("Scalar", getTypename("/wrappers/Matrix</double,3,1>/Gaussian</double,3>/Scalar"));
    BOOST_CHECK_EQUAL("/std/", getNamespace("/std/vector</wrappers/Matrix</double,3,1>>"));

    BOOST_CHECK_EQUAL("Test", getTypename("/NS2/NS3/Test"));
    BOOST_CHECK_EQUAL("Scalar", getTypename("/wrappers/Matrix</double,3,1>/Scalar"));
    BOOST_CHECK_EQUAL("vector</wrappers/Matrix</double,3,1>>", getTypename("/std/vector</wrappers/Matrix</double,3,1>>"));
}

BOOST_AUTO_TEST_CASE( test_typename_path_to )
{
    BOOST_CHECK_EQUAL("B/",    getMinimalPathTo("/A/B/Type", "/A/C"));
    BOOST_CHECK_EQUAL("B/",    getMinimalPathTo("/A/B/Type", "/A/C/"));
    BOOST_CHECK_EQUAL("/B/B/", getMinimalPathTo("/B/B/Type", "/B/A"));
    BOOST_CHECK_EQUAL("/B/B/", getMinimalPathTo("/B/B/Type", "/B/A/"));
    BOOST_CHECK_EQUAL("/A/B/", getMinimalPathTo("/A/B/Type", "/A/B/C"));
    BOOST_CHECK_EQUAL("/A/B/", getMinimalPathTo("/A/B/Type", "/A/B/C/"));
    BOOST_CHECK_EQUAL("C/", getMinimalPathTo("/A/B/C/Type", "/A/B"));
    BOOST_CHECK_EQUAL("C/", getMinimalPathTo("/A/B/C/Type", "/A/B/"));
    BOOST_CHECK_EQUAL("", getMinimalPathTo("/A/B/Type", "/A/B"));
    BOOST_CHECK_EQUAL("", getMinimalPathTo("/A/B/Type", "/A/B/"));
    BOOST_CHECK_EQUAL("/A/", getMinimalPathTo("/A/B", "/C/A"));
}

BOOST_AUTO_TEST_CASE( test_registry_namespaces )
{
    // Add a simple type in an imaginary namespace
    Registry registry;
    registry.add( new Numeric("/NS1/Test", 1, Numeric::SInt) );
    registry.add( new Numeric("/NS2/Test", 2, Numeric::SInt) );

    registry.setDefaultNamespace("/NS2");
    BOOST_REQUIRE_EQUAL("/NS2/Test", registry.getFullName("Test"));
    BOOST_REQUIRE_EQUAL("/NS2/NS3/Test", registry.getFullName("NS3/Test"));
    BOOST_REQUIRE_EQUAL("/Test", registry.getFullName("/Test"));
    registry.add( new Numeric("/NS2/NS3/Test", 3, Numeric::SInt) );
    registry.add( new Numeric("/Test", 4, Numeric::SInt) );
    registry.setDefaultNamespace("");

    Numeric const* ns1, *ns2, *ns3, *ns;
    BOOST_REQUIRE(( ns1 = dynamic_cast<Numeric const*>(registry.get("/NS1/Test")) ));
    BOOST_REQUIRE(( ns2 = dynamic_cast<Numeric const*>(registry.get("/NS2/Test")) ));
    BOOST_REQUIRE(( ns3 = dynamic_cast<Numeric const*>(registry.get("/NS2/NS3/Test")) ));
    BOOST_REQUIRE(( ns  = dynamic_cast<Numeric const*>(registry.get("/Test")) ));

    BOOST_REQUIRE_EQUAL(1, ns1->getSize());
    BOOST_REQUIRE_EQUAL(2, ns2->getSize());
    BOOST_REQUIRE_EQUAL(3, ns3->getSize());
    BOOST_REQUIRE_EQUAL(4, ns->getSize());

    { registry.setDefaultNamespace("/NS2");
        Type const* relative;
        BOOST_REQUIRE(( relative = registry.get("Test") ));
        BOOST_REQUIRE_EQUAL(relative, ns2);
        BOOST_REQUIRE(( relative = registry.get("NS3/Test") ));
        BOOST_REQUIRE_EQUAL(relative, ns3);
        BOOST_REQUIRE(( relative = registry.get("/Test") ));
        BOOST_REQUIRE_EQUAL(relative, ns);
        BOOST_REQUIRE(( relative = registry.get("/NS1/Test") ));
        BOOST_REQUIRE_EQUAL(relative, ns1);
    }

}

BOOST_AUTO_TEST_CASE( test_namespace_update_at_insertion )
{
    Registry registry;
    registry.setDefaultNamespace("/A/B/B");
    registry.add( new Numeric("/A/B/B/Test", 3, Numeric::SInt) );
    BOOST_REQUIRE(( registry.get("Test") ));
    BOOST_REQUIRE(( registry.get("B/Test") ));
    BOOST_REQUIRE(( registry.get("B/B/Test") ));
    BOOST_REQUIRE(( registry.get("A/B/B/Test") ));

    registry.add( new Numeric("/A/B/Test", 3, Numeric::SInt) );
    BOOST_REQUIRE(( registry.get("Test") ));
    BOOST_REQUIRE(( registry.get("B/Test") ));
    BOOST_REQUIRE(( registry.get("B/B/Test") ));
    BOOST_REQUIRE(( registry.get("A/B/B/Test") ));
    BOOST_REQUIRE(( registry.get("A/B/Test") ));

    BOOST_REQUIRE_EQUAL(registry.get("/A/B/B/Test"), registry.get("Test"));
    BOOST_REQUIRE_EQUAL(registry.get("/A/B/B/Test"), registry.get("B/Test"));

    registry.setDefaultNamespace("/A/B/B/B");
    registry.add( new Numeric("/A/B/B/B/Test", 3, Numeric::SInt) );
    BOOST_REQUIRE_EQUAL(registry.get("/A/B/B/B/Test"), registry.get("Test"));
    BOOST_REQUIRE_EQUAL(registry.get("/A/B/B/B/Test"), registry.get("B/Test"));
    BOOST_REQUIRE_EQUAL(registry.get("/A/B/B/B/Test"), registry.get("B/B/Test"));
}

static void assert_registries_equal(Registry const& registry, Registry const& ref)
{
    BOOST_REQUIRE_EQUAL(ref.size(), registry.size());

    Registry::Iterator
        t_it = registry.begin(),
        t_end = registry.end(),
        r_it = ref.begin(),
        r_end = ref.end();

    for (; t_it != t_end && r_it != r_end; ++t_it, ++r_it)
    {
        BOOST_REQUIRE_EQUAL(t_it.getName(), r_it.getName());
        BOOST_REQUIRE_EQUAL(t_it->getName(), r_it->getName());
        BOOST_REQUIRE_MESSAGE(t_it->isSame(*r_it), "definitions of " << t_it->getName() << " differ: " << *t_it << "\n" << *r_it);
    }

    BOOST_REQUIRE(t_it == t_end);
    BOOST_REQUIRE(r_it == r_end);
}

BOOST_AUTO_TEST_CASE( test_repositories_merge )
{
    static const char* test_file = TEST_DATA_PATH("test_cimport.tlb");

    utilmm::config_set config;
    PluginManager::self manager;

    std::unique_ptr<Registry> ref( manager->load("tlb", test_file, config));
    Registry target;
    target.merge(*ref);
    assert_registries_equal(target, *ref);

    target.merge(*ref);
    assert_registries_equal(target, *ref);
}

BOOST_AUTO_TEST_CASE( test_array_auto_alias )
{
    Registry registry;
    Typelib::CXX::addStandardTypes(registry);
    registry.alias("/int", "/A");
    registry.add(new Array(*registry.get("/int"), 10));
    BOOST_REQUIRE(registry.get("/A[10]"));
    BOOST_REQUIRE(*registry.get("/A[10]") == *registry.get("/int[10]"));
    registry.alias("/int", "/B");
    BOOST_REQUIRE(registry.get("/B[10]"));
    BOOST_REQUIRE(*registry.get("/B[10]") == *registry.get("/int[10]"));
}

BOOST_AUTO_TEST_CASE( test_registry_merge_keeps_alias_persistent_flag )
{
    Registry registry;
    Typelib::CXX::addStandardTypes(registry);

    registry.alias("/int", "/Persistent", true);
    registry.alias("/int", "/Temporary", false);
    BOOST_REQUIRE(registry.get("/Persistent"));
    BOOST_REQUIRE(registry.find("/Persistent") != registry.end());

    Registry target;
    target.merge(registry);
    { RegistryIterator it = target.find("/Persistent");
        BOOST_REQUIRE(it != target.end());
        BOOST_REQUIRE(it.isPersistent());
    }

    { RegistryIterator it = target.find("/Temporary");
        BOOST_REQUIRE(it != target.end());
        BOOST_REQUIRE(!it.isPersistent());
    }
}

BOOST_AUTO_TEST_CASE( test_registry_minimal_keeps_alias_persistent_flag )
{
    Registry registry;
    Typelib::CXX::addStandardTypes(registry);
    Registry target;
    Typelib::CXX::addStandardTypes(target);
    target.alias("/int", "/Persistent", true);
    target.alias("/int", "/Temporary", false);

    target.minimal(registry);
    { RegistryIterator it = target.find("/Persistent");
        BOOST_REQUIRE(it != target.end());
        BOOST_REQUIRE(it.isPersistent());
    }

    { RegistryIterator it = target.find("/Temporary");
        BOOST_REQUIRE(it != target.end());
        BOOST_REQUIRE(!it.isPersistent());
    }
}

BOOST_AUTO_TEST_CASE( test_registry_range )
{
    Registry registry;
    Typelib::CXX::addStandardTypes(registry);
    registry.alias("/int", "/A/A", true);
    registry.alias("/int", "/A/C", true);
    registry.alias("/int", "/A/F", true);
    registry.alias("/int", "/A/B/A", true);
    registry.alias("/int", "/A/B/C", true);
    registry.alias("/int", "/A/B/F", true);
    registry.alias("/int", "/B/A", true);
    registry.alias("/int", "/B/C", true);
    registry.alias("/int", "/B/F", true);

    BOOST_REQUIRE_EQUAL("/A/A", registry.begin("/A").getName());
    BOOST_REQUIRE_EQUAL("/B/A", registry.end("/A").getName());
}

