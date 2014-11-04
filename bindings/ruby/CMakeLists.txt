INCLUDE(RubyExtensions)

if (RUBY_VERSION_MAJOR EQUAL 1 AND RUBY_VERSION_MINOR LESS 9)
    message(FATAL_ERROR "typelib required Ruby version 1.9.3 or later")
endif()

if(YARD_EXECUTABLE)
    ADD_CUSTOM_TARGET(ruby_doc yard -o ${CMAKE_BINARY_DIR}/doc/html/ruby/
        --title "Typelib: a C type/value manipulation library" --main README.txt lib/**/*.rb ext/**/*.cc README.txt
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
elseif(RDOC_EXECUTABLE)
    ADD_CUSTOM_TARGET(ruby_doc rdoc -o ${CMAKE_BINARY_DIR}/doc/html/ruby/ --title "Typelib: a C type/value manipulation library" --main README.txt -U ext lib README.txt
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

ADD_RUBY_EXTENSION(typelib_ruby 
    ext/typelib_ruby.cc ext/value.cc ext/strings.cc
    ext/convert.cc ext/specialized_types.cc 
    ext/registry.cc ext/memory.cc ext/metadata.cc)

TARGET_LINK_LIBRARIES(typelib_ruby typeLib typeLang_cSupport ${Utilmm_LIBRARIES})

CONFIGURE_FILE(typelib_ruby.pc.in ${CMAKE_CURRENT_BINARY_DIR}/typelib_ruby.pc @ONLY)

INSTALL(TARGETS typelib_ruby
    LIBRARY DESTINATION ${RUBY_EXTENSIONS_INSTALL_DIR})
INSTALL(FILES ext/typelib.hh ext/typelib_ruby.hh
    DESTINATION ${RUBY_EXTENSIONS_INSTALL_DIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/typelib_ruby.pc DESTINATION lib/pkgconfig)
INSTALL(FILES lib/typelib.rb
    DESTINATION ${RUBY_LIBRARY_INSTALL_DIR})
INSTALL(DIRECTORY lib/typelib
    DESTINATION ${RUBY_LIBRARY_INSTALL_DIR})

