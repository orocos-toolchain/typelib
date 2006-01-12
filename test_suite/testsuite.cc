#include "testsuite.hh"
#include <boost/test/unit_test_log.hpp>
#include <iostream>

test_suite*
init_unit_test_suite( int argc, char * argv[] ) {
    set_log_threshold_level(log_successful_tests);
    test_suite* ts = BOOST_TEST_SUITE( "Testing Typelib" );

    // Check core library behaviour
    test_suite* ts_core = BOOST_TEST_SUITE( "Testing core" );
    test_plugins(ts_core);
    ts->add(ts_core);

    // Check system first, it is used by configfile
    test_suite* ts_lang_c = BOOST_TEST_SUITE( "Testing C import" );
    test_lang_c(ts_lang_c);
    ts->add(ts_lang_c);

//    test_suite* ts_value = BOOST_TEST_SUITE( "Testing in-memory value manipulation" );
//    test_value(ts_value);
//    ts->add(ts_value);

    return ts;
}

