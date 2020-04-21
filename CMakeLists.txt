cmake_minimum_required(VERSION 3.8...3.17)

project(spacedisplay LANGUAGES C CXX)

add_subdirectory(deps/pfd)
add_subdirectory(lib)
add_subdirectory(res)
add_subdirectory(app-gui)

add_subdirectory(tests)
