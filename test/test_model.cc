#include <boost/test/auto_unit_test.hpp>

#include "testsuite.hh"
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include "test_cimport.1"
using namespace Typelib;

BOOST_AUTO_TEST_CASE( test_compound_size )
{
    Registry r;

    Compound str("");
    str.addField("a", *r.get("/int32_t"), 0);
    BOOST_REQUIRE_EQUAL(4U, str.getSize());
    str.addField("b", *r.get("/int32_t"), 4);
    BOOST_REQUIRE_EQUAL(8U, str.getSize());
    str.addField("c", *r.get("/int64_t"), 2);
    BOOST_REQUIRE_EQUAL(10U, str.getSize());
    str.addField("e", *r.get("/int64_t"), 0);
    BOOST_REQUIRE_EQUAL(10U, str.getSize());
}

BOOST_AUTO_TEST_CASE( test_equality )
{
    Registry ra, rb;

    //// Test numerics
    BOOST_REQUIRE(*ra.get("/int32_t") == *ra.get("/int32_t"));
    // same repository, same type, different names
    BOOST_REQUIRE(*ra.get("/int16_t") != *ra.get("/short"));
    // different repositories, same type
    BOOST_REQUIRE(*ra.get("/int32_t") != *rb.get("/int32_t"));
    BOOST_REQUIRE(ra.get("/int32_t")->isSame(*rb.get("/int32_t"))); 
    // same type but different names
    BOOST_REQUIRE(ra.get("/int16_t")->isSame(*rb.get("/short")));
    BOOST_REQUIRE(!ra.get("/int32_t")->isSame(*ra.get("/float"))); // numeric category differs
    BOOST_REQUIRE(!ra.get("/int32_t")->isSame(*ra.get("/uint32_t"))); // numeric category differs
    BOOST_REQUIRE(!ra.get("/int16_t")->isSame(*ra.get("/int32_t"))); // size differs

    //// Test arrays
    BOOST_REQUIRE(*ra.build("/int[16]") != *rb.build("/int[16]"));
    BOOST_REQUIRE(*ra.get("/int[16]") == *ra.get("/int[16]"));
    BOOST_REQUIRE(!ra.get("/int[16]")->isSame(*rb.build("/int")));
    BOOST_REQUIRE(ra.get("/int[16]")->isSame(*rb.build("/int[16]")));
    BOOST_REQUIRE(!ra.get("/int[16]")->isSame(*rb.build("/int[32]")));
    BOOST_REQUIRE(!ra.get("/int[16]")->isSame(*rb.build("/float[16]")));

    //// Test pointers
    BOOST_REQUIRE(*ra.build("/int*") != *rb.build("/int*"));
    BOOST_REQUIRE(*ra.get("/int*") == *ra.get("/int*"));
    BOOST_REQUIRE(ra.get("/int*")->isSame(*rb.build("/int*")));
    BOOST_REQUIRE(!ra.get("/int*")->isSame(*rb.build("/float*")));

    //// Test compounds
    Compound str_a("/A");
    str_a.addField("a", *ra.build("/int[16]"), 0);
    str_a.addField("b", *ra.build("/float*"), 1);
    str_a.addField("c", *ra.get("/int32_t"), 2);

    Compound str_b("/B");
    str_b.addField("a", str_a, 0);
    str_b.addField("b", *ra.build("/nil*"), 1);

    BOOST_REQUIRE(str_a.isSame(str_a));
    BOOST_REQUIRE(str_b.isSame(str_b));
    BOOST_REQUIRE(!str_a.isSame(str_b));

    // same type than str_a, to test checking on str_b
    Compound str_a_dup("/A");
    str_a_dup.addField("a", *rb.build("/int[16]"), 0);
    str_a_dup.addField("b", *rb.build("/float*"), 1);
    str_a_dup.addField("c", *rb.get("/int32_t"), 2);

    { Compound str_c("/C");
	// type name changed
	str_c.addField("a", str_a_dup, 0);
	str_c.addField("b", *rb.build("/nil*"), 1);
	BOOST_REQUIRE(!str_c.isSame(str_b));
    }

    { Compound str_c("/B");
	// field offsets changed
	// the structure size should remain the same
	// in order to really check offset checking
	str_c.addField("a", str_a_dup, 0);
	str_c.addField("b", *rb.build("/nil*"), 0);
	BOOST_REQUIRE_EQUAL(str_c.getSize(), str_b.getSize());
	BOOST_REQUIRE(!str_c.isSame(str_b));
    }

    { Compound str_c("/B");
	// field name change
	str_c.addField("a", str_a_dup, 0);
	str_c.addField("c", *rb.build("/nil*"), 1);
	BOOST_REQUIRE(!str_c.isSame(str_b));
    }
}

BOOST_AUTO_TEST_CASE( test_merge )
{
    Registry ra, rb;

    //// Add definitions of /A and /B in rb
    Compound* str_a = new Compound("/A");
    str_a->addField("a", *rb.build("/int[16]"), 0);
    str_a->addField("b", *rb.build("/float*"), 1);
    str_a->addField("c", *rb.get("/int32_t"), 2);
    rb.add(str_a, "");

    Compound* str_b = new Compound("/B");
    str_b->addField("a", *str_a, 0);
    str_b->addField("b", *rb.build("/nil*"), 1);
    rb.add(str_b, "");
    rb.alias("/A", "/C");
    rb.alias("/B", "/D");
    ra.merge(rb);

    BOOST_REQUIRE(ra.get("/int[16]") != rb.get("/int[16]"));
    BOOST_REQUIRE(ra.get("/int[16]")->isSame(*rb.get("/int[16]")));
    BOOST_REQUIRE(ra.get("/nil*") != rb.get("/nil*"));
    BOOST_REQUIRE(ra.get("/nil*")->isSame(*rb.get("/nil*")));
    BOOST_REQUIRE(ra.get("/A") != rb.get("/A"));
    BOOST_REQUIRE(ra.get("/A")->isSame(*str_a));
    BOOST_REQUIRE(ra.get("/B") != rb.get("/B"));
    BOOST_REQUIRE(ra.get("/B")->isSame(*str_b));
    BOOST_REQUIRE(*ra.get("/A") == *ra.get("/C"));
    BOOST_REQUIRE(*ra.get("/B") == *ra.get("/D"));

    BOOST_REQUIRE(static_cast<Compound const*>(ra.get("/A"))->getField("a")->getType() == *ra.get("/int[16]"));
    BOOST_REQUIRE(static_cast<Compound const*>(ra.get("/A"))->getField("b")->getType() == *ra.get("/float*"));
    BOOST_REQUIRE(static_cast<Compound const*>(ra.get("/A"))->getField("c")->getType() == *ra.get("/int32_t"));
    BOOST_REQUIRE(static_cast<Compound const*>(ra.get("/B"))->getField("a")->getType() == *ra.get("/A"));
    BOOST_REQUIRE(static_cast<Compound const*>(ra.get("/B"))->getField("b")->getType() == *ra.get("/nil*"));
    BOOST_REQUIRE(static_cast<Compound const*>(rb.get("/A"))->getField("a")->getType() == *rb.get("/int[16]"));
    BOOST_REQUIRE(static_cast<Compound const*>(rb.get("/A"))->getField("b")->getType() == *rb.get("/float*"));
    BOOST_REQUIRE(static_cast<Compound const*>(rb.get("/A"))->getField("c")->getType() == *rb.get("/int32_t"));
    BOOST_REQUIRE(static_cast<Compound const*>(rb.get("/B"))->getField("a")->getType() == *rb.get("/A"));
    BOOST_REQUIRE(static_cast<Compound const*>(rb.get("/B"))->getField("b")->getType() == *rb.get("/nil*"));
}
