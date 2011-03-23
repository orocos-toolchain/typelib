#SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)
FIND_PACKAGE(Antlr REQUIRED COMPONENTS CPP)
ADD_ANTLR_GRAMMAR(cparser.g cparser_output)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../csupport)
ADD_LIBRARY(typeLang_c SHARED import.cc Dictionary.cc Support.cc
	    typesolver.cc packing.cc plugin.cc ${cparser_output})
TARGET_LINK_LIBRARIES(typeLang_c typeLang_cSupport typeLib ${Antlr_PIC_LIBRARIES} ${Utilmm_LIBRARIES} ${Boost_FILESYSTEM_LIBRARIES})

INSTALL(TARGETS typeLang_c
    LIBRARY DESTINATION lib/typelib)
    
