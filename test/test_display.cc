#include <boost/test/auto_unit_test.hpp>

#include "testsuite.hh"
#include <utilmm/configfile/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/csvoutput.hh>
using namespace Typelib;
using namespace std;

#include "test_cimport.1"


BOOST_AUTO_TEST_CASE(test_csv)
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
	    
	    DisplayTest test;
	    test.fields[0] = 0;
	    test.fields[1] = 1;
	    test.fields[2] = 2;
	    test.fields[3] = 3;
	    test.f = 1.1;
	    test.d = 2.2;
	    test.a.a = 10;
	    test.a.b = 20;
	    test.a.c = 'b';
	    test.a.d = 42;
	    test.mode = OUTPUT;

	    Type const& display_type = *registry->get("/struct DisplayTest");
	    stream << csv_header(display_type, "t");
	    BOOST_CHECK_EQUAL("t.fields[0] t.fields[1] t.fields[2] t.fields[3] t.f t.d t.a.a t.a.b t.a.c t.a.d t.mode", stream.str());
	    
	    stream.str("");
	    stream << csv(display_type, &test);
	    BOOST_CHECK_EQUAL("0 1 2 3 1.1 2.2 10 20 b 42 OUTPUT", stream.str());
	}
    }
}

