cmake_minimum_required(VERSION 3.20)
project(Tetris CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(vk-bootstrap REQUIRED)

add_executable(window_test src/experiments/window_test.cpp)
target_link_libraries(window_test PRIVATE glfw Vulkan::Vulkan vk-bootstrap::vk-bootstrap)