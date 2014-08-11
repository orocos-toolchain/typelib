CONFIGURE_FILE(test_config.rb.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.rb @ONLY)

file(GLOB_RECURSE testfiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.rb)
foreach(test_file ${testfiles})
    set(test_requires "${test_requires}require '${test_file}';")
endforeach()
# running implicitly against the plugins installed in the install-location
ADD_TEST(NAME RubyInstalledPlugins
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND ${RUBY_EXECUTABLE} -w -I${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/ruby/lib
        -I${CMAKE_CURRENT_SOURCE_DIR}
        -I${CMAKE_CURRENT_BINARY_DIR} -rminitest/autorun -e "${test_requires}")

# the second ruby-test running explicitly against the freshly compiled plugins
# in the build folder
ADD_TEST(NAME RubyLocalPlugins
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND ${RUBY_EXECUTABLE} -w -I${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/ruby/lib
        -I${CMAKE_CURRENT_SOURCE_DIR}
        -I${CMAKE_CURRENT_BINARY_DIR} -rminitest/autorun -e "${test_requires}")
# the pluginmanager can load plugins from a list of paths set in the
# environment. for testing we don't want to use the plugins in the
# install-folder, but the freshly compiled ones in the build-folder. note that
# the folders set in the env-var are hardcoded...
set_property(TEST RubyLocalPlugins APPEND PROPERTY ENVIRONMENT
    "TYPELIB_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lang/csupport:${CMAKE_BINARY_DIR}/lang/tlb:${CMAKE_BINARY_DIR}/lang/idl")
# prepend another RUBYLIB to the already existing list, so that all the other
# rock-depenencies are found.
set_property(TEST RubyLocalPlugins APPEND PROPERTY ENVIRONMENT
    "RUBYLIB=${CMAKE_BINARY_DIR}/bindings/ruby:$ENV{RUBYLIB}")
# there are no typelib-plugins inside _this_ single project which we might
# wanna test
set_property(TEST RubyLocalPlugins APPEND PROPERTY ENVIRONMENT
    "TYPELIB_RUBY_PLUGIN_PATH=")
