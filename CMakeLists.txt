if(COMMAND cmake_minimum_required)
    cmake_minimum_required(VERSION 2.4)
endif(COMMAND cmake_minimum_required)

PROJECT(typeLib)
SET(PROJECT_VERSION 1.1)

SET (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
INCLUDE(UsePkgConfig)
INCLUDE(RPATHHandling)
INCLUDE(CTest)
INCLUDE(GenerateDoxygenDoc)

# Make cmake ignore mixed absolute and -l flags for linker.  I don't want to be
# compatible only with 2.6 for now
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 OLD)
endif(COMMAND cmake_policy)

CMAKE_USE_FULL_RPATH("${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/lib/typelib")

PKGCONFIG(utilmm Utilmm_INCLUDE_DIR Utilmm_LINK_DIR Utilmm_LIBRARIES Utilmm_CFLAGS)
IF(NOT Utilmm_LIBRARIES)
    MESSAGE(FATAL_ERROR "please install util-- before building typelib")
ENDIF(NOT Utilmm_LIBRARIES)
INCLUDE_DIRECTORIES(${Utilmm_INCLUDE_DIR})
LINK_DIRECTORIES(${Utilmm_LINK_DIR})

PKGCONFIG(libxml-2.0 LibXML_INCLUDE_DIR LibXML_LINK_DIR LibXML_LIBRARIES LibXML_CFLAGS)
IF(NOT LibXML_LIBRARIES)
    MESSAGE(FATAL_ERROR "please install libxml-2.0 before building typelib")
ENDIF(NOT LibXML_LIBRARIES)

CONFIGURE_FILE(typelib.pc.in typelib.pc @ONLY)
INSTALL(FILES ${CMAKE_BINARY_DIR}/typelib.pc DESTINATION lib/pkgconfig)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/typelib)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_BINARY_DIR})

INCLUDE(RubyExtensions)
IF(RUBY_EXTENSIONS_AVAILABLE)
    ADD_SUBDIRECTORY(bindings/ruby)
    ADD_SUBDIRECTORY(test/ruby)
ENDIF(RUBY_EXTENSIONS_AVAILABLE)

FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
IF (Boost_UNIT_TEST_FRAMEWORK_FOUND)
    MESSAGE(STATUS "boost/test found ... building test suite")
    ADD_SUBDIRECTORY(test)
ENDIF(Boost_UNIT_TEST_FRAMEWORK_FOUND)

ADD_SUBDIRECTORY(lang/cimport)
ADD_SUBDIRECTORY(lang/tlb)
ADD_SUBDIRECTORY(lang/idl)
ADD_SUBDIRECTORY(typelib)

