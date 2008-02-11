INCLUDE(RubyExtensions)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ADD_RUBY_EXTENSION(typelib_ruby 
    ext/typelib_ruby.cc ext/value.cc ext/strings.cc
    ext/convert.cc ext/functions.cc ext/specialized_types.cc 
    ext/registry.cc)

TARGET_LINK_LIBRARIES(typelib_ruby typeLib)

INSTALL(TARGETS typelib_ruby
    LIBRARY DESTINATION ${RUBY_EXTENSIONS_INSTALL_DIR})
INSTALL(FILES lib/typelib.rb
    DESTINATION ${RUBY_LIBRARY_INSTALL_DIR})
