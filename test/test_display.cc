#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <lang/csupport/standard_types.hh>

#include <test/testsuite.hh>
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
        Typelib::CXX::addStandardTypes(registry);

	float    f32  = 0.52;
	Type const& type = *registry.get("float");
	
	ostringstream stream;
	stream << csv_header(type, "base");
	BOOST_CHECK_EQUAL("base", stream.str());

	stream.str("");
	stream << csv(type, &f32);
	BOOST_CHECK_CLOSE(0.52f, boost::lexical_cast<float>(stream.str()), 0.00001);
    }
    
    {
	// Get the test file into repository
	utilmm::config_set config;
	auto_ptr<Registry> registry(PluginManager::self()->load("tlb", TEST_DATA_PATH("test_cimport.tlb"), config));

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

	    Type const& display_type = *registry->get("/DisplayTest");
	    stream << csv_header(display_type, "t");
	    BOOST_CHECK_EQUAL("t.fields[0] t.fields[1] t.fields[2] t.fields[3] t.f t.d t.a.a t.a.b t.a.c t.a.d t.mode", stream.str());
	}
    }
}

