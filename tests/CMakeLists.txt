add_executable(spacedisplay_test catch.cpp)

target_include_directories(spacedisplay_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../deps/catch2
        )

target_sources(spacedisplay_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/FilePathTest.cpp
        )

target_link_libraries(spacedisplay_test PRIVATE spacedisplay_lib)
