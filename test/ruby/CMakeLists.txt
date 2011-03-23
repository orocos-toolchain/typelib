INCLUDE_DIRECTORIES(${RUBY_INCLUDE_PATH})
ADD_LIBRARY(test_ruby MODULE test_rb_value.cc)
target_link_libraries(test_ruby ${RUBY_LIBRARY})

CONFIGURE_FILE(test_config.rb.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.rb @ONLY)

ADD_TEST(Ruby /usr/bin/env testrb ${CMAKE_CURRENT_SOURCE_DIR} -x marshal)

