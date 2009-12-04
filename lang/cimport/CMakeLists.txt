#SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)
FIND_PACKAGE(Antlr REQUIRED COMPONENTS CPP)
ADD_ANTLR_GRAMMAR(cparser.g cparser_output)

ADD_LIBRARY(typeLang_c SHARED import.cc Dictionary.cc Support.cc
	    typesolver.cc packing.cc containers.cc ${cparser_output})
TARGET_LINK_LIBRARIES(typeLang_c typeLib ${Antlr_PIC_LIBRARIES} ${Utilmm_LIBRARIES})

INSTALL(TARGETS typeLang_c
    LIBRARY DESTINATION lib/typelib)
    
