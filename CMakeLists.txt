cmake_minimum_required(VERSION 3.3.2)

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

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Qt5 COMPONENTS Core Widgets Svg REQUIRED)

include_directories(
        ${PROJECT_SOURCE_DIR}/inc
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/res/build/include
        ${PROJECT_SOURCE_DIR}/deps/pfd
        ${PROJECT_SOURCE_DIR}/deps/stb
        ${PROJECT_SOURCE_DIR}/deps/nanosvg/src
)

file(GLOB all_SRCS
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/res/build/src/resources.cpp
        )

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if( WIN32 )
    add_executable(${PROJECT_NAME} ${all_SRCS}
            ${PROJECT_SOURCE_DIR}/res/win-res.rc
            ${PROJECT_SOURCE_DIR}/res/build/src/win.rc
            )

    ## On Windows disable console if not in debug mode
#    if( NOT (CMAKE_BUILD_TYPE STREQUAL Debug) )
#        if (MSVC)
#            set_target_properties(${PROJECT_NAME} PROPERTIES
#                    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
#                    )
#        endif()
#    endif()

    target_link_libraries(${PROJECT_NAME}
            PRIVATE Qt5::Widgets
            PRIVATE Qt5::Svg
            )
else()
    file(GLOB all_OBJS ${PROJECT_SOURCE_DIR}/res/build/objs/*.o)
    add_executable(${PROJECT_NAME} ${all_SRCS})
    target_link_libraries(${PROJECT_NAME}
            PRIVATE ${CMAKE_THREAD_LIBS_INIT}
            PRIVATE ${all_OBJS}
            PRIVATE Qt5::Widgets
            PRIVATE Qt5::Svg
            )
endif()
