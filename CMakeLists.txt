cmake_minimum_required(VERSION 3.8...3.17)

project(spacedisplay LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake-modules")

option(BUILD_TESTS "Build test programs" OFF)
option(TESTS_COV "Run coverage on tests" OFF)


if (${TESTS_COV})
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif ()
# add dependencies
add_subdirectory(deps)

# add spacescanner library
add_subdirectory(lib)

# add resources library
add_subdirectory(res)

# add gui executable
add_subdirectory(app-gui)

# add tests
if (${BUILD_TESTS})
    add_subdirectory(tests)
    if (${TESTS_COV})
        setup_target_for_coverage_gcovr_xml(
                NAME coverage_xml
                DEPENDENCIES spacedisplay_test spacedisplay_lib
                EXCLUDE "app-gui/*" "deps/*" "res/*" "tests/*"
                "${PROJECT_BINARY_DIR}/app-gui/*"
                EXECUTABLE spacedisplay_test)
        setup_target_for_coverage_gcovr_html(
                NAME coverage_html
                DEPENDENCIES spacedisplay_test spacedisplay_lib
                EXCLUDE "app-gui/*" "deps/*" "res/*" "tests/*"
                "${PROJECT_BINARY_DIR}/app-gui/*"
                EXECUTABLE spacedisplay_test)
    endif ()
endif ()
