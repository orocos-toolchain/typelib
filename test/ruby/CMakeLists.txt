INCLUDE_DIRECTORIES(${RUBY_INCLUDE_PATH})
ADD_LIBRARY(test_ruby MODULE test_rb_value.cc)
target_link_libraries(test_ruby ${RUBY_LIBRARY})

CONFIGURE_FILE(test_config.rb.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.rb @ONLY)

file(GLOB_RECURSE testfiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.rb)
foreach(test_file ${testfiles})
    set(test_requires "${test_requires}require '${test_file}';")
endforeach()
ADD_TEST(NAME Ruby
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND ${RUBY_EXECUTABLE} -w -I${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/ruby/lib
        -I${CMAKE_CURRENT_SOURCE_DIR}
        -I${CMAKE_CURRENT_BINARY_DIR} -rminitest/autorun -e "${test_requires}")

