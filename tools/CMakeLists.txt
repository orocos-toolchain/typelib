include(LLVMTools)
llvm_find_config(3.4)
if (LLVM_CONFIG_EXECUTABLE)
    add_subdirectory(typelib-clang-tlb-importer)
endif()

add_subdirectory(typelib-tlb-merger)
