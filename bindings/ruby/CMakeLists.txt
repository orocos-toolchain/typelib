INCLUDE(RubyExtensions)

if(YARD_EXECUTABLE)
    ADD_CUSTOM_TARGET(ruby_doc yard -o ${CMAKE_BINARY_DIR}/doc/html/ruby/ --title "Typelib: a C type/value manipulation library" --main README.txt -U ext lib README.txt
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
elseif(RDOC_EXECUTABLE)
    ADD_CUSTOM_TARGET(ruby_doc rdoc -o ${CMAKE_BINARY_DIR}/doc/html/ruby/ --title "Typelib: a C type/value manipulation library" --main README.txt -U ext lib README.txt
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

SET(SUPPORTED_DYNCALL_SYSTEMS Linux-i486 Linux-i586 Linux-i686 Linux-x86_64 Linux-ppc)
LIST(FIND SUPPORTED_DYNCALL_SYSTEMS ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR} SYSTEM_SUPPORTED)

SET(ENABLE_DYNCALL ON)
IF(${SYSTEM_SUPPORTED} EQUAL -1)
    MESSAGE(STATUS "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR} is not (yet) supported by dyncall. dyncall support disabled")
    SET(ENABLE_DYNCALL OFF)
ENDIF(${SYSTEM_SUPPORTED} EQUAL -1)

IF(ENABLE_DYNCALL)
    MESSAGE(STATUS "Ruby: enabled dyncall support")
    SET(DYNCALL_SOURCE_DIR  "${CMAKE_CURRENT_BINARY_DIR}/dyncall-0.6")
    SET(DYNCALL_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/dyncall")
    SET(DYNCALL_INCLUDE_DIR ${DYNCALL_INSTALL_DIR}/include)
    SET(DYNCALL_LIBRARIES ${DYNCALL_INSTALL_DIR}/lib/libdyncall_s.a ${DYNCALL_INSTALL_DIR}/lib/libdynload_s.a)
    INCLUDE_DIRECTORIES("${DYNCALL_INSTALL_DIR}/include")
    LINK_DIRECTORIES("${DYNCALL_INSTALL_DIR}/lib")

    ADD_CUSTOM_COMMAND(OUTPUT ${DYNCALL_SOURCE_DIR}/ConfigVars
        COMMAND tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/dyncall-0.6.tar.gz
        COMMAND cd ${DYNCALL_SOURCE_DIR} && ./configure --prefix=${DYNCALL_INSTALL_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dyncall-0.6.tar.gz)

    ADD_CUSTOM_COMMAND(OUTPUT ${DYNCALL_LIBRARIES}
        COMMAND $(MAKE) -j1 "CFLAGS=-O3 -fPIC" "CC=gcc"
        COMMAND $(MAKE) -j1 install
        DEPENDS ${DYNCALL_SOURCE_DIR}/ConfigVars
        WORKING_DIRECTORY ${DYNCALL_SOURCE_DIR})

    ADD_CUSTOM_TARGET(dyncall DEPENDS ${DYNCALL_LIBRARIES})
    ADD_DEFINITIONS(-DWITH_DYNCALL)

    SET(DYNCALL_INTERFACE ext/functions.cc)
ENDIF(ENABLE_DYNCALL)

ADD_RUBY_EXTENSION(typelib_ruby 
    ext/typelib_ruby.cc ext/value.cc ext/strings.cc
    ext/convert.cc ext/specialized_types.cc 
    ext/registry.cc ext/memory.cc ${DYNCALL_INTERFACE})

IF(ENABLE_DYNCALL)
    TARGET_LINK_LIBRARIES(typelib_ruby ${DYNCALL_LIBRARIES})
    ADD_DEPENDENCIES(typelib_ruby dyncall)
ENDIF(ENABLE_DYNCALL)

TARGET_LINK_LIBRARIES(typelib_ruby typeLib typeLang_cSupport ${Utilmm_LIBRARIES})

CONFIGURE_FILE(typelib_ruby.pc.in ${CMAKE_CURRENT_BINARY_DIR}/typelib_ruby.pc @ONLY)

INSTALL(TARGETS typelib_ruby
    LIBRARY DESTINATION ${RUBY_EXTENSIONS_INSTALL_DIR})
INSTALL(FILES ext/typelib_ruby.hh
    DESTINATION ${RUBY_EXTENSIONS_INSTALL_DIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/typelib_ruby.pc DESTINATION lib/pkgconfig)
INSTALL(FILES lib/typelib.rb lib/typelib-gccxml.rb
    DESTINATION ${RUBY_LIBRARY_INSTALL_DIR})

