add_executable(spacedisplay_test DirHelper.cpp)

target_include_directories(spacedisplay_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

target_sources(spacedisplay_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/FileIteratorTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FileEntryTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FileDBTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FilePathTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ScannerTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SpaceWatcherTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/UtilsTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/LoggerTest.cpp
        )

target_link_libraries(spacedisplay_test PRIVATE spacedisplay_lib)
target_link_libraries(spacedisplay_test PRIVATE Catch2WithMain)
