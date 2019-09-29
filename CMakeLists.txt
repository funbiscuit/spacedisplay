cmake_minimum_required(VERSION 3.1)

project(spacedisplay LANGUAGES CXX)

#compile resources
find_package( PythonInterp 3.0 REQUIRED )
set(RES_GEN_COMMAND ${PYTHON_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/deps/resource-builder/resource-builder.py #path to python script that builds resources
        ${PROJECT_SOURCE_DIR}/res   #work directory where resources are stored
        )
execute_process(COMMAND ${RES_GEN_COMMAND})


#when compiling with msvc set boost include directory manually
#in my case I just use boost installed to mingw64 since I use only headers
#and don't care about compiled libraries
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    #TODO replace with something else
    set(Boost_INCLUDE_DIRS "C:\\msys64\\mingw64\\include")
else()
    find_package (Boost REQUIRED)
endif()

include_directories(${Boost_INCLUDE_DIRS})
find_package (Threads REQUIRED)

if( WIN32 )
    set (CMAKE_PREFIX_PATH "C:\\Qt\\Qt5.13.1\\5.13.1\\msvc2015_64\\")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Qt5 COMPONENTS Widgets REQUIRED)


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

    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)
else()
    file(GLOB all_OBJS ${PROJECT_SOURCE_DIR}/res/build/objs/*.o)
    add_executable(${PROJECT_NAME} ${all_SRCS})
    target_link_libraries(${PROJECT_NAME}
            PRIVATE ${CMAKE_THREAD_LIBS_INIT}
            PRIVATE ${all_OBJS}
            PRIVATE Qt5::Widgets)
endif()