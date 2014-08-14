# this merger is kinda fragile... _can_ "throw" with "undefined type" of stuff
# like "/int"... well... was worth a try... hey, what could possibly go wrong,
# merging two xml-files? how hard could it be...
#
# handpicked two tlb-files that work...
add_test(NAME create-typelib-tlb-merger-result
    COMMAND typelib-tlb-merger
        ${CMAKE_SOURCE_DIR}/tools/typelib-clang-tlb-importer/test/arrays.tlb
        ${CMAKE_SOURCE_DIR}/tools/typelib-clang-tlb-importer/test/typedefs.tlb
        ${CMAKE_CURRENT_BINARY_DIR}/test-merge-result.tlb
    )
add_test(NAME compare-typelib-tlb-merger-result
    COMMAND diff -u
        ${CMAKE_CURRENT_BINARY_DIR}/test-merge-result.tlb
        ${CMAKE_CURRENT_SOURCE_DIR}/test-merge-result.tlb
    )
# one tests depending on another: kudos to
# http://stackoverflow.com/a/10824578/3520187
set_tests_properties(compare-typelib-tlb-merger-result
    PROPERTIES DEPENDS create-typelib-tlb-merger-result
    )
