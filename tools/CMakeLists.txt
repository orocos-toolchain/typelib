include(LLVMTools)
llvm_find_config(3.4)
if (LLVM_CONFIG_EXECUTABLE)
    add_subdirectory(typelib-clang-tlb-importer)
else()
    message(STATUS "could not find llvm-config, disabling the typelib/clang importer. Install clang 3.4 libraries and development files and try again")
endif()

add_subdirectory(typelib-tlb-merger)
