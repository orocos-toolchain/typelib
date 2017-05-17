#include <boost/test/auto_unit_test.hpp>

#include <test/testsuite.hh>
#include <typelib/pluginmanager.hh>

using namespace Typelib;

BOOST_AUTO_TEST_CASE( test_manager )
{
    PluginManager &manager(PluginManager::getInstance());
    BOOST_REQUIRE_THROW(manager.importer("BLAH"), PluginNotFound);
}

