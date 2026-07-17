cmake_minimum_required(VERSION 3.16)
project(WorldBox)
set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(worldbox main.cpp mesh.cpp)
target_include_directories(worldbox PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(worldbox ${SDL2_LIBRARIES} OpenGL::GL GLEW::GLEW)
