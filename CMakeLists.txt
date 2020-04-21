cmake_minimum_required(VERSION 3.8...3.17)

project(spacedisplay LANGUAGES C CXX)

# add dependencies
add_subdirectory(deps)

# add spacescanner library
add_subdirectory(lib)

# add resources library
add_subdirectory(res)

# add gui executable
add_subdirectory(app-gui)

# add tests
add_subdirectory(tests)
