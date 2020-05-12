#include <boost/test/unit_test.hpp>

#include <test/testsuite.hh>
#include <typelib/pluginmanager.hh>

using namespace Typelib;

BOOST_AUTO_TEST_CASE( test_manager )
{
    PluginManager::self manager;
    BOOST_REQUIRE_THROW(manager->importer("BLAH"), PluginNotFound);
}

