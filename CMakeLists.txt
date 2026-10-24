cmake_minimum_required(VERSION 3.20)
project(Tetris CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_program(GLSLC glslc REQUIRED)

add_executable(tetris
    src/main.cpp
    src/Grid.cpp
    src/Block.cpp
    src/Blocks.cpp
    src/Engine.cpp
    src/Renderer.cpp
    src/Game.cpp
)

target_include_directories(tetris PRIVATE src/include)
target_link_libraries(tetris PRIVATE glfw Vulkan::Vulkan vk-bootstrap::vk-bootstrap)

set(SHADER_SOURCES
    ${CMAKE_SOURCE_DIR}/shaders/tetris.vert
    ${CMAKE_SOURCE_DIR}/shaders/tetris.frag
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV ${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSLC} ${SHADER} -o ${SPIRV}
        DEPENDS ${SHADER}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(tetris shaders)
