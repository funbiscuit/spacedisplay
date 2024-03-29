

if (WIN32)
    set(QT_WIN_PATH "C:\\" CACHE STRING "Path to installed qt binaries")
    message(STATUS "Looking for installed Qt5 at the following path:\n" ${QT_WIN_PATH})
    set(CMAKE_PREFIX_PATH ${QT_WIN_PATH})
endif ()

find_package(Qt5 COMPONENTS Core Widgets Svg REQUIRED)

add_executable(spacedisplay_gui
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        )

# sources for gui
add_library(spacedisplay_gui_lib STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mainapp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/customtheme.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/customstyle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fileentrypopup.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fileentryview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filetooltip.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fileviewdb.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mainwindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/resources.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spaceview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/statusview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/inotifydialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils-gui.cpp
        )

# headers for moc generation
target_sources(spacedisplay_gui_lib PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fileentrypopup.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mainwindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/spaceview.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/statusview.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/customstyle.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/inotifydialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/logdialog.h
        )

target_compile_features(spacedisplay_gui_lib PUBLIC cxx_std_11)
set_target_properties(spacedisplay_gui_lib PROPERTIES
        CXX_EXTENSIONS OFF
        AUTOMOC ON)

target_include_directories(spacedisplay_gui_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

target_link_libraries(spacedisplay_gui_lib PUBLIC
        Qt5::Widgets Qt5::Svg portable_file_dialogs
        SpacedisplayRes::SpacedisplayRes spacedisplay_lib
        )

target_link_libraries(spacedisplay_gui PRIVATE spacedisplay_gui_lib)

if (WIN32)
    option(WIN32_CONSOLE "Enable default windows console" ON)
    # On Windows disable console if not needed
    if (NOT ${WIN32_CONSOLE})
        message(STATUS "Console is disabled")
        if (MSVC) # msvc compiler
            set_target_properties(spacedisplay_gui PROPERTIES
                    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
                    )
        else () # gcc compiler
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
        endif ()
    endif ()
endif ()

install(TARGETS spacedisplay_gui
        DESTINATION bin)
