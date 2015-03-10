include(LLVMTools)

# we tested and developed _only_ against llvm-3.4
llvm_find_config(3.4)

# note that "cxxflags" additionally returns the "-I" flags... which might not
# be what you want!
llvm_get_config(LLVM_CXXFLAGS "--cxxflags")
string(REGEX REPLACE "-I[^ ]+" "" LLVM_CXXFLAGS ${LLVM_CXXFLAGS})

llvm_get_config(LLVM_LIBRARY_DIRS "--libdir")

# tell cmake where to look for includes
llvm_get_config(LLVM_INCLUDE_DIRS "--includedir")

# The set of clang libraries we need ... no clang-config for us
unset(LLVM_RAW_LIBRARIES CACHE)
list(APPEND LLVM_RAW_LIBRARIES
    clangEdit clangTooling clangDriver
    clangFrontend clangLex clangParse
    clangAnalysis clangSema clangRewriteCore
    clangSerialization clangEdit clangAST
    clangASTMatchers clangBasic clang)

llvm_get_config(llvm_config_libs "--libs"
    core option analysis support bitreader mcparser transformutils)
# Convert the library as reported by llvm-config into a cmake list
string(REPLACE "-l" "" llvm_config_libs "${llvm_config_libs}")
string(REPLACE " " ";" llvm_config_libs "${llvm_config_libs}")
# and add it to LLVM_RAW_LIBRARIES
list(APPEND LLVM_RAW_LIBRARIES ${llvm_config_libs})

foreach(llvm_lib ${LLVM_RAW_LIBRARIES})
    unset(llvm_lib_path CACHE)
    find_library(llvm_lib_path ${llvm_lib} PATHS ${LLVM_LIBRARY_DIRS})
    if (NOT llvm_lib_path)
        message(STATUS "${llvm_lib} not found in ${LLVM_LIBRARY_DIRS}")
        set(LLVM_LIBRARIES FALSE)
        break()
    else()
        list(APPEND LLVM_LIBRARIES ${llvm_lib_path})
    endif()
endforeach()

if (LLVM_LIBRARIES)
    add_definitions(${LLVM_CXXFLAGS})
    include_directories(${LLVM_INCLUDE_DIRS})

    find_package(Threads)

    pkg_check_modules(TINFO REQUIRED tinfo)
    add_definitions(${TINFO_CFLAGS})
    include_directories(${TINFO_INCLUDE_DIRS})

    find_package(DL)
    include_directories(${LIBDL_INCLUDE_DIR})

    # prevent loud warnings from gcc-4.9 caused by a macro-expansion the
    # AST-headers... this only clutters up the compiler output with nonsense
    add_definitions("-Wno-unused-local-typedefs")

    # when compiling inside the rock-framework "Wall" is used. This adds alotta
    # more bogus warnings from Debians llvm-3.4 headers...
    add_definitions("-Wno-strict-aliasing")

    add_executable(typelib-clang-tlb-importer
        main.cpp
        TypelibBuilder.cpp
        NamingConversions.cpp
        HandleMetaData.cpp
        IgnoredOrRenamedTypes.cpp
        )


    target_link_libraries(typelib-clang-tlb-importer
        ${LLVM_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${TINFO_LIBRARIES}
        ${LIBDL_LIBRARIES}
        typeLib
        typeLang_tlb
        )

    add_subdirectory(test)

    # also install the created tool, so that the ruby-binding can execute it via
    # "system()" during tlb-creation time
    install(TARGETS typelib-clang-tlb-importer
        RUNTIME DESTINATION bin
        )
else()
    message(STATUS "clang not found, the typelib-clang importer will not be built")
endif()

