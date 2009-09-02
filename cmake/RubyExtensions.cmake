# This module finds the Ruby package and defines a ADD_RUBY_EXTENSION macro to
# build and install Ruby extensions
# 
# Upon loading, it sets a RUBY_EXTENSIONS_AVAILABLE variable to true if Ruby
# extensions can be built.
#
# The ADD_RUBY_EXTENSION macro can be used as follows:
#  ADD_RUBY_EXTENSION(target_name source1 source2 source3 ...)
#
# 

FIND_PACKAGE(Ruby)
IF(NOT RUBY_LIBRARY)
    MESSAGE(STATUS "Ruby library not found. Cannot build Ruby extensions")
ELSE(NOT RUBY_LIBRARY)
    SET(RUBY_EXTENSIONS_AVAILABLE TRUE)
    STRING(REGEX REPLACE ".*lib/?" "lib/" RUBY_EXTENSIONS_INSTALL_DIR ${RUBY_ARCH_DIR})
    STRING(REGEX REPLACE ".*lib/?" "lib/" RUBY_LIBRARY_INSTALL_DIR ${RUBY_RUBY_LIB_PATH})

    FIND_PROGRAM(RDOC_EXECUTABLE NAMES rdoc1.8 rdoc)

    EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "puts Config::CONFIG['CFLAGS']"
       OUTPUT_VARIABLE RUBY_CFLAGS)
    STRING(REPLACE "\n" "" RUBY_CFLAGS ${RUBY_CFLAGS})

    MACRO(ADD_RUBY_EXTENSION target)
	INCLUDE_DIRECTORIES(${RUBY_INCLUDE_PATH})
	GET_FILENAME_COMPONENT(rubylib_path ${RUBY_LIBRARY} PATH)
	LINK_DIRECTORIES(${rubylib_path})

	SET_SOURCE_FILES_PROPERTIES(${ARGN} PROPERTIES COMPILE_FLAGS "${RUBY_CFLAGS}")
	ADD_LIBRARY(${target} MODULE ${ARGN})
        set_target_properties(${target} PROPERTIES
            LINK_FLAGS "-z noexecstack")
	SET_TARGET_PROPERTIES(${target} PROPERTIES PREFIX "")
    ENDMACRO(ADD_RUBY_EXTENSION)
ENDIF(NOT RUBY_LIBRARY)

