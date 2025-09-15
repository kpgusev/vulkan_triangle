cmake_minimum_required(VERSION 3.31)
project(vulkan_triangle)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(CompilerSettings)
include(Dependencies)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets")
endif()
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}")

add_executable(${PROJECT_NAME} src/${PROJECT_NAME}.cc)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${glfw3_LIBRARIES}
        ${Vulkan_LIBRARIES}
        ${glm_LIBRARIES}
        ${toml11_LIBRARIES}
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

include(CompileShaders)
