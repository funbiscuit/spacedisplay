add_executable(spacedisplay_test catch.cpp)

target_include_directories(spacedisplay_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../deps/catch2
        )

target_sources(spacedisplay_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/FileEntryTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FilePathTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SpaceWatcherTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/UtilsTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DirHelper.cpp
        )

target_link_libraries(spacedisplay_test PRIVATE spacedisplay_lib)
