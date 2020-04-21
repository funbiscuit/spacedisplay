
find_package (Threads REQUIRED)

if( WIN32 )
    set(QT_WIN_PATH "C:\\" CACHE STRING "Path to installed qt binaries")
    message(STATUS "Looking for installed Qt5 at the following path:\n" ${QT_WIN_PATH})
    set(CMAKE_PREFIX_PATH ${QT_WIN_PATH})
endif()

find_package(Qt5 COMPONENTS Core Widgets Svg REQUIRED)


add_library(spacedisplay_lib STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fileentry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filedb.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fileentrypool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filepath.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spacescanner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp
        )


target_compile_features(spacedisplay_lib PUBLIC cxx_std_11)
set_target_properties(spacedisplay_lib PROPERTIES
        CXX_EXTENSIONS OFF)


target_include_directories(spacedisplay_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

target_link_libraries(spacedisplay_lib PRIVATE
        Threads::Threads CRC::CRC)

if(WIN32)
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/platformutils-win.cpp)
else()
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/platformutils-nix.cpp)
endif()
