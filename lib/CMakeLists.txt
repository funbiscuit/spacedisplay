
find_package(Threads REQUIRED)

add_library(spacedisplay_lib STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fileentry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filedb.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filepath.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spacescanner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spacewatcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logger.cpp
        )


target_compile_features(spacedisplay_lib PUBLIC cxx_std_11)
set_target_properties(spacedisplay_lib PROPERTIES
        CXX_EXTENSIONS OFF)


target_include_directories(spacedisplay_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
        )

target_link_libraries(spacedisplay_lib PRIVATE
        Threads::Threads CRC::CRC)

if (WIN32)
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/WinFileIterator.cpp)
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/WinFileManager.cpp)
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/WinPlatformUtils.cpp)
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/WinSpaceWatcher.cpp)
else ()
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/LinuxFileIterator.cpp)
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/LinuxFileManager.cpp)
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/LinuxPlatformUtils.cpp)
    target_sources(spacedisplay_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/LinuxSpaceWatcher.cpp)
endif ()
