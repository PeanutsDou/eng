cmake_minimum_required(VERSION 3.16)
project(WorldBox)
set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(worldbox
    src/main.cpp
    src/engine/engine.cpp
    src/editor/editor.cpp
    src/core/shader.cpp
    src/core/mesh.cpp
    src/sprite/spritesheet.cpp
    src/imgui/imgui_impl_sdl2.cpp
    src/imgui/imgui_impl_opengl3.cpp
)
target_include_directories(worldbox PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_libraries(worldbox
    ${SDL2_LIBRARIES}
    OpenGL::GL
    GLEW::GLEW
    imgui::imgui
)

foreach(dir shaders data sprites)
    add_custom_command(TARGET worldbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/${dir}
        $<TARGET_FILE_DIR:worldbox>/${dir}
    )
endforeach()
