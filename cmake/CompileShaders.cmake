find_program(GLSLC glslangValidator)
if (NOT GLSLC)
    message(FATAL_ERROR "glslangValidator not found")
endif ()

set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADERS_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")
set(SHADERS_TYPES "vert" "frag")
file(MAKE_DIRECTORY ${SHADERS_BINARY_DIR})

set(SHADERS_BINARIES_PATHS)

if (NOT EXISTS "${SHADER_SOURCE_DIR}")
    message(WARNING "${SHADER_SOURCE_DIR} not found")
    return()
endif ()

foreach (type ${SHADERS_TYPES})
    file(GLOB shaders
            "${SHADER_SOURCE_DIR}/*.${type}"
    )

    foreach (shader ${shaders})
        get_filename_component(SHADER_NAME ${shader} NAME)
        set(SHADER_BINARY_PATH "${SHADERS_BINARY_DIR}/${SHADER_NAME}.spv")

        add_custom_command(
                OUTPUT ${SHADER_BINARY_PATH}
                COMMAND ${GLSLC} -V ${shader} -o ${SHADER_BINARY_PATH}
                DEPENDS ${shader}
                COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
                VERBATIM
        )

        list(APPEND SHADERS_BINARIES_PATHS ${SHADER_BINARY_PATH})
    endforeach ()
endforeach ()

if (NOT SHADERS_BINARIES_PATHS)
    message(WARNING "No shaders found. Check SHADER_SOURCE_DIR (${SHADER_SOURCE_DIR}) and SHADERS_TYPES (${SHADERS_TYPES})")
endif ()

add_custom_target(CompileShaders ALL
        DEPENDS ${SHADERS_BINARIES_PATHS}
)

add_dependencies(${PROJECT_NAME} CompileShaders)