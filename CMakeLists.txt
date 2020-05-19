if(COMMAND cmake_minimum_required)
    cmake_minimum_required(VERSION 2.4)
endif(COMMAND cmake_minimum_required)

PROJECT(typelib)
SET(PROJECT_VERSION 1.1)
SET(API_VERSION 1)

#required because of the use of std::unique_ptr
set (CMAKE_CXX_STANDARD 11)

FIND_PACKAGE ( Threads REQUIRED )

# This changes the behaviour of list() w.r.t. empty elements. Typelib'cmake code
# is compatible with the NEW policy, so just set it to avoid getting a cmake
# warning
cmake_policy(SET CMP0007 NEW)
add_compile_options(-Werror=return-type)

# make the clang-based importer optional for now. the option is planned to be
# removed in the future, after using the new importer as default.
option(BUILD_CLANG_TLB_IMPORTER
    "set to ON to enable compiling of the llvm/clang based importer"
    OFF)

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
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

add_definitions(-Wall)
CMAKE_USE_FULL_RPATH("${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/typelib")

IF(APPLE)
  SET(CMAKE_INSTALL_NAME_DIR @rpath)
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
ENDIF(APPLE)

pkg_search_module (LibXML  libxml-2.0 libxml2 libxml>=2)
IF(NOT LibXML_FOUND)
    MESSAGE(FATAL_ERROR "please install libxml-2.0 before building typelib")
ELSE(NOT LibXML_FOUND)
    #adapt from the old pkgconfig cmake util to the new)
    set(LibXML_INCLUDE_DIR ${LibXML_INCLUDE_DIRS})
    set(LibXML_LINK_DIR ${LibXML_LIBRARY_DIRS})
    set(LibXML_LIBRARIES ${LibXML_LIBRARIES})

    INCLUDE_DIRECTORIES(${LibXML_INCLUDE_DIR})
    LINK_DIRECTORIES(${LibXML_LIBDIR})
ENDIF(NOT LibXML_FOUND)

CONFIGURE_FILE(typelib.pc.in typelib.pc @ONLY)
INSTALL(FILES ${CMAKE_BINARY_DIR}/typelib.pc DESTINATION lib/pkgconfig)

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
ADD_SUBDIRECTORY(lang/tlb)
ADD_SUBDIRECTORY(lang/idl)
ADD_SUBDIRECTORY(typelib)


add_subdirectory(tools)
