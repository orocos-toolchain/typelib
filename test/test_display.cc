#include "testsuite.hh"
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/csvoutput.hh>
using namespace Typelib;
using namespace std;

#include "test_cimport.1"

class TC_Display
{
public:
    TC_Display() { }
    ~TC_Display() { }

    // Tests simple value handling
    void test_csv()
    {
        {
            Registry registry;

            float    f32  = 0.52;
            Type const& type = *registry.get("float");
            
            ostringstream stream;
            stream << csv_header(type, "base");
            BOOST_CHECK_EQUAL("base", stream.str());

            stream.str("");
            stream << csv(type, &f32);
            BOOST_CHECK_EQUAL("0.52", stream.str());
        }
        
        {
            // Get the test file into repository
            utilmm::config_set config;
            auto_ptr<Registry> registry(PluginManager::self()->load("c", TEST_DATA_PATH("test_cimport.1"), config));

            {
                ostringstream stream;
                
                A a = { 10, 20, 'b', 52 };
                Type const& a_type = *registry->get("/struct A");
                stream << csv_header(a_type, "a");
                BOOST_CHECK_EQUAL("a.a a.b a.c a.d", stream.str());
                
                stream.str("");
                stream << csv(a_type, &a);
                BOOST_CHECK_EQUAL("10 20 b 52", stream.str());
            }
        }
    }
    
};

void test_display(test_suite* ts) {
    boost::shared_ptr<TC_Display> instance( new TC_Display );
    ts->add( BOOST_CLASS_TEST_CASE( &TC_Display::test_csv, instance ) );
}

