
target_include_directories(spacedisplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(spacedisplay PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/colortheme.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fileentry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fileentrypool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fileentrypopup.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fileentryview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/filepath.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/resources.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/spacescanner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/spaceview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/statusview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
        )

if(WIN32)
    target_sources(spacedisplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platformutils-win.cpp)
else()
    target_sources(spacedisplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platformutils-nix.cpp)
endif()
