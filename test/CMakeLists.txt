FIND_PACKAGE(Antlr REQUIRED COMPONENTS CPP)

set_source_files_properties(test_value.cc PROPERTIES
    COMPILE_FLAGS -fno-strict-aliasing)
set_source_files_properties(test_containers.cc PROPERTIES
    COMPILE_FLAGS -fno-strict-aliasing)

CONFIGURE_FILE(testsuite.hh.in ${CMAKE_BINARY_DIR}/test/testsuite.hh @ONLY)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../lang/cimport ${CMAKE_CURRENT_BINARY_DIR}/../lang/cimport)
if (ENABLE_INTERNAL_CPARSER)
    set(TEST_LANG_C test_lang_c.cc)
endif()
ADD_EXECUTABLE(typelib_testsuite testsuite.cc
    test_registry.cc
    test_display.cc test_lang_tlb.cc test_model.cc
    test_plugin.cc test_value.cc test_containers.cc
    test_memory_layout.cc test_marshalling.cc ${TEST_LANG_C})

TARGET_LINK_LIBRARIES(typelib_testsuite typeLib typeLang_cSupport ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} ${Boost_SYSTEM_LIBRARIES})
if (ENABLE_INTERNAL_CPARSER)
    TARGET_LINK_LIBRARIES(typelib_testsuite typeLang_c)
endif()

ADD_TEST(CxxSuite ${CMAKE_CURRENT_BINARY_DIR}/typelib_testsuite --catch_system_errors=no)

