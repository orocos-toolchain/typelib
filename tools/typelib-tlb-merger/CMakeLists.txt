add_executable(typelib-tlb-merger
    typelib-tlb-merger.cpp
    )

target_link_libraries(typelib-tlb-merger
    typeLib
    typeLang_tlb
    typeLang_cSupport
    ${CMAKE_THREAD_LIBS_INIT}
    )

add_subdirectory(test)
