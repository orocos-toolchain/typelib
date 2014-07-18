cmake_minimum_required(VERSION 2.8)

# prevent loud warnings from gcc-4.9 caused by a macro-expansion the
# AST-headers... this only clutters up the compiler output with nonsense
add_definitions("-Wno-unused-local-typedefs")

include_directories("/usr/lib/llvm-3.4/include/")

macro(llvm_find_config LLVM_REQUIRED_VERSION)
    message("REQUIRED ${LLVM_REQUIRED_VERSION}")
    if (LLVM_REQUIRED_VERSION STREQUAL "")
        list(APPEND __llvm_names llvm-config-3.6 llvm-config-3.5 llvm-config-3.4)
    else()
        list(APPEND __llvm_names llvm-config-${LLVM_REQUIRED_VERSION})
    endif()
    message("NAMES ${__llvm_names}")
    find_program(LLVM_CONFIG_EXECUTABLE NAMES ${__llvm_names})
    if (NOT LLVM_CONFIG_EXECUTABLE)
        message(FATAL_ERROR "could not find the llvm-config script (candidates: ${__llvm_names})")
    endif()
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

llvm_find_config(3.4)
message(${LLVM_CONFIG_EXECUTABLE})
llvm_get_config(LLVM_CXXFLAGS "--cxxflags")
llvm_get_config(LLVM_LIBRARY_DIRS "--libdir")
llvm_get_config(LLVM_LIBRARIES "--libs" core option analysis support bitreader mcparser transformutils)

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
resolve_library_flags(LLVM)
resolve_library_flags(CLANG)

find_package(Threads)

find_package(PkgConfig)
pkg_check_modules(TINFO REQUIRED tinfo)
add_definitions(${TINFO_CFLAGS})
include_directories(${TINFO_INCLUDE_DIRS})
resolve_library_flags(TINFO)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(DL)
include_directories(${LIBDL_INCLUDE_DIR})

add_executable(extractor
    Extractor.cpp
    TypelibBuilder.cpp
    )


target_link_libraries(extractor
    ${CLANG_LIBRARIES}
    ${LLVM_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TINFO_LIBRARIES}
    ${LIBDL_LIBRARIES}
    typeLib
    )

