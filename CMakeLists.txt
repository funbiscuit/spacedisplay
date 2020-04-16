cmake_minimum_required(VERSION 3.13...3.17)

project(spacedisplay LANGUAGES CXX)

#compile resources
find_package( PythonInterp 3.0 REQUIRED )
execute_process(COMMAND ${PYTHON_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/deps/resource-builder/resource-builder.py #path to python script that builds resources
        ${PROJECT_SOURCE_DIR}/res                                       #work directory where resources are stored
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

find_package (Threads REQUIRED)

if( WIN32 )
    set(QT_WIN_PATH "C:\\" CACHE STRING "Path to installed qt binaries")
    set(WIN32_CONSOLE "On" CACHE STRING "Enable default windows console")
    message(STATUS "Looking for installed Qt5 at the following path:\n" ${QT_WIN_PATH})
    set(CMAKE_PREFIX_PATH ${QT_WIN_PATH})
endif()

find_package(Qt5 COMPONENTS Core Widgets Svg REQUIRED)

add_executable(spacedisplay)

target_compile_features(spacedisplay PUBLIC cxx_std_11)
set_target_properties(spacedisplay PROPERTIES
        CXX_EXTENSIONS OFF
        AUTOMOC ON)

add_subdirectory(deps/pfd)

add_subdirectory(src)
add_subdirectory(res)

target_include_directories(spacedisplay PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/stb
        )

target_link_libraries(spacedisplay PRIVATE
        Threads::Threads Qt5::Widgets Qt5::Svg portable_file_dialogs
        )

if( WIN32 )
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
