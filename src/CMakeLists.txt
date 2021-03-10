add_executable(CoronaSim main.cpp world.cpp SimManager/SimManager.cpp)

target_sources(CoronaSim PRIVATE Renderer/Renderer.cpp Renderer/Shader.cpp Renderer/Window.cpp)

target_sources(CoronaSim PRIVATE imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_opengl3.cpp imgui/backends/imgui_impl_sdl.cpp imgui/misc/cpp/imgui_stdlib.cpp)

find_package(PkgConfig REQUIRED) 

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Boost REQUIRED COMPONENTS serialization)
pkg_check_modules(sdl_gfx REQUIRED IMPORTED_TARGET SDL2_gfx)

target_link_libraries(CoronaSim PRIVATE ${SDL2_LIBRARIES} PkgConfig::sdl_gfx GLEW::GLEW OpenGL::GL Boost::boost Boost::serialization)
target_include_directories(CoronaSim PRIVATE ${SDL2_INCLUDE_DIRS} . imgui/)

target_compile_options(CoronaSim PRIVATE -Wall -Wextra -DGLM_SWIZZLE)

add_custom_target(CoronaSim_CopyFiles COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/res)

add_dependencies(CoronaSim CoronaSim_CopyFiles)
