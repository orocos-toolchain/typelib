#include "testsuite.hh"
#include <typelib/pluginmanager.hh>

using namespace Typelib;

class TC_Plugins
{
public:
    TC_Plugins() { }
    ~TC_Plugins() { }

    void test_manager() 
    {
        PluginManager::self manager;
        BOOST_REQUIRE_THROW(manager->importer("BLAH"), PluginNotFound);
    }
};

void test_plugins( test_suite* ts )
{
    boost::shared_ptr<TC_Plugins> instance( new TC_Plugins );
    ts->add( BOOST_CLASS_TEST_CASE( &TC_Plugins::test_manager, instance ) );
}

