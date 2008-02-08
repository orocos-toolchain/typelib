# - Try to find Boost include dirs and libraries
# Usage of this module as follows:
#
#     FIND_PACKAGE( Antlr )

SET(Antlr_FOUND FALSE)
FIND_PROGRAM(ANTLR NAMES cantlr antlr runantlr)

MACRO(ADD_ANTLR_GRAMMAR grammar_file output_var)
    IF (NOT Antlr_FOUND)
	MESSAGE(FATAL_ERROR "Antlr not available")
    ENDIF(NOT Antlr_FOUND)

    FILE(READ ${grammar_file} _antlr_grammar_data)

    STRING(REGEX MATCHALL "class +([^ ]+) +extends" _antlr_classes "${_antlr_grammar_data}")
    STRING(REGEX REPLACE  "class +([^ ]+) +extends" "\\1" _antlr_classes "${_antlr_classes}")

    LIST(LENGTH _antlr_classes _antlr_classes_count)
    MESSAGE(STATUS "Found ${_antlr_classes_count} Antlr classes: ${_antlr_classes}")
    STRING(REGEX REPLACE "([^;]+)" "${CMAKE_CURRENT_BINARY_DIR}/\\1.cpp;${CMAKE_CURRENT_BINARY_DIR}/\\1.hpp" _antlr_generated_files "${_antlr_classes}")

    # Trick: Antlr does not update the output files if they have
    # not been changed. This breaks builds with cmake/Makefile since
    # the source file keeps an update time > than the output files.
    # Fix it by using touch
    ADD_CUSTOM_COMMAND(OUTPUT ${_antlr_generated_files}
	COMMAND ${ANTLR} ${CMAKE_CURRENT_SOURCE_DIR}/${grammar_file}
	COMMAND touch ${_antlr_generated_files}
	DEPENDS ${grammar_file})
    SET(${output_var} "${_antlr_generated_files}")

    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ENDMACRO(ADD_ANTLR_GRAMMAR)

IF(ANTLR)
    SET(Antlr_FOUND TRUE)

    FOREACH(lang ${Antlr_FIND_COMPONENTS})
	STRING(COMPARE EQUAL "CPP" ${lang} Antlr_SEARCH_CPP)
	IF(NOT Antlr_SEARCH_CPP)
	    MESSAGE(FATAL_ERROR "unknown Antlr language package ${lang}")
	ENDIF(NOT Antlr_SEARCH_CPP)

	IF(Antlr_SEARCH_CPP)
	    SET(Antlr_FOUND FALSE)
	    FIND_PROGRAM(ANTLR_CONFIG NAMES antlr-config)

	    IF(ANTLR_CONFIG)
		SET(Antlr_FOUND TRUE)

		EXECUTE_PROCESS(COMMAND ${ANTLR_CONFIG} --cflags
		    OUTPUT_VARIABLE Antlr_CFLAGS)
		STRING(REPLACE "\n" "" Antlr_CFLAGS ${Antlr_CFLAGS})

		EXECUTE_PROCESS(COMMAND ${ANTLR_CONFIG} --libs
		    OUTPUT_VARIABLE Antlr_LIBRARIES)
		STRING(REPLACE "\n" "" Antlr_LIBRARIES ${Antlr_LIBRARIES})
		STRING(REPLACE ".a" "-pic.a" Antlr_PIC_LIBRARIES "${Antlr_LIBRARIES}")

		MARK_AS_ADVANCED(Antlr_CFLAGS Antlr_LIBRARIES Antlr_PIC_LIBRARIES)
	    ENDIF(ANTLR_CONFIG)
	ENDIF(Antlr_SEARCH_CPP)
    ENDFOREACH(lang)
ENDIF(ANTLR)

IF (Antlr_FOUND)
    IF (NOT Antlr_FIND_QUIETLY)
	MESSAGE(STATUS "Found the Antlr executable: ${ANTLR}")
    ENDIF(NOT Antlr_FIND_QUIETLY)
ELSE (Antlr_FOUND)
    IF (Antlr_FIND_REQUIRED)
	MESSAGE(FATAL_ERROR "Please install the Antlr executable and/or the ${Boost_FIND_COMPONENTS} language packages")
    ENDIF(Antlr_FIND_REQUIRED)
ENDIF(Antlr_FOUND)

