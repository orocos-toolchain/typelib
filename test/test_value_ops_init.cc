#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <typelib/utilmm/configset.hh>
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

enum TestEnum { TEST = 10 };
BOOST_AUTO_TEST_CASE(test_init_initializes_enums)
{
    Enum enum_t("/Test", 0);
    enum_t.add("TEST", 10);
    Compound compound_t("/CTest");
    compound_t.addField("test", enum_t, 5);

    std::vector<uint8_t> buffer;
    buffer.resize(compound_t.getSize());
    Typelib::init(Value(&buffer[0], compound_t));
    BOOST_REQUIRE_EQUAL(10, *reinterpret_cast<TestEnum*>(&buffer[5]));
}

BOOST_AUTO_TEST_CASE(test_init_initializes_enums_negative)
{
    Enum enum_t("/Test", 0);
    enum_t.add("TEST", -10);
    Compound compound_t("/CTest");
    compound_t.addField("test", enum_t, 5);

    std::vector<uint8_t> buffer;
    buffer.resize(compound_t.getSize());
    Typelib::init(Value(&buffer[0], compound_t));
    BOOST_REQUIRE_EQUAL(-10, *reinterpret_cast<TestEnum*>(&buffer[5]));
}


