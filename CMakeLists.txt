cmake_minimum_required(VERSION 3.13...3.17)

project(spacedisplay LANGUAGES CXX)

#compile resources
find_package( PythonInterp 3.0 REQUIRED )
set(RES_GEN_COMMAND ${PYTHON_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/deps/resource-builder/resource-builder.py #path to python script that builds resources
        ${PROJECT_SOURCE_DIR}/res   #work directory where resources are stored
        )
execute_process(COMMAND ${RES_GEN_COMMAND})

find_package (Threads REQUIRED)

if( WIN32 )
    # In file qt5path.txt you should write path to your installed qt5. See qt5path.example.txt for an example
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/qt5path.txt")
        file (STRINGS "qt5path.txt" QT5_PATH)
        set (CMAKE_PREFIX_PATH ${QT5_PATH})
    else()
        message(FATAL_ERROR "qt5path.txt not found!\nRename qt5path.example.txt to qt5path.txt and "
                "modify it so it points to your Qt5 installation")
    endif()
endif()

find_package(Qt5 COMPONENTS Core Widgets Svg REQUIRED)

add_executable(spacedisplay "")

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
    # On Windows disable console if not in debug mode
#    if( NOT (CMAKE_BUILD_TYPE STREQUAL Debug) )
#        if (MSVC)
#            set_target_properties(${PROJECT_NAME} PROPERTIES
#                    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
#                    )
#        endif()
#    endif()
endif()
