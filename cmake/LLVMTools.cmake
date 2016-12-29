macro(llvm_find_config LLVM_REQUIRED_VERSION)
    if (LLVM_REQUIRED_VERSION STREQUAL "")
        set(__llvm_names "llvm-config;llvm-config-3.6;llvm-config-3.5;llvm-config-3.4")
    else()
        set(__llvm_names llvm-config;llvm-config-${LLVM_REQUIRED_VERSION})
    endif()
    message("-- llvm: looking for ${__llvm_names}")
    find_program(LLVM_CONFIG_EXECUTABLE NAMES ${__llvm_names})
    if (NOT LLVM_CONFIG_EXECUTABLE)
        message("-- llvm: not found (candidates: ${__llvm_names})")
    else()
        message("-- llvm: found ${LLVM_CONFIG_EXECUTABLE}")
    endif()
endmacro()

function(llvm_has_component VAR name)
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --components
        OUTPUT_VARIABLE llvm_component_list)

    string(REPLACE " " ";" llvm_component_list "${llvm_component_list}")
    list(FIND llvm_component_list "clang" ${VAR})
    set(VAR ${VAR} PARENT_SCOPE)
endfunction()

macro(llvm_get_config OUTPUT)
    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} ${ARGN}
        RESULT_VARIABLE __llvm_ok
        OUTPUT_VARIABLE ${OUTPUT})
    if (NOT __llvm_ok STREQUAL 0)
        message(FATAL_ERROR "failed to execute llvm-config ${ARGN} (result: ${__llvm_ok})")
    endif()
    string(STRIP ${OUTPUT} "${${OUTPUT}}")
    string(REPLACE "\n" "" ${OUTPUT} "${${OUTPUT}}")
endmacro()

