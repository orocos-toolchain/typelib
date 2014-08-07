cmake_minimum_required(VERSION 2.8)

# prevent loud warnings from gcc-4.9 caused by a macro-expansion the
# AST-headers... this only clutters up the compiler output with nonsense
add_definitions("-Wno-unused-local-typedefs")

# when compiling inside the rock-framework "Wall" is used. This adds alotta
# more bogus warnings from system-headers...
add_definitions("-Wno-strict-aliasing")

macro(llvm_find_config LLVM_REQUIRED_VERSION)
    if (LLVM_REQUIRED_VERSION STREQUAL "")
        list(APPEND __llvm_names llvm-config-3.6 llvm-config-3.5 llvm-config-3.4)
    else()
        list(APPEND __llvm_names llvm-config-${LLVM_REQUIRED_VERSION})
    endif()
    message("-- llvm: looking for ${__llvm_names}")
    find_program(LLVM_CONFIG_EXECUTABLE NAMES ${__llvm_names})
    if (NOT LLVM_CONFIG_EXECUTABLE)
        message(FATAL_ERROR "could not find the llvm-config script (candidates: ${__llvm_names})")
    endif()
    message("-- llvm: found ${LLVM_CONFIG_EXECUTABLE}")
endmacro()

macro(llvm_get_config OUTPUT)
    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} ${ARGN}
        RESULT_VARIABLE __llvm_ok
        OUTPUT_VARIABLE ${OUTPUT})
    if (NOT __llvm_ok STREQUAL 0)
        message(FATAL_ERROR "failed to execute llvm-config ${ARGN} (result: ${__llvm_ok})")
    endif()
    string(STRIP ${OUTPUT} "${${OUTPUT}}")
    string(REPLACE "\n" "" ${OUTPUT} "${${OUTPUT}}")
endmacro()

macro(resolve_library_flags VARIABLE)
    foreach(${VARIABLE}_lib ${${VARIABLE}_LIBRARIES})
      set(_${VARIABLE}_lib NOTFOUND)
      string(REPLACE "^-l" "" ${VARIABLE}_lib ${${VARIABLE}_lib})
      find_library(_${VARIABLE}_lib NAMES ${${VARIABLE}_lib} HINTS ${${VARIABLE}_LIBRARY_DIRS})
      if (NOT _${VARIABLE}_lib)
        set(_${VARIABLE}_lib ${${VARIABLE}_lib})
      endif()
      list(APPEND _${VARIABLE}_LIBRARIES ${_${VARIABLE}_lib})
    endforeach()
    list(APPEND _${VARIABLE}_LIBRARIES ${${VARIABLE}_LDFLAGS_OTHER})
    set(${VARIABLE}_LIBRARIES ${_${VARIABLE}_LIBRARIES} CACHE INTERNAL "")
endmacro()

# we tested and developed _only_ against llvm-3.4
llvm_find_config(3.4)

# note that "cxxflags" additionally returns the "-I" flags... which might not
# be what you want!
llvm_get_config(LLVM_CXXFLAGS "--cxxflags")
string(REGEX REPLACE "-I[^ ]+" "" LLVM_CXXFLAGS ${LLVM_CXXFLAGS})

llvm_get_config(LLVM_LIBRARY_DIRS "--libdir")
llvm_get_config(LLVM_LIBRARIES "--libs"
    core option analysis support bitreader mcparser transformutils)

# tell cmake where to look for includes
llvm_get_config(LLVM_INCLUDE_DIRS "--includedir")
include_directories(${LLVM_INCLUDE_DIRS})

# Convert the library as reported by llvm-config into a cmake list
string(REPLACE " " ";" LLVM_LIBRARIES "${LLVM_LIBRARIES}")

# The set of clang libraries we need ... no clang-config for us
list(APPEND CLANG_LIBRARIES
    clangEdit clangTooling clangDriver
    clangFrontend clangLex clangParse
    clangAnalysis clangSema clangRewriteCore
    clangSerialization clangEdit clangAST
    clangASTMatchers clangBasic clang)

add_definitions(${LLVM_CXXFLAGS})
link_directories(${LLVM_LIBRARY_DIRS})

find_package(Threads)

find_package(PkgConfig)
pkg_check_modules(TINFO REQUIRED tinfo)
add_definitions(${TINFO_CFLAGS})
include_directories(${TINFO_INCLUDE_DIRS})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(DL)
include_directories(${LIBDL_INCLUDE_DIR})

add_executable(tlbBuilder
    main.cpp
    TypelibBuilder.cpp
    )


target_link_libraries(tlbBuilder
    ${CLANG_LIBRARIES}
    ${LLVM_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TINFO_LIBRARIES}
    ${LIBDL_LIBRARIES}
    typeLib
    typeLang_tlb
    )

# for the time beeing, until all this is properly implemented into the
# existinging testing "conventions" of this project (if i might be so bold to
# call them so...) we need a way to easily and reproducibly test a set of
# headers.
#
# create a dummy target which depends on all the single test-headers
add_custom_target(create-test-header)
# circumvent the cmake-quirk that targets for testing are not part of all...
# http://stackoverflow.com/a/736838/3520187
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS create-test-header)

# now glob all test-headers
file(GLOB TEST_HEADER_DATA "${CMAKE_CURRENT_SOURCE_DIR}/test-header-data/*.h")
# and create a custom target for the corresponding tlb-file created from these headers
foreach(HEADER_FILE ${TEST_HEADER_DATA})
    # we need the actual header-name
    get_filename_component(HEADER_NAME ${HEADER_FILE} NAME_WE)
    set(TLB_FILE "${CMAKE_CURRENT_BINARY_DIR}/${HEADER_NAME}.tlb")
    # call the "tlbBuilder" tool, give it the header and pipe stdout into the new tlb-file
    # hardcoded hack for eigen ;-)
    if (${HEADER_NAME} STREQUAL "templates")
	set(ADDITIONAL_TEST_FLAGS "-I/usr/include/eigen3")
    endif (${HEADER_NAME} STREQUAL "templates")
    add_custom_target(create-test-${HEADER_NAME}.tlb
        # the "tlbBuilder" is not allowed to throw while parsing the header!
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/tlbBuilder ${HEADER_FILE} -tlbSavePath=${TLB_FILE} -- -x c++ ${ADDITIONAL_TEST_FLAGS}
        DEPENDS tlbBuilder
        )
    # add it to the target
    add_dependencies(create-test-header create-test-${HEADER_NAME}.tlb)
    #
    # aaand compare the created tlb to the one stored in VCS. we assume that
    # the ones checked in are "good" so we just look if they are the same...
    # crude, but effective. so we can at least point to a header file if
    # something in the tlb-creation changed -- in a good or bad way. if the
    # change is good just place the new tlb-file besides the respective header
    # in the VCS and rerun the test
    add_test(NAME compare-${HEADER_NAME}.tlb
        COMMAND diff -u ${CMAKE_CURRENT_BINARY_DIR}/${HEADER_NAME}.tlb ${CMAKE_CURRENT_SOURCE_DIR}/test-header-data/${HEADER_NAME}.tlb
        )
endforeach(HEADER_FILE)
