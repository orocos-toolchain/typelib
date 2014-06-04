ADD_LIBRARY(typeLib SHARED
    registry.cc typebuilder.cc typemodel.cc typename.cc typevisitor.cc
    typedisplay.cc exporter.cc pluginmanager.cc value.cc
    importer.cc csvoutput.cc endianness.cc memory_layout.cc value_ops.cc
    utilmm/configset.cc utilmm/singleton/server.cc utilmm/singleton/dummy.cc)

if (NOT TYPELIB_PLUGIN_PATH)
set_source_files_properties(pluginmanager.cc PROPERTIES
   COMPILE_FLAGS -DTYPELIB_PLUGIN_PATH="\\"${CMAKE_INSTALL_PREFIX}/lib/typelib\\"")
else (NOT TYPELIB_PLUGIN_PATH)
set_source_files_properties(pluginmanager.cc PROPERTIES
   COMPILE_FLAGS -DTYPELIB_PLUGIN_PATH="\\"${TYPELIB_PLUGIN_PATH}\\"")
endif (NOT TYPELIB_PLUGIN_PATH)

set_source_files_properties(endianness.cc PROPERTIES
    COMPILE_FLAGS -fno-strict-aliasing)

target_link_libraries(typeLib ${CMAKE_DL_LIBS} ${Boost_FILESYSTEM_LIBRARIES} ${Boost_SYSTEM_LIBRARIES})
#TARGET_LINK_LIBRARIES(typeLib typeLang_c typeLang_tlb typeLang_idl)

install(TARGETS typeLib
    LIBRARY DESTINATION lib)
set_target_properties(typeLib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${API_VERSION})

INSTALL(FILES exporter.hh importer.hh plugins.hh manip.hh pluginmanager.hh
    registry.hh registryiterator.hh typebuilder.hh typedisplay.hh typemodel.hh
    typename.hh typevisitor.hh value.hh csvoutput.hh endianness.hh
    normalized_numerics.hh memory_layout.hh value_ops.hh endian_swap.hh DESTINATION include/typelib)

install(DIRECTORY utilmm DESTINATION include/typelib)
