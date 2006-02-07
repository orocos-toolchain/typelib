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
    test_value(ts_core);
    ts->add(ts_core);

    test_suite* ts_lang = BOOST_TEST_SUITE( "Testing language plugins" );
    test_lang_tlb(ts_lang);
    test_lang_c(ts_lang);
    ts->add(ts_lang);

    test_suite* ts_display = BOOST_TEST_SUITE( "Testing value display plugins" );
    test_display(ts_lang);
    ts->add(ts_display);

    return ts;
}

