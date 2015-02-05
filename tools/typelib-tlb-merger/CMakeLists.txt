add_executable(typelib-tlb-merger
    typelib-tlb-merger.cpp
    )

target_link_libraries(typelib-tlb-merger
    typeLib
    typeLang_tlb
    typeLang_cSupport
    )

add_subdirectory(test)
