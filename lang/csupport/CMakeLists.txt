add_library(typeLang_cSupport SHARED containers.cc plugin.cc standard_types.cc)
target_link_libraries(typeLang_cSupport typeLib)

install(TARGETS typeLang_cSupport
    LIBRARY DESTINATION lib/typelib)

