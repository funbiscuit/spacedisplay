cmake_minimum_required(VERSION 3.8...3.17)

project(spacedisplay LANGUAGES CXX)

#compile resources
find_package( PythonInterp 3.0 REQUIRED )
execute_process(COMMAND ${PYTHON_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/deps/resource-builder/resource-builder.py #path to python script that builds resources
        ${PROJECT_SOURCE_DIR}/res                                       #work directory where resources are stored
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(spacedisplay src/main.cpp)

add_subdirectory(deps/pfd)

add_subdirectory(lib)
add_subdirectory(res)

add_subdirectory(tests)

target_link_libraries(spacedisplay PRIVATE spacedisplay_lib)

if( WIN32 )
    set(WIN32_CONSOLE "On" CACHE STRING "Enable default windows console")
    # On Windows disable console if not needed
    if(NOT ${WIN32_CONSOLE})
        message(STATUS "Console is disabled")
        if (MSVC) # msvc compiler
            set_target_properties(spacedisplay PROPERTIES
                    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
                    )
        else() # gcc compiler
            set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -mwindows")
        endif()
    endif()
endif()
