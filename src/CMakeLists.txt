add_executable(CoronaSim main.cpp world.cpp SimManager/SimManager.cpp)

target_sources(CoronaSim PRIVATE Renderer/Renderer.cpp Renderer/Shader.cpp Renderer/Window.cpp)


find_package(PkgConfig REQUIRED) 

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
pkg_check_modules(sdl_gfx REQUIRED IMPORTED_TARGET SDL2_gfx)

target_link_libraries(CoronaSim PRIVATE ${SDL2_LIBRARIES} PkgConfig::sdl_gfx GLEW::GLEW)
target_include_directories(CoronaSim PRIVATE ${SDL2_INCLUDE_DIRS} .)

target_compile_options(CoronaSim PRIVATE -Wall -Wextra)
