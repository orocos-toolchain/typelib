CONFIGURE_FILE(testsuite.hh.in test/testsuite.hh @ONLY)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../lang/cimport ${CMAKE_CURRENT_BINARY_DIR}/../lang/cimport)
ADD_EXECUTABLE(typelib_testsuite testsuite.cc
    test_registry.cc
    test_display.cc test_lang_c.cc test_lang_tlb.cc test_model.cc
    test_plugin.cc test_value.cc)

TARGET_LINK_LIBRARIES(typelib_testsuite typeLib ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})
ADD_TEST(CxxSuite ${CMAKE_CURRENT_BINARY_DIR}/typelib_testsuite)

