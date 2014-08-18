
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
file(GLOB TEST_HEADER_DATA "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
# and create a custom target for the corresponding tlb-file created from these headers
foreach(HEADER_FILE ${TEST_HEADER_DATA})
    # we need the actual header-name
    get_filename_component(HEADER_NAME ${HEADER_FILE} NAME_WE)
    set(TLB_FILE "${CMAKE_CURRENT_BINARY_DIR}/${HEADER_NAME}.tlb")
    set(TLB_OPTION "-tlbSavePath=${CMAKE_CURRENT_BINARY_DIR}/${HEADER_NAME}.tlb")
    # we allow optionally loading opaque definitions is the is a files with the
    # same basename but the ending "opaques".
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_NAME}.opaques")
        set(OPAQUE_OPTION "-opaquePath=${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_NAME}.opaques")
    else()
        set(OPAQUE_OPTION "")
    endif()
    # call the "typelib-clang-tlb-importer" tool, give it the header and pipe
    # stdout into the new tlb-file hardcoded hack for eigen ;-)
    if (${HEADER_NAME} STREQUAL "templates")
	set(ADDITIONAL_TEST_FLAGS "-I/usr/include/eigen3")
    endif (${HEADER_NAME} STREQUAL "templates")
    add_custom_target(create-test-${HEADER_NAME}.tlb
        # if the "typelib-clang-tlb-importer" throws while parsing a header this is an error!
        COMMAND ${CMAKE_BINARY_DIR}/tools/typelib-clang-tlb-importer/typelib-clang-tlb-importer
            ${HEADER_FILE}
            ${OPAQUE_OPTION}
            ${TLB_OPTION}
            -- -x c++ ${ADDITIONAL_TEST_FLAGS}
        DEPENDS typelib-clang-tlb-importer
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
        COMMAND diff -u ${TLB_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_NAME}.tlb
        )
endforeach(HEADER_FILE)
