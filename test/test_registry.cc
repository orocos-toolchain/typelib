#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <typelib/typemodel.hh>
#include <typelib/typename.hh>
#include <typelib/registry.hh>
using namespace Typelib;

BOOST_AUTO_TEST_CASE( test_typename_manipulation )
{
    BOOST_REQUIRE_EQUAL("/NS2/", getNormalizedNamespace("/NS2"));
    BOOST_REQUIRE_EQUAL("/NS2/", getNormalizedNamespace("/NS2/"));
    BOOST_REQUIRE_EQUAL("NS2/", getNormalizedNamespace("NS2/"));
    BOOST_REQUIRE_EQUAL("NS2/", getNormalizedNamespace("NS2"));

    BOOST_REQUIRE_EQUAL("NS3/Test", getRelativeName("/NS2/NS3/Test", "/NS2"));
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

