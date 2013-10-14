if(COMMAND cmake_minimum_required)
    cmake_minimum_required(VERSION 2.4)
endif(COMMAND cmake_minimum_required)

PROJECT(typelib)
SET(PROJECT_VERSION 1.1)
SET(API_VERSION 1)

IF(APPLE)
  set(CMAKE_SHARED_MODULE_SUFFIX ".bundle")
ENDIF(APPLE)

SET (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
INCLUDE(FindPkgConfig)
INCLUDE(RPATHHandling)
INCLUDE(CTest)
INCLUDE(GenerateDoxygenDoc)

# Make cmake ignore mixed absolute and -l flags for linker.  I don't want to be
# compatible only with 2.6 for now
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 OLD)
endif(COMMAND cmake_policy)

add_definitions(-Wall)
CMAKE_USE_FULL_RPATH("${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/lib/typelib")

pkg_search_module (Utilmm Utilmm UtilMM utilmm)
IF(NOT Utilmm_FOUND)
    MESSAGE(FATAL_ERROR "please install util-- before building typelib")
ELSE(NOT Utilmm_FOUND)
    #adapt from the old pkgconfig cmake util to the new)
    find_library(Utilmm_LIBRARIES utilmm HINTS ${Utilmm_LIBRARY_DIRS})
    set(Utilmm_INCLUDE_DIR ${Utilmm_INCLUDE_DIRS})
    set(Utilmm_LINK_DIR ${Utilmm_LIBRARY_DIRS})
    #set(Utilmm_LIBRARIES ${Utilmm_LIBRARIES})
    
    INCLUDE_DIRECTORIES(${Utilmm_INCLUDE_DIR})
    LINK_DIRECTORIES(${Utilmm_LINK_DIR})
ENDIF(NOT Utilmm_FOUND)

pkg_search_module (LibXML  libxml-2.0 libxml2 libxml>=2)
IF(NOT LibXML_FOUND)
    MESSAGE(FATAL_ERROR "please install libxml-2.0 before building typelib")
ELSE(NOT LibXML_FOUND)
    #adapt from the old pkgconfig cmake util to the new)
    set(LibXML_INCLUDE_DIR ${LibXML_INCLUDE_DIRS})
    set(LibXML_LINK_DIR ${LibXML_LIBRARY_DIRS})
    set(LibXML_LIBRARIES ${LibXML_LIBRARIES})
    
    INCLUDE_DIRECTORIES(${LibXML_INCLUDE_DIR})
    LINK_DIRECTORIES(${LibXML_LINK_DIR})
ENDIF(NOT LibXML_FOUND)

CONFIGURE_FILE(typelib.pc.in typelib.pc @ONLY)
INSTALL(FILES ${CMAKE_BINARY_DIR}/typelib.pc DESTINATION lib/pkgconfig)
INSTALL(FILES package.xml DESTINATION share/typelib)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/typelib)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_BINARY_DIR})

FIND_PACKAGE(Boost COMPONENTS unit_test_framework system filesystem)
IF(NOT Boost_FOUND)
    MESSAGE(FATAL_ERROR "please install boost or check BOOST_ROOT setting -- before building typelib")
ELSE(NOT Boost_FOUND)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
ENDIF()

INCLUDE(RubyExtensions)
IF(RUBY_EXTENSIONS_AVAILABLE)
    ADD_SUBDIRECTORY(bindings/ruby)
    ADD_SUBDIRECTORY(test/ruby)
    if (DOXYGEN_FOUND)
      ADD_DEPENDENCIES(doc ruby_doc)
    endif (DOXYGEN_FOUND)
ENDIF(RUBY_EXTENSIONS_AVAILABLE)

IF (Boost_UNIT_TEST_FRAMEWORK_FOUND)
    MESSAGE(STATUS "boost/test found ... building test suite")
    ADD_SUBDIRECTORY(test)
ENDIF(Boost_UNIT_TEST_FRAMEWORK_FOUND)

ADD_SUBDIRECTORY(lang/csupport)
OPTION(ENABLE_INTERNAL_CPARSER "set to ON to enable typelib's internal C/C++ parser. This is superseded by the gccxml support in the Ruby interface" OFF)
if (ENABLE_INTERNAL_CPARSER)
    ADD_SUBDIRECTORY(lang/cimport)
endif()
ADD_SUBDIRECTORY(lang/tlb)
ADD_SUBDIRECTORY(lang/idl)
ADD_SUBDIRECTORY(typelib)

