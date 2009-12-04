ADD_LIBRARY(typeLang_idl SHARED
    export.cc)
TARGET_LINK_LIBRARIES(typeLang_idl typeLib ${LibXML_LIBRARIES})

INSTALL(TARGETS typeLang_idl
    LIBRARY DESTINATION lib/typelib)
    
