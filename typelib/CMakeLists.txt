ADD_LIBRARY(typeLib SHARED
    registry.cc typebuilder.cc typemodel.cc typename.cc typevisitor.cc
    typedisplay.cc exporter.cc pluginmanager.cc ioplugins.cc value.cc
    importer.cc csvoutput.cc endianness.cc memory_layout.cc value_ops.cc)
TARGET_LINK_LIBRARIES(typeLib typeLang_c typeLang_tlb typeLang_idl)

install(TARGETS typeLib
    LIBRARY DESTINATION lib)
set_target_properties(typeLib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${API_VERSION})
set_target_properties(typeLib
    PROPERTIES LINK_INTERFACE_LIBRARIES "")

INSTALL(FILES exporter.hh importer.hh ioplugins.hh manip.hh pluginmanager.hh
    registry.hh registryiterator.hh typebuilder.hh typedisplay.hh typemodel.hh
    typename.hh typevisitor.hh value.hh csvoutput.hh endianness.hh
    normalized_numerics.hh memory_layout.hh value_ops.hh DESTINATION include/typelib)

